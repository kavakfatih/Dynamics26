#include "meshing/m2/DelaunayTransaction.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
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
    geometry::Vec3 query,
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
        plan.candidateCells.push_back(plan.candidateCells.front());
        plan.requiredSlotCount += 1U;
        const auto result =
            commitDelaunayInsertion(arena, sites, plan);
        check(!result.ok(), "duplicate/non-manifold candidate patch rejected");
        check(exactStateFingerprint(arena) == before,
              "duplicate candidate causes no partial mutation");
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
    testFailureNoMutation();
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
