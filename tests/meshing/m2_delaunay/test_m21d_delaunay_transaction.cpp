#include "meshing/m2/DelaunayTransaction.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace femcae::meshing;
using namespace femcae::meshing::m2;

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const std::string& message) {
    ++checks;
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

std::vector<CanonicalSite> tetraSites(
    femcae::geometry::Vec3 query,
    PointId queryId = 5U) {
    return {
        {1U, {0.0, 0.0, 0.0}},
        {2U, {1.0, 0.0, 0.0}},
        {3U, {0.0, 1.0, 0.0}},
        {4U, {0.0, 0.0, 1.0}},
        {queryId, query}};
}

DelaunayReferenceArena bootstrap(
    const std::vector<CanonicalSite>& sites) {
    DelaunayBootstrapResult result = buildDelaunayBootstrap(sites);
    if (result.status != DelaunayBootstrapStatus::Ready) {
        throw std::runtime_error("test bootstrap did not produce 3D topology");
    }
    return std::move(result.arena);
}

DelaunayReferenceArena permuteLiveSlots(
    const DelaunayReferenceArena& source,
    const std::vector<std::size_t>& oldSlotsInNewOrder) {
    if (oldSlotsInNewOrder.size() != source.slots().size()) {
        throw std::runtime_error("slot permutation must cover the complete bootstrap");
    }
    std::vector<std::size_t> newSlotForOld(source.slots().size(), 0U);
    std::vector<bool> seen(source.slots().size(), false);
    for (std::size_t newSlot = 0U;
         newSlot < oldSlotsInNewOrder.size(); ++newSlot) {
        const std::size_t oldSlot = oldSlotsInNewOrder[newSlot];
        if (oldSlot >= source.slots().size() || seen[oldSlot]) {
            throw std::runtime_error("slot permutation is not bijective");
        }
        seen[oldSlot] = true;
        newSlotForOld[oldSlot] = newSlot;
    }

    DelaunayReferenceArena result;
    result.reserve(source.slots().size());
    for (const std::size_t oldSlot : oldSlotsInNewOrder) {
        const DelaunayCellSlot& sourceSlot = source.slots()[oldSlot];
        if (!sourceSlot.live || sourceSlot.generation != 1U) {
            throw std::runtime_error("P1D slot fixture expects live generation-1 bootstrap cells");
        }
        DelaunayCellRecord record = sourceSlot.record;
        for (DelaunayCellHandle& neighbor : record.neighbors) {
            if (!neighbor.isValid() ||
                neighbor.slot >= newSlotForOld.size() ||
                neighbor.generation != 1U) {
                throw std::runtime_error("slot permutation encountered invalid bootstrap neighbor");
            }
            neighbor.slot = static_cast<std::uint32_t>(
                newSlotForOld[neighbor.slot]);
        }
        (void)result.appendCell(record);
    }
    return result;
}

std::string vertexText(const DelaunayVertexRef& vertex) {
    if (vertex.isInfinite()) return "I";
    return std::to_string(*vertex.finitePointId());
}

std::string exactStateFingerprint(const DelaunayReferenceArena& arena) {
    std::ostringstream out;
    out << "slots=" << arena.slots().size()
        << ";live=" << arena.liveCount()
        << ";version=" << arena.topologyVersion() << ';';
    for (std::size_t i = 0U; i < arena.slots().size(); ++i) {
        const auto& slot = arena.slots()[i];
        out << i << ':' << slot.generation << ':' << slot.live << ':';
        for (const auto& vertex : slot.record.vertices) {
            out << vertexText(vertex) << ',';
        }
        out << ':';
        for (const auto& neighbor : slot.record.neighbors) {
            out << neighbor.slot << '/' << neighbor.generation << ',';
        }
        out << ":epoch=" << slot.record.visitEpoch << ';';
    }
    return out.str();
}

std::string canonicalTopologyFingerprint(
    const DelaunayReferenceArena& arena) {
    std::vector<std::string> cells;
    for (const auto& slot : arena.slots()) {
        if (!slot.live) continue;
        auto vertices = slot.record.vertices;
        std::sort(vertices.begin(), vertices.end());
        std::ostringstream item;
        item << (slot.record.vertices[0].isInfinite() ? "G:" : "F:");
        for (const auto& vertex : vertices) {
            item << vertexText(vertex) << ',';
        }
        item << "|";
        std::vector<std::string> faces;
        for (std::size_t face = 0U; face < 4U; ++face) {
            const auto key = canonicalDelaunayFaceKey(slot.record, face);
            std::ostringstream f;
            for (const auto& vertex : key.vertices) {
                f << vertexText(vertex) << ',';
            }
            faces.push_back(f.str());
        }
        std::sort(faces.begin(), faces.end());
        for (const auto& face : faces) item << face << '/';
        cells.push_back(item.str());
    }
    std::sort(cells.begin(), cells.end());
    std::ostringstream out;
    for (const auto& cell : cells) out << cell << ';';
    return out.str();
}

void requirePlanOracleEquality(const DelaunayInsertionPlan& plan) {
    check(!plan.conflictOracle.empty(), "conflict oracle is non-empty");
    check(plan.conflictOracle == plan.conflictFlood,
          "M2-G11 flood exactly equals global oracle");
    check(4U * plan.conflictOracle.size() ==
              2U * plan.internalFacets.size() + plan.boundaryFacets.size(),
          "M2-G12 4C=2I+B");
    check(plan.candidateCells.size() == plan.boundaryFacets.size(),
          "one candidate per cavity boundary face");
    check(plan.externalRewires.size() == plan.boundaryFacets.size(),
          "one external rewire per cavity base");
}

void commitAndValidate(
    DelaunayReferenceArena& arena,
    const std::vector<CanonicalSite>& sites,
    DelaunayInsertionPlan& plan,
    const std::string& label) {
    const auto reserve =
        reserveDelaunayInsertion(arena, sites, plan);
    check(reserve.ok(), label + ": reserve succeeds");
    if (!reserve.ok()) return;

    const std::size_t capacity = arena.capacity();
    const auto commit =
        commitDelaunayInsertion(arena, sites, plan);
    check(commit.ok(), label + ": commit succeeds");
    check(commit.commitBarrierCrossed,
          label + ": commit barrier crossed only on success");
    check(arena.capacity() == capacity,
          label + ": no arena allocation after commit barrier");
    check(validateDelaunayTopology(arena.slots(), sites).ok(),
          label + ": post-state typed topology validator passes");
}

void testInterior() {
    auto sites = tetraSites({0.125, 0.125, 0.125});
    auto arena = bootstrap(sites);
    const auto before = exactStateFingerprint(arena);

    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "strict interior plan succeeds");
    check(exactStateFingerprint(arena) == before,
          "planning strict interior does not mutate arena");
    if (!planned.ok()) return;

    requirePlanOracleEquality(*planned.plan);
    check(planned.plan->conflictOracle.size() == 1U,
          "strict interior bootstrap cavity has one conflict cell");
    check(planned.plan->internalFacets.empty(),
          "strict interior bootstrap cavity has no internal faces");
    check(planned.plan->boundaryFacets.size() == 4U,
          "strict interior bootstrap cavity has four boundary faces");

    commitAndValidate(arena, sites, *planned.plan, "strict interior");
    check(arena.liveCount() == 8U,
          "strict interior replaces 1 cell by 4 cells");
}

