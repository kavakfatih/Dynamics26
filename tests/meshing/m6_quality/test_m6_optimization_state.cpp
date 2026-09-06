#include "meshing/m6/state/TetraOptimizationState.h"
#include "meshing/m6/quality/QualityVector.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <map>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

namespace s = femcae::meshing::m6::state;
namespace q = femcae::meshing::m6::quality;
using femcae::meshing::CanonicalFaceKey;
using femcae::meshing::CanonicalSite;
using femcae::meshing::InvalidPointId;
using femcae::meshing::InvalidTetHandle;
using femcae::meshing::PointId;
using femcae::meshing::TetSlot;

[[noreturn]] void fail(const char* message) {
    throw std::runtime_error(message);
}

void require(bool condition, const char* message) {
    if (!condition) {
        fail(message);
    }
}

// The linear scan the incidence index replaced, kept as the reference the index
// must reproduce exactly -- including handle order, which commitSmartSmoothing
// compares element-wise.
std::vector<femcae::meshing::TetHandle> scanIncident(
    const std::vector<TetSlot>& tetraSlots,
    PointId id) {
    std::vector<femcae::meshing::TetHandle> result;
    for (std::size_t slot = 0; slot < tetraSlots.size(); ++slot) {
        const TetSlot& tetra = tetraSlots[slot];
        if (!tetra.live) {
            continue;
        }
        if (std::find(
                tetra.record.vertices.begin(),
                tetra.record.vertices.end(),
                id) != tetra.record.vertices.end()) {
            result.push_back({
                static_cast<std::uint32_t>(slot),
                tetra.generation});
        }
    }
    return result;
}

std::vector<CanonicalSite> sites() {
    return {
        {10U, {0.0, 0.0, 0.0}, {}},
        {30U, {1.0, 0.0, 0.0}, {}},
        {50U, {0.0, 0.0, 1.0}, {}},
        {70U, {0.0, 1.0, 0.0}, {}},
    };
}

std::vector<TetSlot> slots() {
    TetSlot slot;
    slot.generation = 7U;
    slot.live = true;
    slot.record.vertices = {10U, 30U, 50U, 70U};
    slot.record.neighbors = {
        InvalidTetHandle,
        InvalidTetHandle,
        InvalidTetHandle,
        InvalidTetHandle};
    return {slot};
}

s::ConstraintView constraints() {
    CanonicalFaceKey face;
    face.vertices = {70U, 30U, 50U};

    return s::ConstraintView(
        {
            {10U, s::PointMobility::InteriorFree},
            {30U, s::PointMobility::Fixed},
            {50U, s::PointMobility::Fixed},
            {70U, s::PointMobility::Fixed},
        },
        {
            {{50U, 30U}},
        },
        {face});
}

template <class Callable>
void requireInvalid(Callable&& callable, const char* message) {
    bool rejected = false;
    try {
        callable();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, message);
}

} // namespace

