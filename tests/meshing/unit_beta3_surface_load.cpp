#include <femcae/meshing/StructuredHexMesher.h>
#include <femcae/meshing/SurfaceLoadAssembler.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <unordered_set>

namespace {

using femcae::geometry::GeometryEntityId;
using femcae::geometry::Vec3;
using namespace femcae::meshing;

int failures = 0;

void check(const bool condition, const char *message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

[[nodiscard]] bool near(const double a, const double b, const double scale = 1.0)
{
    return std::abs(a - b) <= 2.0e-12 * std::max({1.0, std::abs(scale), std::abs(a), std::abs(b)});
}

[[nodiscard]] bool near(const Vec3 &a, const Vec3 &b, const double scale = 1.0)
{
    return near(a.x, b.x, scale) && near(a.y, b.y, scale) && near(a.z, b.z, scale);
}

[[nodiscard]] Vec3 cross(const Vec3 &a, const Vec3 &b)
{
    return {a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x};
}

[[nodiscard]] Vec3 nodalMomentAbout(const SimulationMesh &mesh,
                                    const SurfaceLoadAssemblyResult &assembly,
                                    const Vec3 &point)
{
    Vec3 moment;
    for (const NodalVectorLoad &load : assembly.nodalLoads) {
        const MeshNode *node = mesh.findNode(load.nodeId);
        if (node == nullptr) {
            return {NAN, NAN, NAN};
        }
        const Vec3 arm{node->x.x - point.x, node->x.y - point.y, node->x.z - point.z};
        const Vec3 contribution = cross(arm, load.value);
        moment.x += contribution.x;
        moment.y += contribution.y;
        moment.z += contribution.z;
    }
    return moment;
}

[[nodiscard]] SimulationMesh boxMesh(const std::size_t nx,
                                     const std::size_t ny,
                                     const std::size_t nz)
{
    StructuredHexMesherOptions options;
    options.nx = nx;
    options.ny = ny;
    options.nz = nz;
    return StructuredHexMesher{}.meshBox(
        {{0.0, 0.0, 0.0}, {2.0, 3.0, 4.0}},
        BoxBoundaryGeometry{100, 101, 102, 103, 104, 105, 106}, 17, options);
}

void verifyDirectionsAndRefinement()
{
    constexpr GeometryEntityId xMax = 102;
    const std::array<Vec3, 3> forces{{
        {135.0, 0.0, 0.0},
        {0.0, -246.0, 0.0},
        {0.0, 0.0, 357.0},
    }};
    const Vec3 referencePoint{0.25, -0.5, 0.75};
    const Vec3 faceCentroid{2.0, 1.5, 2.0};

    for (const std::size_t subdivision : {std::size_t{1}, std::size_t{2}, std::size_t{4}}) {
        const SimulationMesh mesh = boxMesh(subdivision, subdivision, subdivision);
        for (std::size_t direction = 0; direction < forces.size(); ++direction) {
            const SurfaceLoadAssemblyResult assembled = assembleUniformTotalForce(
                mesh, {xMax}, forces[direction]);
            check(assembled.success(), "XMax QUAD4 surface integration succeeds");
            check(near(assembled.referenceArea, 12.0, 12.0),
                  "2x2 quadrature recovers exact reference surface area");
            check(near(assembled.resultant(), forces[direction], 400.0),
                  "assembled nodal resultant conserves Total Force in X/Y/Z");

            const Vec3 arm{faceCentroid.x - referencePoint.x,
                           faceCentroid.y - referencePoint.y,
                           faceCentroid.z - referencePoint.z};
            check(near(nodalMomentAbout(mesh, assembled, referencePoint),
                       cross(arm, forces[direction]), 2000.0),
                  "consistent nodal loads conserve traction moment about reference point");
        }
    }
}

void verifyMultiFaceSingleResultant()
{
    const SimulationMesh mesh = boxMesh(4, 3, 2);
    const Vec3 total{120.0, -35.0, 80.0};
    const SurfaceLoadAssemblyResult assembled = assembleUniformTotalForce(mesh, {102, 104}, total);
    check(assembled.success(), "two selected CAD Faces assemble as one surface scope");
    check(near(assembled.referenceArea, 20.0, 20.0),
          "multi-Face reference area is the sum of all selected boundary facets");
    check(near(assembled.resultant(), total, 150.0),
          "multi-Face Total Force is not repeated once per Face");

    // A_x=12, c_x=(2,1.5,2); A_y=8, c_y=(1,3,2).
    const Vec3 combinedCentroid{1.6, 2.1, 2.0};
    const Vec3 referencePoint{-0.4, 0.2, 0.7};
    const Vec3 arm{combinedCentroid.x - referencePoint.x,
                   combinedCentroid.y - referencePoint.y,
                   combinedCentroid.z - referencePoint.z};
    check(near(nodalMomentAbout(mesh, assembled, referencePoint), cross(arm, total), 1000.0),
          "multi-Face consistent load conserves the area-weighted resultant moment");
}

void verifyNonuniformAndWarpedFacets()
{
    SimulationMesh nonuniform;
    nonuniform.nodes = {
        {1, {0.0, 0.0, 0.0}, 42}, {2, {1.0, 0.0, 0.0}, 42},
        {3, {1.0, 1.0, 0.0}, 42}, {4, {0.0, 1.0, 0.0}, 42},
        {5, {3.0, 0.0, 0.0}, 42}, {6, {5.0, 0.0, 0.0}, 42},
        {7, {5.0, 1.0, 0.0}, 42}, {8, {3.0, 1.0, 0.0}, 42},
    };
    nonuniform.boundaryFacets = {
        {11, {1, 2, 3, 4}, 101, 42},
        {12, {5, 6, 7, 8}, 102, 42},
    };
    const SurfaceLoadAssemblyResult split = assembleUniformTotalForce(
        nonuniform, {42}, {90.0, 0.0, 0.0});
    check(split.success() && near(split.referenceArea, 3.0, 3.0),
          "nonuniform facet fixture integrates unequal reference areas");
    double firstFacetForce = 0.0;
    double secondFacetForce = 0.0;
    for (const NodalVectorLoad &load : split.nodalLoads) {
        (load.nodeId <= 4 ? firstFacetForce : secondFacetForce) += load.value.x;
    }
    check(near(firstFacetForce, 30.0, 90.0) && near(secondFacetForce, 60.0, 90.0),
          "uniform traction distributes force in proportion to nonuniform facet area");

    SimulationMesh warped;
    warped.nodes = {
        {21, {0.0, 0.0, 0.0}, 77}, {22, {2.0, 0.0, 0.0}, 77},
        {23, {2.0, 1.0, 0.5}, 77}, {24, {0.0, 1.0, 0.0}, 77},
    };
    warped.boundaryFacets = {{31, {21, 22, 23, 24}, 201, 77}};
    const SurfaceLoadAssemblyResult curved = assembleUniformTotalForce(
        warped, {77}, {12.0, -8.0, 5.0});
    const Vec3 referencePoint{0.3, -0.2, 0.4};
    const Vec3 firstMomentAboutPoint{
        curved.referenceFirstMoment.x - referencePoint.x * curved.referenceArea,
        curved.referenceFirstMoment.y - referencePoint.y * curved.referenceArea,
        curved.referenceFirstMoment.z - referencePoint.z * curved.referenceArea,
    };
    check(curved.success(), "general bilinear/warped QUAD4 test infrastructure is supported");
    check(near(nodalMomentAbout(warped, curved, referencePoint),
               cross(firstMomentAboutPoint, curved.uniformReferenceTraction), 100.0),
          "warped QUAD4 consistent loads conserve quadrature traction moment");

    SimulationMesh invalid = warped;
    invalid.boundaryFacets.front().nodeIds[3] = 9999;
    check(assembleUniformTotalForce(invalid, {77}, {1.0, 0.0, 0.0}).error
              == SurfaceLoadAssemblyError::MissingNode,
          "missing boundary-facet node fails explicitly without nodal fallback");
}

} // namespace

int main()
{
    verifyDirectionsAndRefinement();
    verifyMultiFaceSingleResultant();
    verifyNonuniformAndWarpedFacets();
    std::cout << (failures == 0 ? "Beta.3 surface Total Force integration PASS"
                                : "Beta.3 surface Total Force integration FAIL")
              << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
