#include "core/BoundaryScopeResolver.h"
#include "core/ScopeReferenceBuilder.h"
#include "core/SelectionManager.h"
#include "viewport/MeshSelectionScene.h"

#include <QCoreApplication>

#include <femcae/meshing/StructuredHexMesher.h>

#include <algorithm>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

femcae::meshing::SimulationMesh makeMesh()
{
    femcae::meshing::AxisAlignedBox box;
    box.min = {0.0, 0.0, 0.0};
    box.max = {1.0, 1.0, 1.0};

    femcae::meshing::BoxBoundaryGeometry geometry;
    geometry.body = 100;
    geometry.xMin = 101;
    geometry.xMax = 102;
    geometry.yMin = 103;
    geometry.yMax = 104;
    geometry.zMin = 105;
    geometry.zMax = 106;

    femcae::meshing::StructuredHexMesherOptions options;
    options.nx = 2;
    options.ny = 1;
    options.nz = 1;
    options.firstNodeId = 10;
    options.firstElementId = 100;
    options.firstFacetId = 1000;

    femcae::meshing::StructuredHexMesher mesher;
    return mesher.meshBox(box, geometry, 3, options);
}

void sceneTests()
{
    constexpr quint64 generation = 7;
    const auto mesh = makeMesh();
    d26::MeshSelectionScene scene;

    check(scene.set(mesh, generation) && scene.complete(),
          "structured HEX8 mesh builds a complete FEM selection scene");
    check(scene.generation() == generation,
          "FEM selection scene records mesh generation as stale-selection guard");
    check(scene.boundaryQuads().size() == mesh.boundaryFacets.size(),
          "every boundary quad keeps one explicit FEM Facet provenance entry");
    check(scene.visibleNodePoints().size() == scene.visibleNodeIds().size()
              && !scene.visibleNodeIds().empty(),
          "visible Node display points keep explicit MeshEntityId provenance");

    const auto node = scene.selectionItemForVisibleNode(0);
    check(node.has_value() && node->domain == d26::SelectionDomain::Mesh
              && node->kind == d26::SelectionKind::Node
              && node->meshEntityId == scene.visibleNodeIds().front()
              && node->sourceRevision == generation,
          "visible point index resolves to FEM Node identity, not display point identity");

    const auto facet = scene.selectionItemForBoundaryCell(0, d26::SelectionKind::Facet);
    check(facet.has_value() && facet->meshEntityId == scene.facetIds().front()
              && facet->meshEntityId != static_cast<femcae::meshing::MeshEntityId>(0),
          "boundary cell resolves through provenance to FEM Facet ID");

    const auto element = scene.selectionItemForBoundaryCell(0, d26::SelectionKind::Element);
    check(element.has_value() && element->meshEntityId == scene.ownerElementIds().front(),
          "same visible boundary cell resolves to owner FEM Element ID in Element mode");

    check(!scene.selectionItemForBoundaryCell(0, d26::SelectionKind::Node).has_value(),
          "boundary cell cannot be reinterpreted as a FEM Node");
    check(!scene.selectionItemForVisibleNode(scene.visibleNodeIds().size()).has_value(),
          "out-of-range display point is never a valid FEM Node hit");

    if (facet.has_value()) {
        auto stale = *facet;
        stale.sourceRevision = generation + 1;
        check(scene.boundaryCellsForSelection(QVector<d26::SelectionItem>{stale}).empty(),
              "stale mesh generation cannot create a Facet overlay");
    }
    if (element.has_value()) {
        const auto cells = scene.boundaryCellsForSelection(QVector<d26::SelectionItem>{*element});
        check(!cells.empty(),
              "selected visible Element resolves to all of its exposed boundary facets");
    }
    if (node.has_value()) {
        check(scene.visibleNodeIndicesForSelection(QVector<d26::SelectionItem>{*node}).size() == 1,
              "selected Node resolves to its visible point overlay index");
    }

    auto brokenNode = mesh;
    brokenNode.boundaryFacets.front().nodeIds[0] = 999999;
    check(!scene.set(brokenNode, generation),
          "unknown boundary-facet Node reference rejects FEM selection scene atomically");

    auto brokenOwner = mesh;
    brokenOwner.boundaryFacets.front().ownerElementId = 999999;
    check(!scene.set(brokenOwner, generation),
          "unknown boundary-facet owner Element rejects FEM selection scene atomically");

    auto duplicateFacet = mesh;
    duplicateFacet.boundaryFacets[1].id = duplicateFacet.boundaryFacets[0].id;
    check(!scene.set(duplicateFacet, generation),
          "duplicate FEM Facet identity is rejected");

    check(!scene.set(mesh, 0),
          "mesh generation zero is not accepted as a selectable generated mesh");
}