int main() {
    try {
        const auto canonicalSites = sites();
        s::TetraOptimizationState state =
            s::TetraOptimizationState::fromCanonicalSites(
                canonicalSites,
                slots(),
                constraints());

        require(state.points().size() == 4U, "point copy count mismatch");
        require(state.hasPoint(10U) && state.hasPoint(70U), "PointId lookup failed");
        require(!state.hasPoint(1U), "state assumed PointId == index + 1");
        require(state.point(30U).x == 1.0, "point coordinate lookup mismatch");

        const auto live = state.liveTetrahedra();
        require(live.size() == 1U, "live tetra count mismatch");
        require(live[0].slot == 0U && live[0].generation == 7U,
                "live tetra handle mismatch");

        const auto incident = state.incidentTetrahedra(10U);
        require(incident == live, "incident-star lookup mismatch");

        const std::vector<PointId> neighbors =
            state.oneRingNeighbors(10U);
        require(
            neighbors == std::vector<PointId>({30U, 50U, 70U}),
            "one-ring neighbors are not canonical PointId order");

        require(
            state.constraints().isInteriorFree(10U),
            "explicit InteriorFree mobility lost");
        require(
            state.constraints().isFixed(30U),
            "explicit Fixed mobility lost");
        require(
            !state.constraints().mobility(999U).has_value(),
            "unknown mobility must remain unresolved/conservative");

        require(
            state.constraints().isProtected(
                s::canonicalProtectedEdgeKey(30U, 50U)),
            "protected edge canonicalization failed");

        CanonicalFaceKey protectedFace;
        protectedFace.vertices = {30U, 50U, 70U};
        require(
            state.constraints().isProtected(protectedFace),
            "protected face canonicalization failed");

        // A query key is canonicalized on the way in, exactly like a stored key.
        // An operation planner holds edges in traversal order and faces in
        // mesh-local winding order; if those orderings answered "not protected"
        // the constraint layer would fail open and let a protected CAD feature
        // be destroyed.
        require(
            state.constraints().isProtected(
                s::ProtectedEdgeKey{{50U, 30U}}),
            "reversed protected edge query must resolve to the same edge");

        CanonicalFaceKey windingFace;
        windingFace.vertices = {70U, 30U, 50U};
        require(
            state.constraints().isProtected(windingFace),
            "rotated protected face query must resolve to the same face");

        CanonicalFaceKey swappedFace;
        swappedFace.vertices = {50U, 70U, 30U};
        require(
            state.constraints().isProtected(swappedFace),
            "reflected protected face query must resolve to the same face");

        // Malformed keys never name a stored feature, so they must answer
        // "not protected" without throwing out of a noexcept query.
        require(
            !state.constraints().isProtected(
                s::ProtectedEdgeKey{{30U, 30U}}),
            "degenerate edge query must not report protection");
        require(
            !state.constraints().isProtected(
                s::ProtectedEdgeKey{{InvalidPointId, 30U}}),
            "invalid edge query must not report protection");

        CanonicalFaceKey degenerateFace;
        degenerateFace.vertices = {30U, 30U, 50U};
        require(
            !state.constraints().isProtected(degenerateFace),
            "degenerate face query must not report protection");

        require(
            !state.constraints().isProtected(
                s::canonicalProtectedEdgeKey(30U, 70U)),
            "unprotected edge must not report protection");

        const q::IndexedTetraCoordinates original =
            state.qualityCell(live[0]);
        require(
            q::isExactPositiveQualityCell(original),
            "authoritative state tetra is not exact-positive");

        const q::IndexedTetraCoordinates candidate =
            state.qualityCell(
                live[0],
                s::PointCoordinateOverride{
                    10U,
                    {0.05, 0.05, 0.05}});
        require(
            q::isExactPositiveQualityCell(candidate),
            "safe coordinate override became non-positive");
        require(
            state.point(10U).x == 0.0 &&
            state.point(10U).y == 0.0 &&
            state.point(10U).z == 0.0,
            "candidate override mutated authoritative point state");

        requireInvalid(
            [&]() {
                auto duplicateSites = sites();
                duplicateSites[1].id = 10U;
                (void)s::TetraOptimizationState::fromCanonicalSites(
                    duplicateSites,
                    slots(),
                    constraints());
            },
            "duplicate PointId state was accepted");

        requireInvalid(
            [&]() {
                auto brokenSlots = slots();
                brokenSlots[0].record.vertices[3] = 999U;
                (void)s::TetraOptimizationState::fromCanonicalSites(
                    canonicalSites,
                    brokenSlots,
                    constraints());
            },
            "tetra referencing unknown point was accepted");

        requireInvalid(
            [&]() {
                auto invertedSlots = slots();
                std::swap(
                    invertedSlots[0].record.vertices[2],
                    invertedSlots[0].record.vertices[3]);
                (void)s::TetraOptimizationState::fromCanonicalSites(
                    canonicalSites,
                    invertedSlots,
                    constraints());
            },
            "non-positive live TET4 state was accepted");

        requireInvalid(
            [&]() {
                s::ConstraintView stale(
                    {{999U, s::PointMobility::Fixed}});
                (void)s::TetraOptimizationState::fromCanonicalSites(
                    canonicalSites,
                    slots(),
                    std::move(stale));
            },
            "constraint referencing unknown PointId was accepted");

        // --- incidence index ---------------------------------------------
        //
        // A star of four tetrahedra around an interior point, plus a point no
        // live tetra references. Every point is cross-checked against the scan
        // the index replaced.
        {
            const std::vector<CanonicalSite> starSites{
                {10U, {0.05, 0.05, 0.05}, {}},
                {30U, {1.0, 0.0, 0.0}, {}},
                {50U, {0.0, 1.0, 0.0}, {}},
                {70U, {0.0, 0.0, 1.0}, {}},
                {90U, {-1.0, -1.0, -1.0}, {}},
                {110U, {5.0, 5.0, 5.0}, {}},
            };

            std::vector<std::array<PointId, 4>> cells{
                {10U, 30U, 50U, 70U},
                {10U, 30U, 90U, 50U},
                {10U, 30U, 70U, 90U},
                {10U, 50U, 90U, 70U},
            };

            std::map<PointId, femcae::geometry::Vec3> coordinates;
            for (const CanonicalSite& site : starSites) {
                coordinates.emplace(site.id, site.point);
            }

            std::vector<TetSlot> starSlots(cells.size());
            for (std::size_t index = 0; index < cells.size(); ++index) {
                auto& vertices = cells[index];
                const auto orientation = [&]() {
                    return femcae::meshing::predicates::orient3d(
                        coordinates.at(vertices[0]),
                        coordinates.at(vertices[1]),
                        coordinates.at(vertices[2]),
                        coordinates.at(vertices[3]));
                };
                if (orientation().sign ==
                    femcae::meshing::predicates::PredicateSign::Negative) {
                    std::swap(vertices[2], vertices[3]);
                }
                starSlots[index].generation =
                    static_cast<std::uint32_t>(index + 11U);
                starSlots[index].live = true;
                starSlots[index].record.vertices = vertices;
                starSlots[index].record.neighbors = {
                    InvalidTetHandle,
                    InvalidTetHandle,
                    InvalidTetHandle,
                    InvalidTetHandle};
            }

            // validateTetTopology requires reciprocal neighbours on every
            // shared face, so the star has to be wired before it is accepted.
            for (std::size_t lhs = 0; lhs < starSlots.size(); ++lhs) {
                for (std::size_t rhs = lhs + 1U; rhs < starSlots.size(); ++rhs) {
                    for (std::size_t lhsFace = 0; lhsFace < 4U; ++lhsFace) {
                        for (std::size_t rhsFace = 0; rhsFace < 4U; ++rhsFace) {
                            if (!(femcae::meshing::canonicalFaceKey(
                                      starSlots[lhs].record, lhsFace) ==
                                  femcae::meshing::canonicalFaceKey(
                                      starSlots[rhs].record, rhsFace))) {
                                continue;
                            }
                            starSlots[lhs].record.neighbors[lhsFace] = {
                                static_cast<std::uint32_t>(rhs),
                                starSlots[rhs].generation};
                            starSlots[rhs].record.neighbors[rhsFace] = {
                                static_cast<std::uint32_t>(lhs),
                                starSlots[lhs].generation};
                        }
                    }
                }
            }

            std::vector<s::PointMobilityEntry> mobility;
            for (const CanonicalSite& site : starSites) {
                mobility.push_back({
                    site.id,
                    site.id == 10U
                        ? s::PointMobility::InteriorFree
                        : s::PointMobility::Fixed});
            }

            const auto starState =
                s::TetraOptimizationState::fromCanonicalSites(
                    starSites,
                    starSlots,
                    s::ConstraintView(mobility));

            for (const CanonicalSite& site : starSites) {
                const auto actual =
                    starState.incidentTetrahedra(site.id);
                const auto expected =
                    scanIncident(starSlots, site.id);
                require(
                    actual == expected,
                    "incidence index disagrees with the reference scan");

                for (std::size_t i = 1; i < actual.size(); ++i) {
                    require(
                        actual[i - 1U].slot < actual[i].slot,
                        "incidence row is not ascending by slot");
                }
            }

            require(
                starState.incidentTetrahedra(110U).empty(),
                "a known point with no incident live tetra must return empty");

            bool threw = false;
            try {
                (void)starState.incidentTetrahedra(999U);
            } catch (const std::out_of_range&) {
                threw = true;
            }
            require(
                threw,
                "an unknown PointId must stay distinguishable from an empty star");

            // The star of the interior point is exactly the live set here, so
            // the index must reproduce liveTetrahedra() order as well.
            require(
                starState.incidentTetrahedra(10U) ==
                    starState.liveTetrahedra(),
                "interior star lost the live-slot ordering");
        }

        std::cout
            << "M6 I4 foundation optimizer state PASS\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M6 I4 foundation optimizer state FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
