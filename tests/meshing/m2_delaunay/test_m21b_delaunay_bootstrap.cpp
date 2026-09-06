#include "meshing/m2/DelaunayTopology.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using femcae::geometry::Vec3;
using femcae::meshing::AffineDimension;
using femcae::meshing::CanonicalSite;
using femcae::meshing::PointId;
using femcae::meshing::m2::DelaunayBootstrapStatus;
using femcae::meshing::m2::DelaunayCellRecord;
using femcae::meshing::m2::DelaunayCellSlot;
using femcae::meshing::m2::DelaunayVertexRef;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

CanonicalSite site(PointId id, double x, double y, double z) {
    CanonicalSite result;
    result.id = id;
    result.point = Vec3{x, y, z};
    result.sourceRecordIds = {id};
    return result;
}

std::vector<CanonicalSite> basisFixture() {
    return {
        site(10, 0.0, 0.0, 0.0),
        site(20, 1.0, 0.0, 0.0),
        site(30, 2.0, 0.0, 0.0),
        site(40, 0.0, 1.0, 0.0),
        site(50, 1.0, 1.0, 0.0),
        site(60, 0.0, 0.0, 1.0),
    };
}

using CellKey = std::array<DelaunayVertexRef, 4>;

bool cellKeyLess(const CellKey& lhs, const CellKey& rhs) {
    for (std::size_t i = 0U; i < 4U; ++i) {
        if (lhs[i] < rhs[i]) return true;
        if (rhs[i] < lhs[i]) return false;
    }
    return false;
}

std::vector<CellKey> canonicalCellKeys(
    std::span<const DelaunayCellSlot> slots) {
    std::vector<CellKey> keys;
    for (const DelaunayCellSlot& slot : slots) {
        if (!slot.live) continue;
        CellKey key = slot.record.vertices;
        std::sort(key.begin(), key.end());
        keys.push_back(std::move(key));
    }
    std::sort(keys.begin(), keys.end(), cellKeyLess);
    return keys;
}

void verifyTypedVertexDomain() {
    const DelaunayVertexRef infinity = DelaunayVertexRef::infinite();
    require(infinity.isInfinite(), "Infinite vertex lost its topological tag");
    require(!infinity.finitePointId().has_value(),
            "Infinite vertex exposed a fake PointId");

    const DelaunayVertexRef finite = DelaunayVertexRef::finite(42);
    require(finite.isFinite(), "Finite vertex lost its PointId tag");
    require(finite.finitePointId() == std::optional<PointId>{42},
            "Finite vertex PointId mismatch");

    bool rejected = false;
    try {
        (void)DelaunayVertexRef::finite(femcae::meshing::InvalidPointId);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "InvalidPointId was accepted as a finite M2 vertex");
}

void verifyDeterministicBootstrap() {
    std::vector<CanonicalSite> sites = basisFixture();
    const auto bootstrap =
        femcae::meshing::m2::buildDelaunayBootstrap(sites);

    require(bootstrap.status == DelaunayBootstrapStatus::Ready,
            "3D canonical sites did not produce a bootstrap");
    require(bootstrap.affineDimension == AffineDimension::Three,
            "3D bootstrap affine dimension mismatch");
    require(
        bootstrap.basisPointIds == std::array<PointId, 4>{10, 20, 40, 60},
        "deterministic affine basis did not choose first valid PointIds");
    require(bootstrap.arena.liveCount() == 5U,
            "bootstrap must contain one finite plus four ghost cells");

    const auto report = femcae::meshing::m2::validateDelaunayTopology(
        bootstrap.arena.slots(), sites);
    require(report.ok(), "bootstrap topology validator rejected valid complex");

    const auto stats = femcae::meshing::m2::computeDelaunayComplexStats(
        bootstrap.arena.slots());
    require(stats.vertices == 5U, "bootstrap S3 vertex count mismatch");
    require(stats.edges == 10U, "bootstrap S3 edge count mismatch");
    require(stats.faces == 10U, "bootstrap S3 face count mismatch");
    require(stats.cells == 5U, "bootstrap S3 cell count mismatch");
    require(stats.finiteCells == 1U, "bootstrap finite-cell count mismatch");
    require(stats.ghostCells == 4U, "bootstrap ghost-cell count mismatch");
    require(stats.eulerCharacteristic == 0,
            "bootstrap S3 Euler characteristic must be zero");

    std::size_t finiteCount = 0U;
    std::size_t ghostCount = 0U;
    for (const DelaunayCellSlot& slot : bootstrap.arena.slots()) {
        if (!slot.live) continue;

        std::size_t finiteVertices = 0U;
        for (const auto& vertex : slot.record.vertices) {
            finiteVertices += vertex.isFinite() ? 1U : 0U;
        }

        if (finiteVertices == 4U) {
            ++finiteCount;
            std::array<Vec3, 4> points{};
            for (std::size_t i = 0U; i < 4U; ++i) {
                const PointId id = *slot.record.vertices[i].finitePointId();
                const auto found = std::find_if(
                    sites.begin(), sites.end(),
                    [id](const CanonicalSite& candidate) {
                        return candidate.id == id;
                    });
                require(found != sites.end(), "finite bootstrap PointId lookup failed");
                points[i] = found->point;
            }
            require(
                femcae::meshing::predicates::orient3d(
                    points[0], points[1], points[2], points[3]).sign ==
                    femcae::meshing::predicates::PredicateSign::Positive,
                "stored bootstrap finite tetra is not exact-positive");
        } else {
            ++ghostCount;
            require(slot.record.vertices[0].isInfinite(),
                    "ghost Infinite vertex is not fixed at local slot zero");
            require(slot.record.neighbors[0].isValid(),
                    "ghost finite neighbor is missing");
            for (std::size_t localFace = 1U; localFace < 4U; ++localFace) {
                require(slot.record.neighbors[localFace].isValid(),
                        "ghost lateral neighbor is missing");
            }
        }
    }
    require(finiteCount == 1U && ghostCount == 4U,
            "bootstrap finite/ghost classification mismatch");

    std::reverse(sites.begin(), sites.end());
    const auto reversed =
        femcae::meshing::m2::buildDelaunayBootstrap(sites);
    require(reversed.basisPointIds == bootstrap.basisPointIds,
            "basis depends on input enumeration order");
    require(
        canonicalCellKeys(reversed.arena.slots()) ==
            canonicalCellKeys(bootstrap.arena.slots()),
        "bootstrap topology depends on input enumeration order");
}

