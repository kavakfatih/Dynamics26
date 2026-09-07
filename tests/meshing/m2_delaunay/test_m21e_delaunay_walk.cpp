#include "meshing/m2/DelaunayWalk.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace femcae::meshing;
using namespace femcae::meshing::m2;
using femcae::geometry::Vec3;
using predicates::PredicateSign;
using Kind = DelaunayLocationKind;

std::size_t checks = 0U;

void require(bool value, const std::string& message) {
    ++checks;
    if (!value) {
        throw std::runtime_error(message);
    }
}

struct Fixture {
    std::vector<CanonicalSite> sites;
    std::vector<DelaunayCellSlot> slots;
};

struct QualificationStats {
    std::uint64_t queries{0};
    std::uint64_t successes{0};
    std::uint64_t stalls{0};
    std::uint64_t mismatches{0};
    std::array<std::uint64_t, 5> kinds{};
    DelaunayWalkTelemetry walk;
};

QualificationStats qualification;

std::array<DelaunayVertexRef, 3> face(
    const DelaunayCellRecord& cell,
    std::size_t localFace) {
    std::array<DelaunayVertexRef, 3> key;
    std::size_t write = 0U;
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        if (vertex != localFace) {
            key[write++] = cell.vertices[vertex];
        }
    }
    std::sort(key.begin(), key.end());
    return key;
}

std::array<DelaunayVertexRef, 4> cellIdentity(
    const DelaunayCellRecord& cell) {
    auto key = cell.vertices;
    std::sort(key.begin(), key.end());
    return key;
}

bool finiteCell(const DelaunayCellRecord& cell) {
    return std::all_of(
        cell.vertices.begin(),
        cell.vertices.end(),
        [](const DelaunayVertexRef& vertex) {
            return vertex.isFinite();
        });
}

Fixture bootstrap(double scale = 1.0, double height = 1.0) {
    Fixture fixture;
    fixture.sites = {
        {10U, {0.0, 0.0, 0.0}, {}},
        {80U, {scale, 0.0, 0.0}, {}},
        {30U, {0.0, scale, 0.0}, {}},
        {900U, {0.0, 0.0, height}, {}}};
    const auto built =
        buildDelaunayBootstrap(fixture.sites);
    require(
        built.status == DelaunayBootstrapStatus::Ready,
        "bootstrap fixture must be three-dimensional");
    fixture.slots.assign(
        built.arena.slots().begin(),
        built.arena.slots().end());
    return fixture;
}

