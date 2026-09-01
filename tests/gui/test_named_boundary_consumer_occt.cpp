#include "core/ProjectModel.h"
#include "core/SelectionTypes.h"
#include "services/AnalysisService.h"
#include "services/GeometryService.h"
#include "services/MaterialService.h"
#include "services/MeshService.h"
#include "services/NamedSelectionService.h"

#include <BRepPrimAPI_MakeBox.hxx>
#include <IFSelect_ReturnStatus.hxx>
#include <STEPControl_StepModelType.hxx>
#include <STEPControl_Writer.hxx>

#include <QCoreApplication>
#include <QVector>

#include <cmath>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

bool writeBoxStep(const std::filesystem::path &path)
{
    STEPControl_Writer writer;
    // OCCT model birimi mm. GeometryService -> MeshService sınırında SI dönüşümü
    // yapılır; test boyutu 100 x 20 x 20 mm'dir.
    const auto shape = BRepPrimAPI_MakeBox(100.0, 20.0, 20.0).Shape();
    return writer.Transfer(shape, STEPControl_AsIs) == IFSelect_RetDone
        && writer.Write(path.string().c_str()) == IFSelect_RetDone;
}

d26::SelectionItem faceItem(const d26::GeometryService &geometry,
                            const femcae::geometry::GeometryEntityId faceId)
{
    d26::SelectionItem item;
    const auto *entity = geometry.document().find(faceId);
    item.domain = d26::SelectionDomain::Geometry;
    item.kind = d26::SelectionKind::Face;
    item.geometryEntityId = faceId;
    item.parentGeometryId = entity != nullptr ? entity->parentId : femcae::geometry::InvalidGeometryId;
    item.sourceRevision = geometry.summary().revision;
    return item;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app)

    using namespace d26;

    ProjectModel project;
    GeometryService geometry;
    MeshService mesh(&geometry);
    MaterialService materials(&project);
    AnalysisService analysis(&project, &mesh, &materials);
    NamedSelectionService namedSelections(&project, &geometry, &mesh);
    analysis.setNamedSelectionService(&namedSelections);

    materials.resetToDefault();
    const ObjectId analysisId = analysis.createAnalysis(AnalysisType::StaticStructural);
    check(analysisId != InvalidObjectId,
          "Static Structural analysis fixture creates persistent boundary consumers");

    const auto path = std::filesystem::temp_directory_path() / "dynamics26_named_boundary_box.step";
    check(writeBoxStep(path), "OCCT writes deterministic STEP box fixture");
    check(geometry.importStep(QString::fromStdString(path.string())),
          "GeometryService imports real STEP B-Rep fixture");
    std::filesystem::remove(path);

    const auto bodies = geometry.bodies();
    check(bodies.size() == 1, "STEP fixture exposes exactly one CAD Body");
    if (bodies.size() != 1) {
        return 1;
    }
    const auto descriptor = geometry.boxDescriptor(bodies.front());
    check(descriptor.has_value(), "GeometryService recovers axis-aligned box face provenance");
    if (!descriptor.has_value()) {
        return 1;
    }

    mesh.setDivisions(2, 2, 1);
    check(mesh.generate(), "MeshService generates HEX8 mesh from real CAD face provenance");
    check(mesh.isUpToDate(), "generated CAD-driven mesh is current");

    const SelectionItem supportFace = faceItem(geometry, descriptor->xMinFace);
    const SelectionItem loadFaceA = faceItem(geometry, descriptor->xMaxFace);
    const SelectionItem loadFaceB = faceItem(geometry, descriptor->yMaxFace);
    check(supportFace.isValid() && loadFaceA.isValid() && loadFaceB.isValid(),
          "real CAD Face SelectionItem identities include parent Body and current revision");

    const NamedSelectionCreateResult supportNamed = namedSelections.createFromSelection(
        QVector<SelectionItem>{supportFace}, QStringLiteral("Fixed End"));
    const NamedSelectionCreateResult loadNamed = namedSelections.createFromSelection(
        QVector<SelectionItem>{loadFaceA, loadFaceB}, QStringLiteral("Loaded Faces"));
    check(supportNamed.success() && loadNamed.success(),
          "real CAD Face scopes create persistent Named Selection objects");
    check(namedSelections.validate(supportNamed.id) == ScopeReferenceValidationError::None
              && namedSelections.validate(loadNamed.id) == ScopeReferenceValidationError::None,
          "real CAD Face Named Selections validate against current B-Rep identity");

    const AnalysisRecord *record = analysis.analysis(analysisId);
    check(record != nullptr && !record->supports.isEmpty() && !record->loads.isEmpty(),
          "analysis exposes default Fixed Support and Force consumers");
    if (record == nullptr || record->supports.isEmpty() || record->loads.isEmpty()
        || !supportNamed.success() || !loadNamed.success()) {
        return 1;
    }

    const ObjectId supportId = record->supports.front();
    const ObjectId loadId = record->loads.front();

    SupportDefinition support = *analysis.support(supportId);
    support.scopingMethod = BoundaryScopingMethod::NamedSelection;
    support.namedSelectionId = supportNamed.id;
    analysis.updateSupport(supportId, support);

    LoadDefinition load = *analysis.load(loadId);
    load.scopingMethod = BoundaryScopingMethod::NamedSelection;
    load.namedSelectionId = loadNamed.id;
    load.fxN = 1200.0;
    load.fyN = 0.0;
    load.fzN = 0.0;
    analysis.updateLoad(loadId, load);

    const BoundaryScopeResolution supportResolution = analysis.resolveBoundaryScope(support);
    const BoundaryScopeResolution loadResolution = analysis.resolveBoundaryScope(load);
    check(supportResolution.valid && supportResolution.geometryFaceIds.size() == 1,
          "Fixed Support resolves referenced Named Selection to one real CAD Face");
    check(loadResolution.valid && loadResolution.geometryFaceIds.size() == 2,
          "Force resolves referenced Named Selection to two real CAD Faces");

    const int supportNodes = analysis.resolvedBoundaryNodeCount(support);
    const int loadUnionNodes = analysis.resolvedBoundaryNodeCount(load);
    const int separateLoadNodes = mesh.nodeCountFor(BoxFace::XMax) + mesh.nodeCountFor(BoxFace::YMax);
    check(supportNodes > 0 && loadUnionNodes > 0,
          "persistent CAD Face scopes resolve to current FEM Node identities");
    check(loadUnionNodes < separateLoadNodes,
          "multi-face Force deduplicates shared edge/corner nodes into one union scope");

    const PreflightReport report = analysis.preflight(analysisId);
    check(report.passed(),
          "valid CAD Face Named Selection Fixed Support / Force pass solver preflight");
    check(analysis.solve(analysisId),
          "Static Structural solver consumes valid Named Selection boundary scopes");

    const AnalysisRecord *solved = analysis.analysis(analysisId);
    check(solved != nullptr && solved->solved && solved->solveResults.valid,
          "Named Selection solve produces a valid result database");
    if (solved != nullptr && solved->solved) {
        // One Force object = one total vector. Two selected faces must not create
        // 2 x 1200 N. Equilibrium reaction magnitude remains 1200 N.
        const double reactionMagnitudeX = std::abs(solved->solveResults.reactionXN);
        check(std::abs(reactionMagnitudeX - 1200.0) < 1.0e-5,
              "multi-face Named Selection applies total 1200 N Force exactly once");
    }

    const NamedSelectionDefinition *loadDefinition = namedSelections.byId(loadNamed.id);
    check(loadDefinition != nullptr, "load Named Selection remains addressable by persistent ObjectId");
    if (loadDefinition != nullptr) {
        const ScopeReference originalScope = loadDefinition->scope;
        const auto singleFaceBuild = buildGeometryScopeReference(
            QVector<SelectionItem>{loadFaceA}, geometry.document());
        check(singleFaceBuild.success(), "alternate real CAD Face scope builds for staleness regression");
        if (singleFaceBuild.success()) {
            namedSelections.replaceScope(loadNamed.id, singleFaceBuild.scope);
            check(analysis.solutionIsOutOfDate(analysisId),
                  "changing referenced Named Selection scope makes solved analysis OutOfDate");
            namedSelections.replaceScope(loadNamed.id, originalScope);
            check(!analysis.solutionIsOutOfDate(analysisId),
                  "restoring identical persistent scope restores solved input signature validity");
        }
    }

    std::cout << (failures == 0 ? "Named boundary OCCT consumer PASS\n"
                                : "Named boundary OCCT consumer FAIL\n");
    return failures == 0 ? 0 : 1;
}