void testOutsideHull() {
    auto sites = tetraSites({2.0, 2.0, 2.0});
    auto arena = bootstrap(sites);

    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "strict exterior plan succeeds");
    if (!planned.ok()) return;
    requirePlanOracleEquality(*planned.plan);

    const std::size_t ghostConflict = std::count_if(
        planned.plan->conflictOracle.begin(),
        planned.plan->conflictOracle.end(),
        [&](DelaunayCellHandle handle) {
            return arena.cell(handle).vertices[0].isInfinite();
        });
    check(ghostConflict > 0U, "outside-hull cavity contains ghost conflict");
    commitAndValidate(arena, sites, *planned.plan, "outside hull");
}

void testHullFacet() {
    auto sites = tetraSites({0.25, 0.25, 0.0});
    auto arena = bootstrap(sites);
    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "exact hull facet plan succeeds");
    if (!planned.ok()) return;
    requirePlanOracleEquality(*planned.plan);
    check(planned.plan->conflictOracle.size() >= 2U,
          "hull facet point conflicts with finite and ghost sides");
    commitAndValidate(arena, sites, *planned.plan, "hull facet");
}

void testCosphericalGolden() {
    std::vector<CanonicalSite> sites{
        {1U, {0.0, 0.0, 0.0}},
        {2U, {0.0, 0.0, 1.0}},
        {3U, {0.0, 1.0, 0.0}},
        {4U, {1.0, 0.0, 0.0}},
        {5U, {1.0, 1.0, 1.0}}};
    auto arena = bootstrap(sites);
    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "co-spherical D26LIFT1 plan succeeds");
    if (!planned.ok()) return;
    requirePlanOracleEquality(*planned.plan);
    commitAndValidate(arena, sites, *planned.plan, "co-spherical");

    std::vector<std::array<PointId, 4>> finite;
    for (const auto& slot : arena.slots()) {
        if (!slot.live ||
            !std::all_of(slot.record.vertices.begin(), slot.record.vertices.end(),
                         [](const auto& v) { return v.isFinite(); })) {
            continue;
        }
        std::array<PointId, 4> ids{};
        for (std::size_t i = 0U; i < 4U; ++i) {
            ids[i] = *slot.record.vertices[i].finitePointId();
        }
        std::sort(ids.begin(), ids.end());
        finite.push_back(ids);
    }
    std::sort(finite.begin(), finite.end());
    const std::vector<std::array<PointId, 4>> expected{
        {1U, 2U, 3U, 4U},
        {2U, 3U, 4U, 5U}};
    check(finite == expected,
          "five-site D26LIFT1 finite connectivity matches frozen golden");
}

