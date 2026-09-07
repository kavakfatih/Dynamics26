#include "DelaunayWalk.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

thread_local std::optional<std::size_t> stepGuardOverride;

enum class WalkCellKind : std::uint8_t {
    Invalid = 0,
    Finite,
    Ghost
};

WalkCellKind walkCellKind(const DelaunayCellRecord& cell) noexcept {
    std::size_t finiteCount = 0U;
    for (const auto& vertex : cell.vertices) {
        if (vertex.isFinite()) {
            ++finiteCount;
        }
    }
    if (finiteCount == 4U) {
        return WalkCellKind::Finite;
    }
    if (finiteCount == 3U &&
        cell.vertices[0].isInfinite() &&
        cell.vertices[1].isFinite() &&
        cell.vertices[2].isFinite() &&
        cell.vertices[3].isFinite()) {
        return WalkCellKind::Ghost;
    }
    return WalkCellKind::Invalid;
}

std::array<DelaunayVertexRef, 4> canonicalCellIdentity(
    const DelaunayCellRecord& cell) {
    auto identity = cell.vertices;
    std::sort(identity.begin(), identity.end());
    return identity;
}

LocatedCellEvidence evidence(
    DelaunayCellHandle handle,
    const DelaunayCellSlot& slot) {
    return {
        canonicalCellIdentity(slot.record),
        handle};
}

bool liveHandle(
    std::span<const DelaunayCellSlot> slots,
    DelaunayCellHandle handle) noexcept {
    return handle.isValid() &&
           handle.slot < slots.size() &&
           slots[handle.slot].live &&
           slots[handle.slot].generation == handle.generation;
}

std::map<PointId, geometry::Vec3> validatedPointMap(
    std::span<const CanonicalSite> sites) {
    std::map<PointId, geometry::Vec3> points;
    std::set<std::array<double, 3>> coordinates;
    for (const CanonicalSite& site : sites) {
        if (site.id == InvalidPointId ||
            !std::isfinite(site.point.x) ||
            !std::isfinite(site.point.y) ||
            !std::isfinite(site.point.z) ||
            !points.emplace(site.id, site.point).second ||
            !coordinates.insert(
                {site.point.x, site.point.y, site.point.z}).second) {
            throw std::invalid_argument(
                "P1E requires unique finite canonical sites");
        }
    }
    return points;
}

std::optional<std::size_t> matchingFace(
    const DelaunayCellRecord& cell,
    const DelaunayFaceKey& key) {
    for (std::size_t face = 0U; face < 4U; ++face) {
        if (canonicalDelaunayFaceKey(cell, face) == key) {
            return face;
        }
    }
    return std::nullopt;
}

bool cellContainsEntity(
    const DelaunayCellRecord& cell,
    const std::vector<PointId>& entity) {
    return std::all_of(
        entity.begin(), entity.end(),
        [&](PointId id) {
            return std::find(
                       cell.vertices.begin(),
                       cell.vertices.end(),
                       DelaunayVertexRef::finite(id)) !=
                   cell.vertices.end();
        });
}

bool gatherIncidentCells(
    std::span<const DelaunayCellSlot> slots,
    const std::vector<PointId>& entity,
    DelaunayCellHandle containing,
    std::vector<LocatedCellEvidence>& incident) {
    std::vector<DelaunayCellHandle> queue{containing};
    std::set<std::pair<std::uint32_t, std::uint32_t>> visited;

    for (std::size_t cursor = 0U; cursor < queue.size(); ++cursor) {
        const DelaunayCellHandle current = queue[cursor];
        if (!liveHandle(slots, current)) {
            return false;
        }
        if (!visited.emplace(current.slot, current.generation).second) {
            continue;
        }

        const auto& slot = slots[current.slot];
        if (!cellContainsEntity(slot.record, entity)) {
            continue;
        }
        incident.push_back(evidence(current, slot));

        for (const DelaunayCellHandle neighbor :
             slot.record.neighbors) {
            if (!liveHandle(slots, neighbor)) {
                return false;
            }
            if (cellContainsEntity(
                    slots[neighbor.slot].record, entity)) {
                queue.push_back(neighbor);
            }
        }
    }

    std::sort(
        incident.begin(), incident.end(),
        [](const LocatedCellEvidence& lhs,
           const LocatedCellEvidence& rhs) {
            return lhs.canonicalVertices < rhs.canonicalVertices;
        });
    incident.erase(
        std::unique(
            incident.begin(), incident.end(),
            [](const LocatedCellEvidence& lhs,
               const LocatedCellEvidence& rhs) {
                return lhs.canonicalVertices ==
                       rhs.canonicalVertices;
            }),
        incident.end());
    return !incident.empty();
}