Fixture twoCells() {
    // P1C ile ayni bagimsiz sabit fixture: z=0 ucgenini paylasan iki tetra.
    Fixture fixture;
    fixture.sites = {
        {10U, {0.0, 0.0, 0.0}, {}},
        {80U, {1.0, 0.0, 0.0}, {}},
        {30U, {0.0, 1.0, 0.0}, {}},
        {900U, {0.0, 0.0, 1.0}, {}},
        {700U, {0.0, 0.0, -1.0}, {}}};

    std::map<PointId, Vec3> points;
    for (const auto& site : fixture.sites) {
        points.emplace(site.id, site.point);
    }

    for (const auto ids :
         {std::array<PointId, 4>{10U, 80U, 30U, 900U},
          std::array<PointId, 4>{10U, 80U, 30U, 700U}}) {
        DelaunayCellSlot slot;
        slot.live = true;
        for (std::size_t i = 0U; i < 4U; ++i) {
            slot.record.vertices[i] =
                DelaunayVertexRef::finite(ids[i]);
        }
        if (predicates::orient3d(
                points.at(ids[0]),
                points.at(ids[1]),
                points.at(ids[2]),
                points.at(ids[3])).sign ==
            PredicateSign::Negative) {
            std::swap(
                slot.record.vertices[0],
                slot.record.vertices[1]);
        }
        fixture.slots.push_back(slot);
    }

    using Owner = std::pair<std::size_t, std::size_t>;
    std::map<
        std::array<DelaunayVertexRef, 3>,
        std::vector<Owner>> faces;
    for (std::size_t cell = 0U; cell < 2U; ++cell) {
        for (std::size_t localFace = 0U;
             localFace < 4U; ++localFace) {
            faces[face(
                fixture.slots[cell].record,
                localFace)].push_back(
                    {cell, localFace});
        }
    }

    for (const auto& [key, owners] : faces) {
        if (owners.size() != 1U) {
            continue;
        }
        const auto [cell, opposite] = owners[0];
        DelaunayCellSlot ghost;
        ghost.live = true;
        ghost.record.vertices[0] =
            DelaunayVertexRef::infinite();
        for (std::size_t i = 0U; i < 3U; ++i) {
            ghost.record.vertices[i + 1U] = key[i];
        }

        const auto oppositePoint =
            points.at(
                *fixture.slots[cell]
                     .record.vertices[opposite]
                     .finitePointId());
        if (predicates::orient3d(
                points.at(*key[0].finitePointId()),
                points.at(*key[1].finitePointId()),
                points.at(*key[2].finitePointId()),
                oppositePoint).sign ==
            PredicateSign::Positive) {
            std::swap(
                ghost.record.vertices[1],
                ghost.record.vertices[2]);
        }
        fixture.slots.push_back(ghost);
    }

    faces.clear();
    for (std::size_t cell = 0U;
         cell < fixture.slots.size(); ++cell) {
        fixture.slots[cell].generation =
            static_cast<std::uint32_t>(cell + 11U);
        for (std::size_t localFace = 0U;
             localFace < 4U; ++localFace) {
            faces[face(
                fixture.slots[cell].record,
                localFace)].push_back(
                    {cell, localFace});
        }
    }

    for (const auto& [key, owners] : faces) {
        (void)key;
        require(
            owners.size() == 2U,
            "two-cell fixture face must have two owners");
        for (std::size_t i = 0U; i < 2U; ++i) {
            const auto [owner, ownerFace] = owners[i];
            const auto other = owners[1U - i].first;
            fixture.slots[owner]
                .record.neighbors[ownerFace] = {
                    static_cast<std::uint32_t>(other),
                    fixture.slots[other].generation};
        }
    }

    require(
        validateDelaunayTopology(
            fixture.slots, fixture.sites).ok(),
        "two-cell fixture topology invalid");
    return fixture;
}

std::vector<
    std::array<DelaunayVertexRef, 4>>
incidentIdentities(const DelaunayLocationResult& result) {
    std::vector<std::array<DelaunayVertexRef, 4>> keys;
    for (const auto& cell : result.incidentCells) {
        keys.push_back(cell.canonicalVertices);
    }
    std::sort(keys.begin(), keys.end());
    return keys;
}

bool semanticAgreement(
    const DelaunayLocationResult& walk,
    const DelaunayLocationResult& oracle) {
    if (walk.kind != oracle.kind ||
        walk.entityVertices != oracle.entityVertices) {
        return false;
    }
    if (walk.kind != Kind::OutsideConvexHull) {
        return incidentIdentities(walk) ==
               incidentIdentities(oracle);
    }
    return walk.entityVertices.empty() &&
           walk.incidentCells.empty() &&
           walk.outsideWitness.has_value() &&
           oracle.outsideWitness.has_value();
}