void testSharedFiniteFacetAndEdge() {
    {
        std::vector<CanonicalSite> sites{
            {1U, {0.0, 0.0, 0.0}},
            {2U, {1.0, 0.0, 0.0}},
            {3U, {0.0, 1.0, 0.0}},
            {4U, {0.0, 0.0, 1.0}},
            {5U, {0.125, 0.125, 0.125}},
            {6U, {0.3125, 0.0625, 0.0625}}};
        auto arena = bootstrap(sites);
        auto first = buildDelaunayInsertionPlan(arena, sites, 5U);
        check(first.ok(), "shared-facet setup first insertion plans");
        if (!first.ok()) return;
        commitAndValidate(arena, sites, *first.plan, "shared-facet setup");

        auto facet = buildDelaunayInsertionPlan(arena, sites, 6U);
        check(facet.ok(), "exact shared finite facet insertion plans");
        if (facet.ok()) {
            requirePlanOracleEquality(*facet.plan);
            check(facet.plan->conflictOracle.size() >= 2U,
                  "shared finite facet cavity includes both sides");
            commitAndValidate(arena, sites, *facet.plan, "shared finite facet");
        }
    }

    {
        std::vector<CanonicalSite> sites{
            {1U, {0.0, 0.0, 0.0}},
            {2U, {1.0, 0.0, 0.0}},
            {3U, {0.0, 1.0, 0.0}},
            {4U, {0.0, 0.0, 1.0}},
            {5U, {0.125, 0.125, 0.125}},
            {6U, {0.0625, 0.0625, 0.0625}}};
        auto arena = bootstrap(sites);
        auto first = buildDelaunayInsertionPlan(arena, sites, 5U);
        check(first.ok(), "edge setup first insertion plans");
        if (!first.ok()) return;
        commitAndValidate(arena, sites, *first.plan, "edge setup");

        auto edge = buildDelaunayInsertionPlan(arena, sites, 6U);
        check(edge.ok(), "exact finite edge insertion plans");
        if (edge.ok()) {
            requirePlanOracleEquality(*edge.plan);
            check(edge.plan->conflictOracle.size() >= 3U,
                  "edge cavity discovers complete incident star");
            commitAndValidate(arena, sites, *edge.plan, "finite edge");
        }
    }
}