void verifyLowerDimensionalOutcomes() {
    const std::vector<std::vector<CanonicalSite>> fixtures{
        {},
        {site(1, 0.0, 0.0, 0.0)},
        {site(1, 0.0, 0.0, 0.0), site(2, 1.0, 0.0, 0.0),
         site(3, 2.0, 0.0, 0.0)},
        {site(1, 0.0, 0.0, 0.0), site(2, 1.0, 0.0, 0.0),
         site(3, 0.0, 1.0, 0.0), site(4, 1.0, 1.0, 0.0)},
    };
    const std::array<AffineDimension, 4> expected{
        AffineDimension::Empty,
        AffineDimension::Zero,
        AffineDimension::One,
        AffineDimension::Two};

    for (std::size_t i = 0U; i < fixtures.size(); ++i) {
        const auto result =
            femcae::meshing::m2::buildDelaunayBootstrap(fixtures[i]);
        require(result.status == DelaunayBootstrapStatus::LowerDimensional,
                "lower-dimensional input created tetra cells");
        require(result.affineDimension == expected[i],
                "lower-dimensional classification mismatch");
        require(result.arena.liveCount() == 0U,
                "lower-dimensional bootstrap mutated reference arena");
    }
}

void verifyInputValidation() {
    bool rejected = false;
    try {
        const std::vector<CanonicalSite> duplicateId{
            site(1, 0.0, 0.0, 0.0),
            site(1, 1.0, 0.0, 0.0)};
        (void)femcae::meshing::m2::buildDelaunayBootstrap(duplicateId);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "duplicate PointId entered M2 bootstrap");

    rejected = false;
    try {
        const std::vector<CanonicalSite> duplicateCoordinate{
            site(1, 0.0, 0.0, 0.0),
            site(2, 0.0, 0.0, 0.0)};
        (void)femcae::meshing::m2::buildDelaunayBootstrap(duplicateCoordinate);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, "non-canonical duplicate coordinate entered M2 bootstrap");
}

void verifyValidatorNegativeControls() {
    const std::vector<CanonicalSite> sites = basisFixture();
    const auto bootstrap =
        femcae::meshing::m2::buildDelaunayBootstrap(sites);

    std::vector<DelaunayCellSlot> stale(
        bootstrap.arena.slots().begin(),
        bootstrap.arena.slots().end());
    stale[0].record.neighbors[0].generation += 1U;
    require(
        !femcae::meshing::m2::validateDelaunayTopology(stale, sites).ok(),
        "validator accepted stale neighbor generation");

    std::vector<DelaunayCellSlot> wrongOrientation(
        bootstrap.arena.slots().begin(),
        bootstrap.arena.slots().end());
    std::swap(
        wrongOrientation[1].record.vertices[1],
        wrongOrientation[1].record.vertices[2]);
    require(
        !femcae::meshing::m2::validateDelaunayTopology(
            wrongOrientation, sites).ok(),
        "validator accepted inward ghost hull orientation");

    std::vector<DelaunayCellSlot> wrongInfinite(
        bootstrap.arena.slots().begin(),
        bootstrap.arena.slots().end());
    wrongInfinite[1].record.vertices[0] = DelaunayVertexRef::finite(30);
    require(
        !femcae::meshing::m2::validateDelaunayTopology(
            wrongInfinite, sites).ok(),
        "validator accepted ghost without typed Infinite slot zero");
}

} // namespace

int main() {
    try {
        verifyTypedVertexDomain();
        verifyDeterministicBootstrap();
        verifyLowerDimensionalOutcomes();
        verifyInputValidation();
        verifyValidatorNegativeControls();

        std::cout
            << "M2.1-B typed bootstrap PASS"
            << " cells=5 finite=1 ghost=4"
            << " V=5 E=10 F=10 C=5 euler=0"
            << " deterministic=yes corruption=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-B typed bootstrap FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