void verifyWalkOutsideWitness(
    const Fixture& fixture,
    const Vec3& query,
    const DelaunayWalkResult& walk) {
    require(
        walk.ok() &&
            walk.location->kind ==
                Kind::OutsideConvexHull &&
            walk.location->outsideWitness.has_value(),
        "walk outside result must retain a witness");
    const auto& witness =
        *walk.location->outsideWitness;

    require(
        witness.finiteCell.slot < fixture.slots.size() &&
            witness.ghostCell.slot < fixture.slots.size(),
        "walk outside witness handle out of range");
    const auto& finite =
        fixture.slots[witness.finiteCell.slot];
    const auto& ghost =
        fixture.slots[witness.ghostCell.slot];
    require(
        finite.live && ghost.live &&
            finite.generation ==
                witness.finiteCell.generation &&
            ghost.generation ==
                witness.ghostCell.generation,
        "walk outside witness generation invalid");
    require(
        finiteCell(finite.record) &&
            ghost.record.vertices[0].isInfinite() &&
            ghost.record.neighbors[0] ==
                witness.finiteCell,
        "walk outside witness is not finite-to-ghost");
    require(
        canonicalDelaunayFaceKey(
            ghost.record, 0U).vertices ==
            walk.conflictSeed->witnessFacet->vertices,
        "outside seed facet does not match ghost finite face");

    std::map<PointId, Vec3> points;
    for (const auto& site : fixture.sites) {
        points.emplace(site.id, site.point);
    }
    require(
        predicates::orient3d(
            points.at(witness.outwardFace[0]),
            points.at(witness.outwardFace[1]),
            points.at(witness.outwardFace[2]),
            query).sign ==
            PredicateSign::Positive,
        "walk outside witness is not strictly violated");

    auto canonical = witness.outwardFace;
    std::sort(canonical.begin(), canonical.end());
    require(
        canonical == witness.canonicalFace,
        "walk outside canonical witness identity mismatch");
    require(
        walk.conflictSeed.has_value() &&
            walk.conflictSeed->cell ==
                witness.ghostCell &&
            walk.conflictSeed->semantic ==
                DelaunayConflictSeedSemantic::OutsideGhost,
        "outside conflict seed is not the crossed ghost");
}

void verifySeedSemantics(
    const DelaunayWalkResult& walk) {
    require(walk.ok(), "seed semantics requires successful walk");
    switch (walk.location->kind) {
    case Kind::Cell:
        require(
            walk.conflictSeed.has_value() &&
                walk.conflictSeed->semantic ==
                    DelaunayConflictSeedSemantic::CellInterior,
            "CELL must produce containing finite seed");
        break;
    case Kind::Facet:
        require(
            walk.conflictSeed.has_value() &&
                walk.conflictSeed->semantic ==
                    DelaunayConflictSeedSemantic::FacetIncident,
            "FACET must produce canonical incident finite seed");
        break;
    case Kind::Edge:
        require(
            walk.conflictSeed.has_value() &&
                walk.conflictSeed->semantic ==
                    DelaunayConflictSeedSemantic::EdgeIncident,
            "EDGE must produce canonical incident finite seed");
        break;
    case Kind::Vertex:
        require(
            !walk.conflictSeed.has_value(),
            "VERTEX must not silently produce an insertion seed");
        break;
    case Kind::OutsideConvexHull:
        require(
            walk.conflictSeed.has_value() &&
                walk.conflictSeed->semantic ==
                    DelaunayConflictSeedSemantic::OutsideGhost,
            "OUTSIDE must produce crossed ghost seed");
        break;
    }
}

void compareAccepted(
    const Fixture& fixture,
    const Vec3& query,
    std::optional<DelaunayCellHandle> hint = std::nullopt) {
    ++qualification.queries;
    const auto walk =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            query,
            hint,
            &qualification.walk);
    const auto oracle =
        locateBruteForceExact(
            fixture.slots,
            fixture.sites,
            query);

    if (!walk.ok()) {
        ++qualification.stalls;
        ++qualification.mismatches;
        throw std::runtime_error(
            "accepted P1E query did not complete the adjacency walk");
    }
    ++qualification.successes;
    ++qualification.kinds[
        static_cast<std::size_t>(walk.location->kind)];

    if (!semanticAgreement(*walk.location, oracle)) {
        ++qualification.mismatches;
        throw std::runtime_error(
            "P1E walk disagrees with P1C brute-force semantic oracle");
    }

    verifySeedSemantics(walk);
    if (walk.location->kind ==
        Kind::OutsideConvexHull) {
        verifyWalkOutsideWitness(
            fixture, query, walk);
    }
}

