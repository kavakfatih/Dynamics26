#include "femcae/meshing/SurfaceLoadAssembler.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace femcae::meshing {
namespace {

using geometry::Vec3;

constexpr std::array<std::array<double, 2>, 4> kQuad4NaturalCoordinates{{
    {-1.0, -1.0},
    { 1.0, -1.0},
    { 1.0,  1.0},
    {-1.0,  1.0},
}};

struct SurfaceQuadratureSample {
    std::array<MeshEntityId, 4> nodeIds{};
    std::array<double, 4> shape{};
    Vec3 position;
    double differentialArea{0.0};
};

[[nodiscard]] bool finite(const Vec3 &value) noexcept
{
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Vec3 cross(const Vec3 &a, const Vec3 &b) noexcept
{
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

[[nodiscard]] double norm(const Vec3 &value) noexcept
{
    return std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z);
}

void addScaled(Vec3 &target, const Vec3 &value, const double scale) noexcept
{
    target.x += value.x * scale;
    target.y += value.y * scale;
    target.z += value.z * scale;
}

[[nodiscard]] SurfaceLoadAssemblyResult failure(const SurfaceLoadAssemblyError error)
{
    SurfaceLoadAssemblyResult result;
    result.error = error;
    return result;
}

} // namespace

Vec3 SurfaceLoadAssemblyResult::resultant() const noexcept
{
    Vec3 value;
    for (const NodalVectorLoad &load : nodalLoads) {
        value.x += load.value.x;
        value.y += load.value.y;
        value.z += load.value.z;
    }
    return value;
}

SurfaceLoadAssemblyResult assembleUniformTotalForce(
    const SimulationMesh &mesh,
    const std::vector<geometry::GeometryEntityId> &geometryFaceIds,
    const Vec3 &totalForce)
{
    if (geometryFaceIds.empty()) {
        return failure(SurfaceLoadAssemblyError::EmptyScope);
    }
    if (!finite(totalForce)) {
        return failure(SurfaceLoadAssemblyError::NonFiniteInput);
    }

    std::unordered_set<geometry::GeometryEntityId> selectedFaces;
    selectedFaces.reserve(geometryFaceIds.size());
    for (const geometry::GeometryEntityId id : geometryFaceIds) {
        if (id == geometry::InvalidGeometryId) {
            return failure(SurfaceLoadAssemblyError::InvalidScope);
        }
        selectedFaces.insert(id);
    }

    std::unordered_map<MeshEntityId, const MeshNode *> nodes;
    nodes.reserve(mesh.nodes.size());
    for (const MeshNode &node : mesh.nodes) {
        if (node.id == InvalidMeshId || !finite(node.x)
            || !nodes.emplace(node.id, &node).second) {
            return failure(SurfaceLoadAssemblyError::NonFiniteInput);
        }
    }

    std::vector<SurfaceQuadratureSample> samples;
    samples.reserve(4 * mesh.boundaryFacets.size());
    std::unordered_set<MeshEntityId> selectedFacetIds;
    bool foundFacet = false;

    constexpr double gauss = 0.57735026918962576450914878050195746; // 1/sqrt(3)
    constexpr std::array<double, 2> points{-gauss, gauss};

    for (const MeshFacet &facet : mesh.boundaryFacets) {
        if (!selectedFaces.contains(facet.sourceGeometryId)) {
            continue;
        }
        foundFacet = true;
        if (facet.id == InvalidMeshId || !selectedFacetIds.insert(facet.id).second) {
            return failure(SurfaceLoadAssemblyError::InvalidFacet);
        }
        std::unordered_set<MeshEntityId> facetNodeIds;
        std::array<Vec3, 4> coordinates{};
        for (std::size_t a = 0; a < facet.nodeIds.size(); ++a) {
            const MeshEntityId nodeId = facet.nodeIds[a];
            const auto node = nodes.find(nodeId);
            if (nodeId == InvalidMeshId || node == nodes.end()) {
                return failure(SurfaceLoadAssemblyError::MissingNode);
            }
            if (!facetNodeIds.insert(nodeId).second) {
                return failure(SurfaceLoadAssemblyError::InvalidFacet);
            }
            coordinates[a] = node->second->x;
        }

        for (const double eta : points) {
            for (const double xi : points) {
                SurfaceQuadratureSample sample;
                sample.nodeIds = facet.nodeIds;
                Vec3 tangentXi;
                Vec3 tangentEta;

                for (std::size_t a = 0; a < kQuad4NaturalCoordinates.size(); ++a) {
                    const double xiA = kQuad4NaturalCoordinates[a][0];
                    const double etaA = kQuad4NaturalCoordinates[a][1];
                    const double shape = 0.25 * (1.0 + xiA * xi) * (1.0 + etaA * eta);
                    const double derivativeXi = 0.25 * xiA * (1.0 + etaA * eta);
                    const double derivativeEta = 0.25 * etaA * (1.0 + xiA * xi);
                    sample.shape[a] = shape;
                    addScaled(sample.position, coordinates[a], shape);
                    addScaled(tangentXi, coordinates[a], derivativeXi);
                    addScaled(tangentEta, coordinates[a], derivativeEta);
                }

                sample.differentialArea = norm(cross(tangentXi, tangentEta));
                if (!(sample.differentialArea > 0.0)
                    || !std::isfinite(sample.differentialArea)
                    || !finite(sample.position)) {
                    return failure(SurfaceLoadAssemblyError::DegenerateFacet);
                }
                samples.push_back(sample);
            }
        }
    }

    if (!foundFacet || samples.empty()) {
        return failure(SurfaceLoadAssemblyError::NoMatchingFacets);
    }

    SurfaceLoadAssemblyResult result;
    for (const SurfaceQuadratureSample &sample : samples) {
        result.referenceArea += sample.differentialArea;
        addScaled(result.referenceFirstMoment, sample.position, sample.differentialArea);
    }
    if (!(result.referenceArea > 0.0) || !std::isfinite(result.referenceArea)
        || !finite(result.referenceFirstMoment)) {
        return failure(SurfaceLoadAssemblyError::DegenerateFacet);
    }

    result.uniformReferenceTraction = {
        totalForce.x / result.referenceArea,
        totalForce.y / result.referenceArea,
        totalForce.z / result.referenceArea,
    };
    if (!finite(result.uniformReferenceTraction)) {
        return failure(SurfaceLoadAssemblyError::NonFiniteInput);
    }

    std::unordered_map<MeshEntityId, Vec3> accumulated;
    for (const SurfaceQuadratureSample &sample : samples) {
        for (std::size_t a = 0; a < sample.nodeIds.size(); ++a) {
            addScaled(accumulated[sample.nodeIds[a]], result.uniformReferenceTraction,
                      sample.shape[a] * sample.differentialArea);
        }
    }

    result.nodalLoads.reserve(accumulated.size());
    for (const auto &[nodeId, value] : accumulated) {
        result.nodalLoads.push_back(NodalVectorLoad{nodeId, value});
    }
    std::sort(result.nodalLoads.begin(), result.nodalLoads.end(),
              [](const NodalVectorLoad &a, const NodalVectorLoad &b) {
                  return a.nodeId < b.nodeId;
              });
    return result;
}

const char *surfaceLoadAssemblyErrorMessage(const SurfaceLoadAssemblyError error) noexcept
{
    switch (error) {
    case SurfaceLoadAssemblyError::None: return "surface load assembly succeeded";
    case SurfaceLoadAssemblyError::EmptyScope: return "surface scope is empty";
    case SurfaceLoadAssemblyError::InvalidScope: return "surface scope contains an invalid geometry identity";
    case SurfaceLoadAssemblyError::NoMatchingFacets: return "surface scope has no matching FEM boundary facets";
    case SurfaceLoadAssemblyError::InvalidFacet: return "surface scope contains an invalid or duplicate QUAD4 facet";
    case SurfaceLoadAssemblyError::MissingNode: return "surface facet references a missing FEM node";
    case SurfaceLoadAssemblyError::DegenerateFacet: return "surface facet has a degenerate reference Jacobian";
    case SurfaceLoadAssemblyError::NonFiniteInput: return "surface load input contains a non-finite value";
    }
    return "unknown surface load assembly error";
}

} // namespace femcae::meshing