void testReversedEnumerationDeterminism() {
    auto forward = tetraSites({0.125, 0.125, 0.125});
    auto reverse = forward;
    std::reverse(reverse.begin(), reverse.end());

    auto arenaA = bootstrap(forward);
    auto arenaB = bootstrap(reverse);
    auto planA = buildDelaunayInsertionPlan(arenaA, forward, 5U);
    auto planB = buildDelaunayInsertionPlan(arenaB, reverse, 5U);
    check(planA.ok() && planB.ok(),
          "reversed input enumeration plans both succeed");
    if (!planA.ok() || !planB.ok()) return;

    commitAndValidate(arenaA, forward, *planA.plan, "enumeration forward");
    commitAndValidate(arenaB, reverse, *planB.plan, "enumeration reverse");
    check(canonicalTopologyFingerprint(arenaA) ==
              canonicalTopologyFingerprint(arenaB),
          "reversed input enumeration gives same canonical P1D topology");
}


void testCellSlotPermutationDeterminism() {
    auto sites = tetraSites({0.125, 0.125, 0.125});
    auto canonicalArena = bootstrap(sites);
    auto permutedArena = permuteLiveSlots(
        canonicalArena,
        {4U, 2U, 0U, 3U, 1U});

    check(validateDelaunayTopology(canonicalArena.slots(), sites).ok(),
          "canonical bootstrap valid before slot-permutation test");
    check(validateDelaunayTopology(permutedArena.slots(), sites).ok(),
          "legally slot-permuted bootstrap remains valid");

    auto canonicalPlan =
        buildDelaunayInsertionPlan(canonicalArena, sites, 5U);
    auto permutedPlan =
        buildDelaunayInsertionPlan(permutedArena, sites, 5U);
    check(canonicalPlan.ok() && permutedPlan.ok(),
          "canonical and slot-permuted P1D plans both succeed");
    if (!canonicalPlan.ok() || !permutedPlan.ok()) return;

    requirePlanOracleEquality(*canonicalPlan.plan);
    requirePlanOracleEquality(*permutedPlan.plan);
    commitAndValidate(
        canonicalArena, sites, *canonicalPlan.plan,
        "canonical-slot commit");
    commitAndValidate(
        permutedArena, sites, *permutedPlan.plan,
        "permuted-slot commit");

    check(canonicalTopologyFingerprint(canonicalArena) ==
              canonicalTopologyFingerprint(permutedArena),
          "legal cell-slot permutation cannot change canonical P1D topology");
}