std::string exactFingerprint(
    const std::vector<DelaunayCellSlot>& slots) {
    std::ostringstream out;
    for (std::size_t i = 0U; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        out << i << ':' << slot.generation << ':'
            << slot.live << ':'
            << slot.record.visitEpoch << ':';
        for (const auto& vertex : slot.record.vertices) {
            if (const auto id = vertex.finitePointId()) {
                out << 'F' << *id;
            } else {
                out << 'I';
            }
            out << ',';
        }
        out << '|';
        for (const auto neighbor : slot.record.neighbors) {
            out << neighbor.slot << '@'
                << neighbor.generation << ',';
        }
        out << ';';
    }
    return out.str();
}

Fixture permuteSlots(
    const Fixture& source,
    const std::vector<std::size_t>& oldSlotsInNewOrder) {
    require(
        oldSlotsInNewOrder.size() ==
            source.slots.size(),
        "slot permutation must cover every slot");

    Fixture result;
    result.sites = source.sites;
    result.slots.resize(source.slots.size());

    std::vector<std::size_t> newSlotForOld(
        source.slots.size(), 0U);
    std::vector<bool> seen(
        source.slots.size(), false);

    for (std::size_t newSlot = 0U;
         newSlot < oldSlotsInNewOrder.size();
         ++newSlot) {
        const std::size_t oldSlot =
            oldSlotsInNewOrder[newSlot];
        require(
            oldSlot < source.slots.size() &&
                !seen[oldSlot],
            "slot permutation must be bijective");
        seen[oldSlot] = true;
        newSlotForOld[oldSlot] = newSlot;
        result.slots[newSlot] =
            source.slots[oldSlot];
    }

    for (auto& slot : result.slots) {
        if (!slot.live) {
            continue;
        }
        for (auto& neighbor : slot.record.neighbors) {
            require(
                neighbor.isValid() &&
                    neighbor.slot <
                        newSlotForOld.size(),
                "fixture neighbor invalid before permutation");
            neighbor.slot =
                static_cast<std::uint32_t>(
                    newSlotForOld[neighbor.slot]);
        }
    }

    require(
        validateDelaunayTopology(
            result.slots, result.sites).ok(),
        "permuted topology must remain valid");
    return result;
}

DelaunayCellHandle finiteHandleContaining(
    const Fixture& fixture,
    PointId id) {
    for (std::size_t slot = 0U;
         slot < fixture.slots.size(); ++slot) {
        const auto& cellSlot = fixture.slots[slot];
        if (!cellSlot.live ||
            !finiteCell(cellSlot.record)) {
            continue;
        }
        if (std::find(
                cellSlot.record.vertices.begin(),
                cellSlot.record.vertices.end(),
                DelaunayVertexRef::finite(id)) !=
            cellSlot.record.vertices.end()) {
            return {
                static_cast<std::uint32_t>(slot),
                cellSlot.generation};
        }
    }
    throw std::runtime_error(
        "requested finite fixture cell not found");
}

void testClassificationAndOracleAgreement() {
    auto fixture = bootstrap();
    const std::vector<Vec3> queries{
        {0.125, 0.125, 0.125},
        {0.25, 0.25, 0.0},
        {0.5, 0.0, 0.0},
        {-0.0, 0.0, -0.0},
        {-1.0, -1.0, -1.0},
        {2.0, 0.0, 0.0}};
    const std::vector<Kind> expected{
        Kind::Cell,
        Kind::Facet,
        Kind::Edge,
        Kind::Vertex,
        Kind::OutsideConvexHull,
        Kind::OutsideConvexHull};

    for (std::size_t i = 0U;
         i < queries.size(); ++i) {
        compareAccepted(fixture, queries[i]);
        const auto walk =
            locateDeterministicWalk(
                fixture.slots,
                fixture.sites,
                queries[i]);
        require(
            walk.location->kind == expected[i],
            "analytic classification family mismatch");
    }
}