std::optional<DelaunayConflictSeed> finiteIncidentSeed(
    const std::vector<LocatedCellEvidence>& incident,
    std::span<const DelaunayCellSlot> slots,
    DelaunayConflictSeedSemantic semantic) {
    std::optional<LocatedCellEvidence> selected;
    for (const auto& item : incident) {
        if (!liveHandle(slots, item.handle) ||
            walkCellKind(slots[item.handle.slot].record) !=
                WalkCellKind::Finite) {
            continue;
        }
        if (!selected ||
            item.canonicalVertices < selected->canonicalVertices) {
            selected = item;
        }
    }
    if (!selected) {
        return std::nullopt;
    }
    return DelaunayConflictSeed{
        selected->handle,
        selected->canonicalVertices,
        semantic,
        std::nullopt};
}

std::optional<std::pair<DelaunayCellHandle, std::size_t>>
canonicalFiniteStart(std::span<const DelaunayCellSlot> slots) {
    std::optional<std::pair<
        std::array<DelaunayVertexRef, 4>,
        DelaunayCellHandle>> selected;

    std::size_t finiteCount = 0U;
    for (std::size_t slot = 0U; slot < slots.size(); ++slot) {
        const auto& cellSlot = slots[slot];
        if (!cellSlot.live ||
            walkCellKind(cellSlot.record) != WalkCellKind::Finite) {
            continue;
        }
        ++finiteCount;
        const auto identity =
            canonicalCellIdentity(cellSlot.record);
        const DelaunayCellHandle handle{
            static_cast<std::uint32_t>(slot),
            cellSlot.generation};
        if (!selected || identity < selected->first) {
            selected = std::make_pair(identity, handle);
        }
    }
    if (!selected) {
        return std::nullopt;
    }
    return std::make_pair(selected->second, finiteCount);
}

void recordStep(DelaunayWalkTelemetry* telemetry) noexcept {
    if (telemetry != nullptr) {
        ++telemetry->walkSteps;
    }
}

void recordSuccess(
    DelaunayWalkTelemetry* telemetry,
    std::size_t steps) noexcept {
    if (telemetry == nullptr) {
        return;
    }
    ++telemetry->walkSuccesses;
    telemetry->maxWalkSteps = std::max<std::uint64_t>(
        telemetry->maxWalkSteps,
        static_cast<std::uint64_t>(steps));
}

DelaunayWalkResult stallWithDiagnostic(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites,
    const geometry::Vec3& query,
    DelaunayWalkTrace trace,
    DelaunayWalkTelemetry* telemetry,
    std::string detailText) {
    if (telemetry != nullptr) {
        ++telemetry->walkStalls;
        ++telemetry->diagnosticBruteForceCalls;
        telemetry->maxWalkSteps = std::max<std::uint64_t>(
            telemetry->maxWalkSteps,
            static_cast<std::uint64_t>(
                trace.crossedFacets.size()));
    }

    DelaunayWalkResult result;
    result.status = DelaunayWalkStatus::WalkStalled;
    result.trace = std::move(trace);
    result.detail = std::move(detailText);
    try {
        result.diagnosticOracle =
            locateBruteForceExact(slots, sites, query);
    } catch (const std::exception& error) {
        result.detail +=
            std::string("; diagnostic brute-force failed: ") +
            error.what();
    }
    return result;
}

DelaunayWalkResult simpleFailure(
    DelaunayWalkStatus status,
    std::string detailText) {
    DelaunayWalkResult result;
    result.status = status;
    result.detail = std::move(detailText);
    return result;
}

} // namespace

namespace detail {

void setDelaunayWalkStepGuardOverrideForQualification(
    std::optional<std::size_t> guard) noexcept {
    stepGuardOverride = guard;
}

} // namespace detail