void testFailureNoMutation() {
    auto sites = tetraSites({0.125, 0.125, 0.125});
    auto arena = bootstrap(sites);

    {
        const auto before = exactStateFingerprint(arena);
        auto duplicate = buildDelaunayInsertionPlan(arena, sites, 1U);
        check(!duplicate.ok() &&
                  duplicate.failure ==
                      DelaunayTransactionFailure::DuplicateLiveSite,
              "live query PointId is rejected explicitly");
        check(exactStateFingerprint(arena) == before,
              "duplicate query failure leaves topology identical");
    }

    auto good = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(good.ok(), "failure-injection base plan succeeds");
    if (!good.ok()) return;

    {
        auto plan = *good.plan;
        const auto before = exactStateFingerprint(arena);
        auto finite = std::find_if(
            plan.candidateCells.begin(), plan.candidateCells.end(),
            [](const auto& candidate) {
                return std::all_of(
                    candidate.record.vertices.begin(),
                    candidate.record.vertices.end(),
                    [](const auto& v) { return v.isFinite(); });
            });
        check(finite != plan.candidateCells.end(),
              "failure injection finds finite candidate");
        if (finite != plan.candidateCells.end()) {
            std::swap(finite->record.vertices[0],
                      finite->record.vertices[1]);
            const auto result =
                commitDelaunayInsertion(arena, sites, plan);
            check(!result.ok() &&
                      result.failure ==
                          DelaunayTransactionFailure::DegenerateCandidate,
                  "invalid finite orientation rejected pre-commit");
            check(exactStateFingerprint(arena) == before,
                  "invalid finite candidate causes no partial mutation");
        }
    }

    {
        auto plan = *good.plan;
        const auto before = exactStateFingerprint(arena);
        plan.candidateCells[0].record.neighbors[0] =
            InvalidDelaunayCellHandle;
        const auto result =
            commitDelaunayInsertion(arena, sites, plan);
        check(!result.ok(), "missing reciprocal/new adjacency rejected");
        check(exactStateFingerprint(arena) == before,
              "missing adjacency causes no partial mutation");
    }

    {
        auto plan = *good.plan;
        const auto before = exactStateFingerprint(arena);
        check(plan.candidateCells.size() >= 2U,
              "duplicate injection has two candidate cells");
        if (plan.candidateCells.size() >= 2U) {
            plan.candidateCells[1].record =
                plan.candidateCells[0].record;
            const auto result =
                commitDelaunayInsertion(arena, sites, plan);
            check(!result.ok() &&
                      result.failure ==
                          DelaunayTransactionFailure::DuplicateCandidate,
                  "duplicate candidate connectivity rejected explicitly");
            check(exactStateFingerprint(arena) == before,
                  "duplicate candidate causes no partial mutation");
        }
    }

    {
        auto plan = *good.plan;
        const auto before = exactStateFingerprint(arena);
        ++plan.conflictOracle.front().generation;
        ++plan.conflictFlood.front().generation;
        const auto result =
            commitDelaunayInsertion(arena, sites, plan);
        check(!result.ok() &&
                  result.failure ==
                      DelaunayTransactionFailure::InvalidTopology,
              "stale cavity handle generation rejected explicitly");
        check(exactStateFingerprint(arena) == before,
              "stale generation causes no partial mutation");
    }

    {
        const auto before = exactStateFingerprint(arena);
        auto missingQuery =
            buildDelaunayInsertionPlan(arena, sites, 999U);
        check(!missingQuery.ok() &&
                  missingQuery.failure ==
                      DelaunayTransactionFailure::InvalidQuery,
              "missing query PointId rejected explicitly");
        check(exactStateFingerprint(arena) == before,
              "invalid query causes no partial mutation");
    }

    {
        auto plan = *good.plan;
        const auto before = exactStateFingerprint(arena);
        const DelaunayResourceLimits limit{arena.slots().size()};
        const auto result =
            reserveDelaunayInsertion(arena, sites, plan, limit);
        check(!result.ok() &&
                  result.failure ==
                      DelaunayTransactionFailure::ResourceLimit,
              "controlled capacity limit returns typed resource status");
        check(exactStateFingerprint(arena) == before,
              "resource limit changes no topology state");
    }

    {
        const auto before = exactStateFingerprint(arena);
        DelaunayCellRecord invalid;
        invalid.vertices[0] = DelaunayVertexRef::finite(1U);
        invalid.vertices[1] = DelaunayVertexRef::finite(2U);
        invalid.vertices[2] = DelaunayVertexRef::finite(3U);
        invalid.vertices[3] = DelaunayVertexRef::finite(4U);
        DelaunayReferenceArena badArena;
        badArena.appendCell(invalid);
        const auto badBefore = exactStateFingerprint(badArena);
        auto result = buildDelaunayInsertionPlan(badArena, sites, 5U);
        check(!result.ok() &&
                  result.failure ==
                      DelaunayTransactionFailure::InvalidTopology,
              "invalid source topology rejected explicitly");
        check(exactStateFingerprint(badArena) == badBefore,
              "invalid topology planning failure does not mutate bad arena");
        check(exactStateFingerprint(arena) == before,
              "invalid-topology fixture cannot affect good arena");
    }
}