void testExactZeroAndOneUlp() {
    auto fixture = bootstrap();

    const auto exactFacet =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.25, 0.25, 0.0});
    require(
        exactFacet.ok() &&
            exactFacet.location->kind == Kind::Facet &&
            exactFacet.trace.crossedFacets.empty(),
        "exact-zero hull facet must classify boundary without crossing");

    const auto exactEdge =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.5, 0.0, 0.0});
    require(
        exactEdge.ok() &&
            exactEdge.location->kind == Kind::Edge &&
            exactEdge.trace.crossedFacets.empty(),
        "exact-zero edge must not become a progress direction");

    const auto exactVertex =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.0, 0.0, 0.0});
    require(
        exactVertex.ok() &&
            exactVertex.location->kind == Kind::Vertex &&
            exactVertex.trace.crossedFacets.empty(),
        "exact-zero vertex must not become a progress direction");

    for (double z :
         {std::nextafter(0.0, 1.0),
          std::nextafter(0.0, -1.0)}) {
        compareAccepted(
            fixture, {0.25, 0.25, z});
        const auto walk =
            locateDeterministicWalk(
                fixture.slots,
                fixture.sites,
                {0.25, 0.25, z});
        require(
            walk.location->kind ==
                (z > 0.0
                     ? Kind::Cell
                     : Kind::OutsideConvexHull),
            "one-ulp side collapsed into exact-zero boundary");
    }
}

void testCanonicalCrossingTieAndLocalFacePermutation() {
    auto fixture = bootstrap();
    const Vec3 query{-1.0, -1.0, -1.0};

    std::size_t finiteSlot =
        fixture.slots.size();
    for (std::size_t i = 0U;
         i < fixture.slots.size(); ++i) {
        if (fixture.slots[i].live &&
            finiteCell(fixture.slots[i].record)) {
            finiteSlot = i;
            break;
        }
    }
    require(
        finiteSlot < fixture.slots.size(),
        "tie fixture needs finite cell");

    std::map<PointId, Vec3> points;
    for (const auto& site : fixture.sites) {
        points.emplace(site.id, site.point);
    }

    std::vector<DelaunayFaceKey> violated;
    for (std::size_t localFace = 0U;
         localFace < 4U; ++localFace) {
        const auto outward =
            outwardFiniteFace(
                fixture.slots[finiteSlot].record,
                localFace);
        if (predicates::orient3d(
                points.at(outward[0]),
                points.at(outward[1]),
                points.at(outward[2]),
                query).sign ==
            PredicateSign::Positive) {
            violated.push_back(
                canonicalDelaunayFaceKey(
                    fixture.slots[finiteSlot].record,
                    localFace));
        }
    }
    require(
        violated.size() >= 2U,
        "tie fixture must expose multiple strict violated facets");
    const DelaunayFaceKey expected =
        *std::min_element(
            violated.begin(), violated.end());

    const auto walk =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            query);
    require(
        walk.ok() &&
            !walk.trace.crossedFacets.empty() &&
            walk.trace.crossedFacets.front() ==
                expected,
        "walk did not choose canonical minimum violated facet");

    auto permuted = fixture;
    auto& finite = permuted.slots[finiteSlot].record;
    const auto oldVertices = finite.vertices;
    const auto oldNeighbors = finite.neighbors;
    constexpr std::array<std::size_t, 4> permutation{
        1U, 2U, 0U, 3U}; // even 3-cycle: positive orientation preserved
    for (std::size_t i = 0U; i < 4U; ++i) {
        finite.vertices[i] =
            oldVertices[permutation[i]];
        finite.neighbors[i] =
            oldNeighbors[permutation[i]];
    }
    require(
        validateDelaunayTopology(
            permuted.slots,
            permuted.sites).ok(),
        "local-face permutation must preserve valid topology");

    const auto permutedWalk =
        locateDeterministicWalk(
            permuted.slots,
            permuted.sites,
            query);
    require(
        permutedWalk.ok() &&
            !permutedWalk.trace.crossedFacets.empty() &&
            permutedWalk.trace.crossedFacets.front() ==
                expected,
        "local-face permutation changed canonical crossing tie");
}