DelaunayWalkResult locateDeterministicWalk(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites,
    const geometry::Vec3& query,
    std::optional<DelaunayCellHandle> startHint,
    DelaunayWalkTelemetry* telemetry) {
    if (telemetry != nullptr) {
        ++telemetry->locateCalls;
    }

    if (!std::isfinite(query.x) ||
        !std::isfinite(query.y) ||
        !std::isfinite(query.z) ||
        slots.size() >
            static_cast<std::size_t>(
                std::numeric_limits<std::uint32_t>::max())) {
        return simpleFailure(
            DelaunayWalkStatus::InvalidInput,
            "P1E requires a finite query within the 32-bit handle domain");
    }

    std::map<PointId, geometry::Vec3> points;
    try {
        points = validatedPointMap(sites);
    } catch (const std::exception& error) {
        return simpleFailure(
            DelaunayWalkStatus::InvalidInput,
            error.what());
    }

    try {
        if (!validateDelaunayTopology(slots, sites).ok()) {
            return simpleFailure(
                DelaunayWalkStatus::InvalidTopology,
                "P1E received invalid finite/ghost topology");
        }
    } catch (const std::exception& error) {
        return simpleFailure(
            DelaunayWalkStatus::InvalidTopology,
            error.what());
    }

    const auto defaultStart = canonicalFiniteStart(slots);
    if (!defaultStart) {
        return simpleFailure(
            DelaunayWalkStatus::InvalidTopology,
            "P1E requires a nonempty finite complex");
    }
    const std::size_t finiteCellCount = defaultStart->second;

    DelaunayCellHandle current = defaultStart->first;
    if (startHint.has_value()) {
        if (!liveHandle(slots, *startHint) ||
            walkCellKind(slots[startHint->slot].record) !=
                WalkCellKind::Finite) {
            return simpleFailure(
                DelaunayWalkStatus::InvalidStartHint,
                "P1E start hint is stale, dead, out of range or not finite");
        }
        current = *startHint;
    }

    DelaunayWalkTrace trace;
    trace.stepGuard =
        stepGuardOverride.value_or(finiteCellCount);
    trace.startCell = evidence(current, slots[current.slot]);

    std::set<std::pair<std::uint32_t, std::uint32_t>>
        visited;

    while (true) {
        if (!liveHandle(slots, current) ||
            walkCellKind(slots[current.slot].record) !=
                WalkCellKind::Finite) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E traversal reached a stale/non-finite current cell");
        }
        if (!visited.emplace(
                current.slot, current.generation).second) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E traversal repeated a finite cell");
        }

        const DelaunayCellRecord& cell =
            slots[current.slot].record;
        trace.visitedCells.push_back(
            evidence(current, slots[current.slot]));

        struct Crossing {
            DelaunayFaceKey key;
            std::size_t localFace{0U};
            std::array<PointId, 3> outward{};
        };
        std::vector<Crossing> violated;
        std::array<PredicateSign, 4> faceSigns{};

        for (std::size_t localFace = 0U;
             localFace < 4U; ++localFace) {
            const auto outward =
                outwardFiniteFace(cell, localFace);
            const PredicateSign sign =
                predicates::orient3d(
                    points.at(outward[0]),
                    points.at(outward[1]),
                    points.at(outward[2]),
                    query).sign;
            faceSigns[localFace] = sign;
            // Exact zero spatial boundary bilgisidir; progress direction
            // degildir. Yalniz strict exterior Positive crossing adayidir.
            if (sign == PredicateSign::Positive) {
                violated.push_back({
                    canonicalDelaunayFaceKey(
                        cell, localFace),
                    localFace,
                    outward});
            }
        }

        if (violated.empty()) {
            DelaunayLocationResult location;
            for (std::size_t vertex = 0U;
                 vertex < 4U; ++vertex) {
                if (faceSigns[vertex] ==
                    PredicateSign::Negative) {
                    location.entityVertices.push_back(
                        *cell.vertices[vertex].finitePointId());
                }
            }
            std::sort(
                location.entityVertices.begin(),
                location.entityVertices.end());

            switch (location.entityVertices.size()) {
            case 4U:
                location.kind = DelaunayLocationKind::Cell;
                break;
            case 3U:
                location.kind = DelaunayLocationKind::Facet;
                break;
            case 2U:
                location.kind = DelaunayLocationKind::Edge;
                break;
            case 1U:
                location.kind = DelaunayLocationKind::Vertex;
                break;
            default:
                return stallWithDiagnostic(
                    slots, sites, query, std::move(trace),
                    telemetry,
                    "P1E contained cell produced an invalid exact boundary dimension");
            }

            if (!gatherIncidentCells(
                    slots,
                    location.entityVertices,
                    current,
                    location.incidentCells)) {
                return stallWithDiagnostic(
                    slots, sites, query, std::move(trace),
                    telemetry,
                    "P1E could not gather a reciprocal incident star");
            }

            DelaunayWalkResult result;
            result.status = DelaunayWalkStatus::Success;
            result.location = std::move(location);
            result.trace = std::move(trace);

            switch (result.location->kind) {
            case DelaunayLocationKind::Cell:
                result.conflictSeed =
                    finiteIncidentSeed(
                        result.location->incidentCells,
                        slots,
                        DelaunayConflictSeedSemantic::CellInterior);
                break;
            case DelaunayLocationKind::Facet:
                result.conflictSeed =
                    finiteIncidentSeed(
                        result.location->incidentCells,
                        slots,
                        DelaunayConflictSeedSemantic::FacetIncident);
                break;
            case DelaunayLocationKind::Edge:
                result.conflictSeed =
                    finiteIncidentSeed(
                        result.location->incidentCells,
                        slots,
                        DelaunayConflictSeedSemantic::EdgeIncident);
                break;
            case DelaunayLocationKind::Vertex:
                // Existing vertex is duplicate/already-inserted semantics;
                // cavity seed uretmek frozen contract'a aykiridir.
                result.conflictSeed.reset();
                break;
            case DelaunayLocationKind::OutsideConvexHull:
                break;
            }

            if (result.location->kind !=
                    DelaunayLocationKind::Vertex &&
                !result.conflictSeed.has_value()) {
                return stallWithDiagnostic(
                    slots, sites, query,
                    std::move(result.trace),
                    telemetry,
                    "P1E contained entity has no verified finite conflict seed");
            }

            recordSuccess(
                telemetry,
                result.trace.crossedFacets.size());
            return result;
        }

        // local face index veya slot sirasi tie authority degildir.
        const auto chosen = std::min_element(
            violated.begin(), violated.end(),
            [](const Crossing& lhs, const Crossing& rhs) {
                return lhs.key < rhs.key;
            });

        if (trace.crossedFacets.size() >=
            trace.stepGuard) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E topology-sized step guard exceeded");
        }

        const DelaunayCellHandle neighbor =
            cell.neighbors[chosen->localFace];
        if (!liveHandle(slots, neighbor)) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E violated facet points to a stale neighbor");
        }

        const DelaunayCellRecord& neighborCell =
            slots[neighbor.slot].record;
        const auto reciprocalFace =
            matchingFace(neighborCell, chosen->key);
        if (!reciprocalFace.has_value() ||
            neighborCell.neighbors[*reciprocalFace] != current) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E violated facet neighbor is not reciprocal");
        }

        trace.crossedFacets.push_back(chosen->key);
        recordStep(telemetry);

        if (walkCellKind(neighborCell) ==
            WalkCellKind::Finite) {
            current = neighbor;
            continue;
        }

        if (walkCellKind(neighborCell) !=
                WalkCellKind::Ghost ||
            *reciprocalFace != 0U ||
            canonicalDelaunayFaceKey(
                neighborCell, 0U) != chosen->key) {
            return stallWithDiagnostic(
                slots, sites, query, std::move(trace),
                telemetry,
                "P1E hull crossing did not enter the paired ghost finite face");
        }

        std::array<PointId, 3> canonicalFace{
            chosen->outward[0],
            chosen->outward[1],
            chosen->outward[2]};
        std::sort(
            canonicalFace.begin(),
            canonicalFace.end());

        DelaunayLocationResult location;
        location.kind =
            DelaunayLocationKind::OutsideConvexHull;
        location.outsideWitness =
            ViolatedHullWitness{
                canonicalFace,
                chosen->outward,
                current,
                neighbor};

        DelaunayWalkResult result;
        result.status = DelaunayWalkStatus::Success;
        result.location = std::move(location);
        result.trace = std::move(trace);
        result.conflictSeed =
            DelaunayConflictSeed{
                neighbor,
                canonicalCellIdentity(neighborCell),
                DelaunayConflictSeedSemantic::OutsideGhost,
                chosen->key};

        recordSuccess(
            telemetry,
            result.trace.crossedFacets.size());
        return result;
    }
}

} // namespace femcae::meshing::m2