void testNonManifoldFaceIncidenceNoMutation() {
    auto sites = tetraSites({0.125, 0.125, 0.125});
    sites.push_back({6U, {0.75, 0.625, 0.5}});
    auto arena = bootstrap(sites);
    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "non-manifold injection base plan succeeds");
    if (!planned.ok()) return;

    auto plan = *planned.plan;
    using Owner = std::pair<std::size_t, std::size_t>;
    std::map<DelaunayFaceKey, std::vector<Owner>> owners;
    for (std::size_t candidate = 0U;
         candidate < plan.candidateCells.size(); ++candidate) {
        for (std::size_t face = 0U; face < 4U; ++face) {
            owners[canonicalDelaunayFaceKey(
                plan.candidateCells[candidate].record, face)]
                .push_back({candidate, face});
        }
    }

    std::map<PointId, femcae::geometry::Vec3> points;
    for (const CanonicalSite& site : sites) {
        points.emplace(site.id, site.point);
    }

    std::optional<DelaunayFaceKey> injectedFace;
    std::array<DelaunayVertexRef, 4> injectedVertices{};
    std::size_t thirdCandidate = plan.candidateCells.size();
    for (const auto& [key, faceOwners] : owners) {
        if (faceOwners.size() != 2U ||
            !std::all_of(
                key.vertices.begin(), key.vertices.end(),
                [](const DelaunayVertexRef& vertex) {
                    return vertex.isFinite();
                })) {
            continue;
        }

        const auto third = std::find_if(
            plan.candidateCells.begin(),
            plan.candidateCells.end(),
            [&](const DelaunayCandidateCell& candidate) {
                const std::size_t index =
                    static_cast<std::size_t>(
                        &candidate - plan.candidateCells.data());
                return index != faceOwners[0].first &&
                       index != faceOwners[1].first;
            });
        if (third == plan.candidateCells.end()) continue;

        injectedVertices[0] = key.vertices[0];
        injectedVertices[1] = key.vertices[1];
        injectedVertices[2] = key.vertices[2];
        injectedVertices[3] = DelaunayVertexRef::finite(6U);
        auto p = [&](const DelaunayVertexRef& vertex) {
            return points.at(*vertex.finitePointId());
        };
        auto sign = predicates::orient3d(
            p(injectedVertices[0]),
            p(injectedVertices[1]),
            p(injectedVertices[2]),
            p(injectedVertices[3])).sign;
        if (sign == predicates::PredicateSign::Zero) continue;
        if (sign == predicates::PredicateSign::Negative) {
            std::swap(injectedVertices[0], injectedVertices[1]);
        }
        injectedFace = key;
        thirdCandidate =
            static_cast<std::size_t>(
                third - plan.candidateCells.begin());
        break;
    }

    check(injectedFace.has_value() &&
              thirdCandidate < plan.candidateCells.size(),
          "fixture found a non-coplanar third owner for a lateral face");
    if (!injectedFace.has_value() ||
        thirdCandidate >= plan.candidateCells.size()) {
        return;
    }

    DelaunayCandidateCell& corrupt =
        plan.candidateCells[thirdCandidate];
    DelaunayCellRecord record;
    record.vertices = injectedVertices;
    record.neighbors =
        corrupt.record.neighbors;
    record.neighbors[3] = corrupt.outsideCell;
    corrupt.record = record;
    corrupt.baseFace = *injectedFace;
    corrupt.baseLocalFace = 3U;

    std::vector<DelaunayCellSlot> simulated(
        arena.slots().begin(), arena.slots().end());
    for (const DelaunayCellHandle handle : plan.conflictOracle) {
        simulated[handle.slot].live = false;
    }
    for (const DelaunayCandidateCell& candidate :
         plan.candidateCells) {
        DelaunayCellSlot slot;
        slot.generation = candidate.futureHandle.generation;
        slot.live = true;
        slot.record = candidate.record;
        simulated.push_back(slot);
    }
    for (const DelaunayExternalRewire& rewire :
         plan.externalRewires) {
        simulated[rewire.outsideCell.slot]
            .record.neighbors[rewire.outsideLocalFace] =
            rewire.newCell;
    }
    const auto report =
        validateDelaunayTopology(simulated, sites);
    const bool hasNonManifoldFace = std::any_of(
        report.issues.begin(), report.issues.end(),
        [](const DelaunayTopologyIssue& issue) {
            return issue.code ==
                   DelaunayTopologyIssueCode::FaceIncidenceNotTwo;
        });
    check(hasNonManifoldFace,
          "independent typed validator observes non-manifold face incidence");

    const auto before = exactStateFingerprint(arena);
    const auto result =
        commitDelaunayInsertion(arena, sites, plan);
    check(!result.ok() &&
              result.failure ==
                  DelaunayTransactionFailure::InvalidCandidateTopology,
          "non-manifold candidate patch is rejected before commit barrier");
    check(exactStateFingerprint(arena) == before,
          "non-manifold candidate failure causes no partial mutation");
}