void testTwoCellGridStartAndStorageDeterminism() {
    auto fixture = twoCells();
    std::vector<std::size_t> reverse(
        fixture.slots.size());
    for (std::size_t i = 0U;
         i < reverse.size(); ++i) {
        reverse[i] = reverse.size() - 1U - i;
    }
    auto permuted =
        permuteSlots(fixture, reverse);
    std::reverse(
        permuted.sites.begin(),
        permuted.sites.end());

    const auto before =
        exactFingerprint(fixture.slots);
    const auto permutedBefore =
        exactFingerprint(permuted.slots);

    for (int i = -1; i <= 5; ++i) {
        for (int j = -1; j <= 5; ++j) {
            for (int k = -5; k <= 5; ++k) {
                const Vec3 query{
                    i / 4.0,
                    j / 4.0,
                    k / 4.0};
                compareAccepted(fixture, query);
                compareAccepted(permuted, query);

                const auto walk =
                    locateDeterministicWalk(
                        fixture.slots,
                        fixture.sites,
                        query);
                const auto permutedWalk =
                    locateDeterministicWalk(
                        permuted.slots,
                        permuted.sites,
                        query);

                require(
                    walk.trace.startCell.has_value() &&
                        permutedWalk.trace.startCell.has_value() &&
                        walk.trace.startCell
                                ->canonicalVertices ==
                            permutedWalk.trace.startCell
                                ->canonicalVertices,
                    "slot permutation changed canonical default start identity");
                require(
                    semanticAgreement(
                        *walk.location,
                        *permutedWalk.location),
                    "slot permutation changed walk semantic result");
                if (walk.location->kind ==
                    Kind::OutsideConvexHull) {
                    require(
                        walk.location->outsideWitness
                                ->canonicalFace ==
                            permutedWalk.location
                                ->outsideWitness
                                ->canonicalFace,
                        "slot permutation changed deterministic crossed hull witness");
                }
            }
        }
    }

    require(
        exactFingerprint(fixture.slots) == before &&
            exactFingerprint(permuted.slots) ==
                permutedBefore,
        "walk mutated topology/state during grid qualification");

    const DelaunayCellHandle upper =
        finiteHandleContaining(fixture, 900U);
    const DelaunayCellHandle lower =
        finiteHandleContaining(fixture, 700U);

    const Vec3 upperQuery{0.125, 0.125, 0.25};
    const auto fromUpper =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            upperQuery,
            upper);
    const auto fromLower =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            upperQuery,
            lower);
    require(
        fromUpper.ok() && fromLower.ok() &&
            semanticAgreement(
                *fromUpper.location,
                *fromLower.location),
        "different valid start hints changed semantic location");

    const auto sharedFacet =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.25, 0.25, 0.0},
            lower);
    require(
        sharedFacet.ok() &&
            sharedFacet.location->kind == Kind::Facet &&
            sharedFacet.trace.crossedFacets.empty(),
        "exact shared facet was crossed as strict violation");

    const auto hullEdge =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.5, 0.0, 0.0});
    require(
        hullEdge.ok() &&
            hullEdge.location->kind == Kind::Edge &&
            hullEdge.location->incidentCells.size() == 4U,
        "hull edge lost unified finite/ghost star evidence");
}