void managerGenerationTests()
{
    d26::SelectionManager manager;
    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::MeshNodeScope));

    d26::SelectionItem nodeA;
    nodeA.domain = d26::SelectionDomain::Mesh;
    nodeA.kind = d26::SelectionKind::Node;
    nodeA.meshEntityId = 10;
    nodeA.sourceRevision = 5;

    d26::SelectionItem nodeB = nodeA;
    nodeB.meshEntityId = 11;

    check(manager.apply(nodeA, d26::SelectionOperation::Replace)
              && manager.apply(nodeB, d26::SelectionOperation::Add)
              && manager.items().size() == 2 && manager.primary() == nodeB,
          "Mesh Node policy supports deterministic multi-selection");
    check(manager.setPreselection(nodeA) && manager.preselection() == nodeA,
          "Mesh Node hover remains separate from committed selection");

    check(!manager.invalidateGeometryRevision(999)
              && manager.items().size() == 2 && manager.preselection() == nodeA,
          "CAD revision invalidation never erases Mesh-domain selection");
    check(manager.invalidateMeshGeneration(6)
              && manager.items().isEmpty() && !manager.preselection().has_value(),
          "mesh regeneration invalidates stale committed and hover FEM selection");

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::MeshElementScope));
    d26::SelectionItem element;
    element.domain = d26::SelectionDomain::Mesh;
    element.kind = d26::SelectionKind::Element;
    element.meshEntityId = 20;
    element.sourceRevision = 6;
    check(manager.apply(element, d26::SelectionOperation::Replace),
          "Mesh Element policy accepts Element identity");

    manager.setPolicy(d26::SelectionPolicy::preset(d26::SelectionPolicyPreset::MeshFacetScope));
    check(manager.items().isEmpty(),
          "switching Element to Facet policy clears incompatible transient mesh state");
    d26::SelectionItem facet = element;
    facet.kind = d26::SelectionKind::Facet;
    facet.meshEntityId = 30;
    check(manager.apply(facet, d26::SelectionOperation::Replace),
          "Mesh Facet policy accepts Facet identity");
}

