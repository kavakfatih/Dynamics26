#include "meshing/m6/state/TetraOptimizationState.h"
#include "meshing/m6/quality/QualityVector.h"

#include <algorithm>
#include <array>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

namespace s = femcae::meshing::m6::state;
namespace q = femcae::meshing::m6::quality;
using femcae::meshing::CanonicalFaceKey;
using femcae::meshing::CanonicalSite;
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