void testInvalidGhostOrientationNoMutation() {
    auto sites = tetraSites({2.0, 2.0, 2.0});
    auto arena = bootstrap(sites);
    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "ghost-orientation injection base plan succeeds");
    if (!planned.ok()) return;

    auto plan = *planned.plan;
    const auto ghost = std::find_if(
        plan.candidateCells.begin(), plan.candidateCells.end(),
        [](const DelaunayCandidateCell& candidate) {
            return candidate.record.vertices[0].isInfinite();
        });
    check(ghost != plan.candidateCells.end(),
          "outside-hull patch contains ghost candidate");
    if (ghost == plan.candidateCells.end()) return;

    const auto before = exactStateFingerprint(arena);
    std::swap(
        ghost->record.vertices[1],
        ghost->record.vertices[2]);
    const auto result =
        commitDelaunayInsertion(arena, sites, plan);
    check(!result.ok() &&
              result.failure ==
                  DelaunayTransactionFailure::InvalidGhostOrientation,
          "inward ghost candidate rejected with typed failure");
    check(exactStateFingerprint(arena) == before,
          "invalid ghost orientation causes no partial mutation");
}

void testStalePlan() {
    std::vector<CanonicalSite> sites{
        {1U, {0.0, 0.0, 0.0}},
        {2U, {1.0, 0.0, 0.0}},
        {3U, {0.0, 1.0, 0.0}},
        {4U, {0.0, 0.0, 1.0}},
        {5U, {0.125, 0.125, 0.125}},
        {6U, {0.25, 0.125, 0.125}}};
    auto arena = bootstrap(sites);
    auto stale = buildDelaunayInsertionPlan(arena, sites, 5U);
    auto winner = buildDelaunayInsertionPlan(arena, sites, 6U);
    check(stale.ok() && winner.ok(), "two snapshot-A plans build");
    if (!stale.ok() || !winner.ok()) return;

    commitAndValidate(arena, sites, *winner.plan, "stale-plan winner");
    const auto before = exactStateFingerprint(arena);
    const auto staleResult =
        commitDelaunayInsertion(arena, sites, *stale.plan);
    check(!staleResult.ok() &&
              staleResult.failure ==
                  DelaunayTransactionFailure::StalePlan,
          "snapshot-A plan rejects after arena advances to B");
    check(exactStateFingerprint(arena) == before,
          "stale plan rejection causes no mutation");
}

void testNearDegenerateExactNonzero() {
    const double tiny = 0x1p-500;
    std::vector<CanonicalSite> sites{
        {1U, {0.0, 0.0, 0.0}},
        {2U, {1.0, 0.0, 0.0}},
        {3U, {0.0, 1.0, 0.0}},
        {4U, {0.0, 0.0, tiny}},
        {5U, {0.125, 0.125, 0.125 * tiny}}};
    auto arena = bootstrap(sites);
    auto planned = buildDelaunayInsertionPlan(arena, sites, 5U);
    check(planned.ok(), "near-degenerate exact-nonzero insertion plans");
    if (!planned.ok()) return;
    requirePlanOracleEquality(*planned.plan);
    commitAndValidate(arena, sites, *planned.plan,
                      "near-degenerate exact-nonzero");
}

} // namespace

int main() {
    testInterior();
    testOutsideHull();
    testHullFacet();
    testCosphericalGolden();
    testSharedFiniteFacetAndEdge();
    testReversedEnumerationDeterminism();
    testCellSlotPermutationDeterminism();
    testFailureNoMutation();
    testNonManifoldFaceIncidenceNoMutation();
    testInvalidGhostOrientationNoMutation();
    testStalePlan();
    testNearDegenerateExactNonzero();

    if (failures != 0) {
        std::cerr << failures << " / " << checks
                  << " P1D checks failed\n";
        return 1;
    }
    std::cout << "M2.1-D cavity/transaction: "
              << checks << " checks passed\n";
    return 0;
}