void testInvalidHintTopologyAndStallControls() {
    auto fixture = twoCells();
    const auto before =
        exactFingerprint(fixture.slots);

    const DelaunayCellHandle valid =
        finiteHandleContaining(fixture, 700U);
    const DelaunayCellHandle stale{
        valid.slot,
        static_cast<std::uint32_t>(
            valid.generation + 1U)};
    const auto staleResult =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.125, 0.125, 0.25},
            stale);
    require(
        staleResult.status ==
            DelaunayWalkStatus::InvalidStartHint &&
            !staleResult.location.has_value(),
        "stale start hint must be typed failure without fallback");

    auto corrupt = fixture;
    corrupt.slots[valid.slot]
        .record.neighbors[0].generation++;
    const auto invalidTopology =
        locateDeterministicWalk(
            corrupt.slots,
            corrupt.sites,
            {0.125, 0.125, 0.25});
    require(
        invalidTopology.status ==
            DelaunayWalkStatus::InvalidTopology,
        "invalid reciprocity/stale topology must be rejected before walk");

    DelaunayWalkTelemetry negativeTelemetry;
    detail::setDelaunayWalkStepGuardOverrideForQualification(0U);
    const auto stalled =
        locateDeterministicWalk(
            fixture.slots,
            fixture.sites,
            {0.125, 0.125, 0.25},
            valid,
            &negativeTelemetry);
    detail::setDelaunayWalkStepGuardOverrideForQualification(
        std::nullopt);

    require(
        stalled.status ==
            DelaunayWalkStatus::WalkStalled &&
            !stalled.location.has_value() &&
            stalled.diagnosticOracle.has_value() &&
            stalled.diagnosticOracle->kind ==
                Kind::Cell,
        "step-guard negative control must remain WalkStalled with separate P1C diagnosis");
    require(
        negativeTelemetry.walkStalls == 1U &&
            negativeTelemetry.diagnosticBruteForceCalls ==
                1U,
        "WalkStalled diagnostic telemetry mismatch");

    require(
        exactFingerprint(fixture.slots) == before,
        "negative controls mutated P1E topology snapshot");
}

void testRangeAndThinExactNonzero() {
    for (int exponent : {-500, 0, 500}) {
        const double scale =
            std::ldexp(1.0, exponent);
        auto fixture =
            bootstrap(scale, scale);
        compareAccepted(
            fixture,
            {scale / 8.0,
             scale / 8.0,
             scale / 8.0});
        compareAccepted(
            fixture,
            {scale / 4.0,
             scale / 4.0,
             0.0});
    }

    const double height =
        std::ldexp(1.0, -1000);
    auto thin = bootstrap(1.0, height);
    compareAccepted(
        thin,
        {0.125, 0.125, height / 4.0});
}

} // namespace

int main() {
    try {
        testClassificationAndOracleAgreement();
        testExactZeroAndOneUlp();
        testCanonicalCrossingTieAndLocalFacePermutation();
        testTwoCellGridStartAndStorageDeterminism();
        testInvalidHintTopologyAndStallControls();
        testRangeAndThinExactNonzero();

        require(
            qualification.stalls == 0U &&
                qualification.mismatches == 0U,
            "accepted P1E corpus must have zero stalls/mismatches");
        require(
            qualification.walk.locateCalls ==
                    qualification.queries &&
                qualification.walk.walkSuccesses ==
                    qualification.queries &&
                qualification.walk.walkStalls == 0U,
            "accepted P1E telemetry does not partition qualification queries");
        require(
            std::all_of(
                qualification.kinds.begin(),
                qualification.kinds.end(),
                [](std::uint64_t count) {
                    return count > 0U;
                }),
            "P1E qualification did not cover all five location kinds");

        std::cout
            << "M2.1-E deterministic walk PASS"
            << " checks=" << checks
            << " queries=" << qualification.queries
            << " successes=" << qualification.successes
            << " stalls=" << qualification.stalls
            << " mismatches=" << qualification.mismatches
            << " steps=" << qualification.walk.walkSteps
            << " max_steps=" << qualification.walk.maxWalkSteps
            << " kinds="
            << qualification.kinds[0] << '/'
            << qualification.kinds[1] << '/'
            << qualification.kinds[2] << '/'
            << qualification.kinds[3] << '/'
            << qualification.kinds[4]
            << '\n';
        return 0;
    } catch (const std::exception& error) {
        detail::setDelaunayWalkStepGuardOverrideForQualification(
            std::nullopt);
        std::cerr
            << "M2.1-E deterministic walk FAIL: "
            << error.what() << '\n';
        return 1;
    }
}
