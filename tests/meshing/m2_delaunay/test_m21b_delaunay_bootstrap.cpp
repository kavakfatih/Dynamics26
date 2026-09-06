#include "meshing/m2/DelaunayTopology.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <limits>
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

using Issue = femcae::meshing::m2::DelaunayTopologyIssueCode;
using Handle = femcae::meshing::m2::DelaunayCellHandle;

bool hasIssue(const femcae::meshing::m2::DelaunayTopologyValidationReport& report,
              Issue code) {
    return std::any_of(report.issues.begin(), report.issues.end(),
                       [code](const auto& issue) { return issue.code == code; });
}

void verifyValidatorNegativeControls() {
    const auto sites = basisFixture();
    const auto bootstrap = femcae::meshing::m2::buildDelaunayBootstrap(sites);
    const auto copy = [&] {
        return std::vector<DelaunayCellSlot>(bootstrap.arena.slots().begin(),
                                           bootstrap.arena.slots().end());
    };
    const auto expect = [&](const auto& slots, Issue code) {
        require(hasIssue(femcae::meshing::m2::validateDelaunayTopology(slots, sites), code),
                "validator omitted expected issue " + std::to_string(static_cast<int>(code)));
    };
    auto stale = copy();
    stale[0].record.neighbors[0].generation += 1U;
    expect(stale, Issue::StaleNeighborGeneration);

    auto orientation = copy();
    std::swap(orientation[1].record.vertices[1], orientation[1].record.vertices[2]);
    std::swap(orientation[1].record.neighbors[1], orientation[1].record.neighbors[2]);
    const auto report = femcae::meshing::m2::validateDelaunayTopology(orientation, sites);
    require(report.issues.size() == 1 && hasIssue(report, Issue::GhostHullOrientationInvalid),
            "orientation test must fail ONLY for inward ghost orientation");

    // Iki gercek ghost-face owner'in her birini kendisine bagla.
    // Eski validator reciprocal self-links + Euler=0 kompleksini kabul ediyordu.
    auto self = copy();
    const auto other = self[1].record.neighbors[1];
    const auto key = femcae::meshing::m2::canonicalDelaunayFaceKey(self[1].record, 1);
    for (std::size_t f = 0; f < 4; ++f) {
        if (femcae::meshing::m2::canonicalDelaunayFaceKey(self[other.slot].record, f) == key)
            self[other.slot].record.neighbors[f] = other;
    }
    self[1].record.neighbors[1] = Handle{1, self[1].generation};
    expect(self, Issue::FaceAdjacencyMismatch);

    std::vector<DelaunayCellSlot> duplicate{copy()[0], copy()[0]};
    for (std::size_t i = 0; i < 2; ++i)
        for (auto& n : duplicate[i].record.neighbors)
            n = Handle{static_cast<std::uint32_t>(1-i), 1};
    require(femcae::meshing::m2::computeDelaunayComplexStats(duplicate).eulerCharacteristic == 0,
            "duplicate-cell counterexample must have Euler zero");
    expect(duplicate, Issue::DuplicateCell);

    auto missing = copy();
    missing[0].record.vertices[0] = DelaunayVertexRef::finite(999);
    expect(missing, Issue::MissingFinitePoint); // must return a report, not throw

    for (unsigned mask = 2; mask < 16; ++mask) {
        auto pattern = copy();
        for (unsigned i = 0; i < 4; ++i)
            if (mask & (1U << i)) pattern[0].record.vertices[i] = DelaunayVertexRef::infinite();
        expect(pattern, Issue::InvalidCellVertexPattern);
    }
    auto duplicateVertex = copy();
    duplicateVertex[0].record.vertices[1] = duplicateVertex[0].record.vertices[0];
    expect(duplicateVertex, Issue::DuplicateFiniteVertex);
    auto dead = copy(); dead[1].live = false;
    expect(dead, Issue::NeighborDead);
    auto out = copy(); out[0].record.neighbors[0].slot = 999;
    expect(out, Issue::NeighborOutOfRange);
    auto zeroGeneration = copy(); zeroGeneration[0].generation = 0;
    expect(zeroGeneration, Issue::LiveSlotZeroGeneration);
    for (Handle h : {Handle{}, Handle{999, 1}, Handle{0, 2}}) {
        bool rejected = false;
        try { (void)bootstrap.arena.cell(h); }
        catch (const std::out_of_range&) { rejected = true; }
        require(rejected, "arena accepted stale/invalid handle");
    }
}

void verifyScaleAndIndependentIncidence() {
    for (int exponent : {-500, 0, 500}) {
        auto sites = basisFixture();
        for (auto& site : sites) {
            site.id = std::numeric_limits<PointId>::max() - site.id;
            site.point.x = std::ldexp(site.point.x, exponent);
            site.point.y = std::ldexp(site.point.y, exponent);
            site.point.z = std::ldexp(site.point.z, exponent);
        }
        const auto result = femcae::meshing::m2::buildDelaunayBootstrap(sites);
        const auto slots = result.arena.slots();
        // Validator sonucunu tekrar sormak yerine tum komsuluklari bagimsiz tara.
        for (std::size_t i = 0; i < slots.size(); ++i) {
            for (std::size_t f = 0; f < 4; ++f) {
                const auto n = slots[i].record.neighbors[f];
                require(n.slot != i && n.slot < slots.size(), "invalid neighbor owner");
                std::vector<DelaunayVertexRef> face;
                for (std::size_t v = 0; v < 4; ++v)
                    if (v != f) face.push_back(slots[i].record.vertices[v]);
                std::sort(face.begin(), face.end());
                std::size_t owners = 0;
                for (std::size_t j = 0; j < slots.size(); ++j) {
                    for (std::size_t g = 0; g < 4; ++g) {
                        std::vector<DelaunayVertexRef> other;
                        for (std::size_t v = 0; v < 4; ++v)
                            if (v != g) other.push_back(slots[j].record.vertices[v]);
                        std::sort(other.begin(), other.end());
                        if (face != other) continue;
                        ++owners;
                        if (j != i)
                            require(n == Handle{static_cast<std::uint32_t>(j), slots[j].generation} &&
                                    slots[j].record.neighbors[g] ==
                                        Handle{static_cast<std::uint32_t>(i), slots[i].generation},
                                    "actual face owners are not reciprocal");
                    }
                }
                require(owners == 2, "unified face must have exactly two owners");
            }
        }
    }
}

} // namespace

int main() {
    try {
        verifyTypedVertexDomain();
        verifyDeterministicBootstrap();
        verifyLowerDimensionalOutcomes();
        verifyInputValidation();
        verifyValidatorNegativeControls();
        verifyScaleAndIndependentIncidence();

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