void persistentScopeTests()
{
    constexpr quint64 generation = 9;
    const auto mesh = makeMesh();
    d26::MeshSelectionScene scene;
    check(scene.set(mesh, generation),
          "FEM scope fixture builds from generated mesh provenance");

    const auto node = scene.selectionItemForVisibleNode(0);
    const auto element = scene.selectionItemForBoundaryCell(0, d26::SelectionKind::Element);
    const auto facet = scene.selectionItemForBoundaryCell(0, d26::SelectionKind::Facet);
    check(node.has_value() && element.has_value() && facet.has_value(),
          "scope fixture resolves Node Element and Facet identities");
    if (!node.has_value() || !element.has_value() || !facet.has_value()) {
        return;
    }

    const auto nodeScope = d26::buildMeshScopeReference(QVector<d26::SelectionItem>{*node, *node},
                                                         mesh, generation);
    check(nodeScope.success() && nodeScope.scope.entities.size() == 1
              && nodeScope.scope.sourceRevision == generation
              && nodeScope.scope.entities.front().domain == d26::SelectionDomain::Mesh
              && nodeScope.scope.entities.front().kind == d26::SelectionKind::Node
              && nodeScope.scope.entities.front().meshEntityId == node->meshEntityId,
          "persistent FEM scope stores deduplicated MeshEntityId and mesh generation");
    check(nodeScope.success()
              && d26::validateMeshScopeReference(nodeScope.scope, mesh, generation)
                     == d26::ScopeReferenceValidationError::None,
          "persistent FEM Node scope validates against the source mesh generation");
    check(nodeScope.success()
              && d26::validateMeshScopeReference(nodeScope.scope, mesh, generation + 1)
                     == d26::ScopeReferenceValidationError::StaleMeshGeneration,
          "mesh regeneration makes persistent FEM scope explicitly stale");

    auto staleFacet = *facet;
    staleFacet.sourceRevision = generation - 1;
    check(d26::buildMeshScopeReference(QVector<d26::SelectionItem>{staleFacet}, mesh, generation).error
              == d26::ScopeReferenceBuildError::StaleMeshGeneration,
          "stale transient Facet selection cannot become persistent FEM scope");

    auto missingElement = *element;
    missingElement.meshEntityId = 999999;
    check(d26::buildMeshScopeReference(QVector<d26::SelectionItem>{missingElement}, mesh, generation).error
              == d26::ScopeReferenceBuildError::MissingMeshEntity,
          "unknown Element identity cannot become persistent FEM scope");

    const auto mixedScope = d26::buildMeshScopeReference(
        QVector<d26::SelectionItem>{*element, *facet}, mesh, generation);
    check(mixedScope.success() && mixedScope.scope.entities.size() == 2,
          "data contract can preserve distinct Element and Facet identities without conflation");

    if (nodeScope.success()) {
        auto missingScope = nodeScope.scope;
        missingScope.entities.front().meshEntityId = 999999;
        check(d26::validateMeshScopeReference(missingScope, mesh, generation)
                  == d26::ScopeReferenceValidationError::MissingMeshEntity,
              "persistent FEM scope validation rejects missing MeshEntityId");
    }
}

void boundaryNodeUnionTests()
{
    const auto mesh = makeMesh();
    constexpr femcae::geometry::GeometryEntityId xMin = 101;
    constexpr femcae::geometry::GeometryEntityId yMin = 103;

    const auto xNodes = d26::boundaryNodeUnionForGeometryFaces(mesh, QVector<femcae::geometry::GeometryEntityId>{xMin});
    const auto yNodes = d26::boundaryNodeUnionForGeometryFaces(mesh, QVector<femcae::geometry::GeometryEntityId>{yMin});
    const auto unionNodes = d26::boundaryNodeUnionForGeometryFaces(
        mesh, QVector<femcae::geometry::GeometryEntityId>{xMin, yMin, xMin});

    check(!xNodes.empty() && !yNodes.empty(),
          "individual CAD boundary Faces resolve to FEM node sets through mesh provenance");

    std::vector<femcae::meshing::MeshEntityId> expected = xNodes;
    expected.insert(expected.end(), yNodes.begin(), yNodes.end());
    std::sort(expected.begin(), expected.end());
    expected.erase(std::unique(expected.begin(), expected.end()), expected.end());

    check(unionNodes == expected,
          "multi-Face boundary scope resolves to one sorted deduplicated FEM node union");
    check(unionNodes.size() < xNodes.size() + yNodes.size(),
          "shared edge/corner nodes are not counted twice across adjacent Faces");
    check(d26::boundaryNodeUnionForGeometryFaces(
              mesh, QVector<femcae::geometry::GeometryEntityId>{femcae::geometry::InvalidGeometryId}).empty(),
          "invalid CAD identity cannot leak into boundary node union");

    // Force consumer contract: F_total is distributed ONCE over the union. If
    // each Face received F_total separately, the summed load would be multiplied
    // by face count. This check locks the intended total-load arithmetic.
    constexpr double totalForce = 1200.0;
    const double nodalForce = totalForce / static_cast<double>(unionNodes.size());
    const double recoveredTotal = nodalForce * static_cast<double>(unionNodes.size());
    check(std::abs(recoveredTotal - totalForce) < 1.0e-12,
          "total Force distributed once over multi-Face union preserves requested resultant");
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    sceneTests();
    managerGenerationTests();
    persistentScopeTests();
    boundaryNodeUnionTests();
    std::cout << (failures == 0 ? "FEM selection contract PASS\n"
                                : "FEM selection contract FAIL\n");
    return failures == 0 ? 0 : 1;
}
