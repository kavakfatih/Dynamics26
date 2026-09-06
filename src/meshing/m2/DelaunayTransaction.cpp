#include "DelaunayTransaction.h"

#include "DelaunayPredicates.h"
#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <limits>
#include <map>
#include <new>
#include <set>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

thread_local detail::DelaunayCommitBarrierAuditHooks
    commitBarrierAuditHooks{};

struct EdgeKey {
    std::array<DelaunayVertexRef, 2> vertices{};

    friend bool operator<(const EdgeKey& lhs, const EdgeKey& rhs) noexcept {
        if (lhs.vertices[0] < rhs.vertices[0]) return true;
        if (rhs.vertices[0] < lhs.vertices[0]) return false;
        return lhs.vertices[1] < rhs.vertices[1];
    }
};

struct CavityIncidence {
    DelaunayCellHandle cell;
    std::uint8_t localFace{0};
};

struct CandidateFaceOwner {
    std::size_t candidate{0};
    std::uint8_t localFace{0};
};

bool finiteCell(const DelaunayCellRecord& cell) noexcept {
    return std::all_of(
        cell.vertices.begin(), cell.vertices.end(),
        [](const DelaunayVertexRef& vertex) { return vertex.isFinite(); });
}

bool ghostCell(const DelaunayCellRecord& cell) noexcept {
    return cell.vertices[0].isInfinite() &&
           cell.vertices[1].isFinite() &&
           cell.vertices[2].isFinite() &&
           cell.vertices[3].isFinite();
}

bool samePoint(
    const geometry::Vec3& lhs,
    const geometry::Vec3& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

std::array<DelaunayVertexRef, 4> canonicalCellKey(
    const DelaunayCellRecord& cell) {
    auto key = cell.vertices;
    std::sort(key.begin(), key.end());
    return key;
}

std::array<DelaunayVertexRef, 3> localFaceVertices(
    const DelaunayCellRecord& cell,
    std::size_t localFace) {
    if (localFace >= 4U) {
        throw std::out_of_range("M2 P1D local face index must be in [0,3]");
    }
    std::array<DelaunayVertexRef, 3> face{};
    std::size_t write = 0U;
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        if (vertex != localFace) {
            face[write++] = cell.vertices[vertex];
        }
    }
    return face;
}

std::optional<std::size_t> matchingFace(
    const DelaunayCellRecord& cell,
    const DelaunayFaceKey& key) {
    std::optional<std::size_t> result;
    for (std::size_t face = 0U; face < 4U; ++face) {
        if (canonicalDelaunayFaceKey(cell, face) != key) {
            continue;
        }
        if (result.has_value()) {
            return std::nullopt;
        }
        result = face;
    }
    return result;
}

bool liveHandle(
    std::span<const DelaunayCellSlot> slots,
    DelaunayCellHandle handle) noexcept {
    return handle.isValid() &&
           handle.slot < slots.size() &&
           slots[handle.slot].live &&
           slots[handle.slot].generation == handle.generation;
}

DelaunayCellHandle handleFor(
    std::size_t slot,
    const DelaunayCellSlot& record) {
    if (slot > static_cast<std::size_t>(
                   std::numeric_limits<std::uint32_t>::max())) {
        throw std::length_error("M2 P1D handle domain overflow");
    }
    return {
        static_cast<std::uint32_t>(slot),
        record.generation};
}

std::map<PointId, geometry::Vec3> validatedPoints(
    std::span<const CanonicalSite> sites) {
    std::map<PointId, geometry::Vec3> points;
    std::set<std::array<double, 3>> coordinates;
    for (const CanonicalSite& site : sites) {
        if (site.id == InvalidPointId ||
            !std::isfinite(site.point.x) ||
            !std::isfinite(site.point.y) ||
            !std::isfinite(site.point.z)) {
            throw std::invalid_argument(
                "M2 P1D requires finite canonical sites with non-zero PointId");
        }
        if (!points.emplace(site.id, site.point).second) {
            throw std::invalid_argument("M2 P1D received duplicate PointId");
        }
        if (!coordinates.insert(
                {site.point.x, site.point.y, site.point.z}).second) {
            throw std::invalid_argument(
                "M2 P1D expects M1-canonicalized unique coordinates");
        }
    }
    return points;
}

std::uint64_t canonicalSiteCoordinateBits(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(
            "M2 P1D site snapshot requires finite coordinates");
    }
    std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
    // M1/D26SITE1 canonicalization normalizes both signed-zero encodings to +0.
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0ULL) {
        bits = 0ULL;
    }
    return bits;
}

std::vector<DelaunaySiteSnapshotEntry> exactSiteSnapshot(
    std::span<const CanonicalSite> sites) {
    std::vector<DelaunaySiteSnapshotEntry> snapshot;
    snapshot.reserve(sites.size());
    for (const CanonicalSite& site : sites) {
        if (site.id == InvalidPointId) {
            throw std::invalid_argument(
                "M2 P1D site snapshot requires non-zero PointId");
        }
        snapshot.push_back({
            site.id,
            canonicalSiteCoordinateBits(site.point.x),
            canonicalSiteCoordinateBits(site.point.y),
            canonicalSiteCoordinateBits(site.point.z)});
    }
    std::sort(
        snapshot.begin(), snapshot.end(),
        [](const DelaunaySiteSnapshotEntry& lhs,
           const DelaunaySiteSnapshotEntry& rhs) {
            return lhs.id < rhs.id;
        });
    return snapshot;
}

geometry::Vec3 pointFor(
    const std::map<PointId, geometry::Vec3>& points,
    PointId id) {
    const auto iterator = points.find(id);
    if (iterator == points.end()) {
        throw std::out_of_range("M2 P1D topology references missing PointId");
    }
    return iterator->second;
}

bool queryIsLive(
    std::span<const DelaunayCellSlot> slots,
    PointId queryId) noexcept {
    for (const DelaunayCellSlot& slot : slots) {
        if (!slot.live) continue;
        for (const DelaunayVertexRef& vertex : slot.record.vertices) {
            if (vertex.finitePointId() == queryId) {
                return true;
            }
        }
    }
    return false;
}

DelaunayConflict classifyCellConflict(
    std::span<const DelaunayCellSlot> slots,
    const std::map<PointId, geometry::Vec3>& points,
    DelaunayCellHandle handle,
    const IndexedPoint3& query) {
    if (!liveHandle(slots, handle)) {
        throw std::invalid_argument("M2 P1D conflict classification received stale handle");
    }
    const DelaunayCellRecord& cell = slots[handle.slot].record;

    if (finiteCell(cell)) {
        std::array<IndexedPoint3, 4> tetra{};
        for (std::size_t i = 0U; i < 4U; ++i) {
            const PointId id = *cell.vertices[i].finitePointId();
            tetra[i] = {id, pointFor(points, id)};
        }
        return classifyFiniteCellConflict(tetra, query);
    }

    if (!ghostCell(cell)) {
        throw std::invalid_argument("M2 P1D conflict classification received invalid ghost");
    }

    const DelaunayCellHandle finiteNeighbor = cell.neighbors[0];
    if (!liveHandle(slots, finiteNeighbor) ||
        !finiteCell(slots[finiteNeighbor.slot].record)) {
        throw std::invalid_argument("M2 P1D ghost has no current finite neighbor");
    }

    const DelaunayFaceKey hullKey = canonicalDelaunayFaceKey(cell, 0U);
    const DelaunayCellRecord& finite = slots[finiteNeighbor.slot].record;
    const auto finiteFace = matchingFace(finite, hullKey);
    if (!finiteFace.has_value()) {
        throw std::invalid_argument("M2 P1D ghost/finite hull face mismatch");
    }

    const auto witnessId = finite.vertices[*finiteFace].finitePointId();
    if (!witnessId.has_value()) {
        throw std::invalid_argument("M2 P1D ghost finite witness is not finite");
    }

    const std::array<IndexedPoint3, 3> outward{{
        {*cell.vertices[1].finitePointId(),
         pointFor(points, *cell.vertices[1].finitePointId())},
        {*cell.vertices[2].finitePointId(),
         pointFor(points, *cell.vertices[2].finitePointId())},
        {*cell.vertices[3].finitePointId(),
         pointFor(points, *cell.vertices[3].finitePointId())}}};

    return classifyGhostCellConflict(
        outward,
        {*witnessId, pointFor(points, *witnessId)},
        query);
}

void sortHandlesCanonically(
    std::vector<DelaunayCellHandle>& handles,
    std::span<const DelaunayCellSlot> slots) {
    std::sort(
        handles.begin(), handles.end(),
        [slots](DelaunayCellHandle lhs, DelaunayCellHandle rhs) {
            const auto lk = canonicalCellKey(slots[lhs.slot].record);
            const auto rk = canonicalCellKey(slots[rhs.slot].record);
            if (lk < rk) return true;
            if (rk < lk) return false;
            if (lhs.generation != rhs.generation) {
                return lhs.generation < rhs.generation;
            }
            return lhs.slot < rhs.slot;
        });
}

std::vector<DelaunayCellHandle> globalConflictOracle(
    std::span<const DelaunayCellSlot> slots,
    const std::map<PointId, geometry::Vec3>& points,
    const IndexedPoint3& query) {
    std::vector<DelaunayCellHandle> conflict;
    conflict.reserve(slots.size());
    for (std::size_t index = 0U; index < slots.size(); ++index) {
        const DelaunayCellSlot& slot = slots[index];
        if (!slot.live) continue;
        const DelaunayCellHandle handle = handleFor(index, slot);
        if (classifyCellConflict(slots, points, handle, query) ==
            DelaunayConflict::Conflict) {
            conflict.push_back(handle);
        }
    }
    sortHandlesCanonically(conflict, slots);
    return conflict;
}

std::vector<DelaunayCellHandle> adjacencyFlood(
    std::span<const DelaunayCellSlot> slots,
    const std::map<PointId, geometry::Vec3>& points,
    const IndexedPoint3& query,
    DelaunayCellHandle verifiedSeed) {
    if (classifyCellConflict(slots, points, verifiedSeed, query) !=
        DelaunayConflict::Conflict) {
        throw std::logic_error("M2 P1D flood seed is not exact-conflicting");
    }

    std::vector<bool> visited(slots.size(), false);
    std::vector<DelaunayCellHandle> stack{verifiedSeed};
    std::vector<DelaunayCellHandle> conflict;

    while (!stack.empty()) {
        const DelaunayCellHandle current = stack.back();
        stack.pop_back();
        if (!liveHandle(slots, current) || visited[current.slot]) {
            continue;
        }
        visited[current.slot] = true;

        if (classifyCellConflict(slots, points, current, query) !=
            DelaunayConflict::Conflict) {
            continue;
        }
        conflict.push_back(current);

        std::vector<DelaunayCellHandle> neighbors;
        neighbors.reserve(4U);
        for (const DelaunayCellHandle neighbor :
             slots[current.slot].record.neighbors) {
            if (liveHandle(slots, neighbor) && !visited[neighbor.slot]) {
                neighbors.push_back(neighbor);
            }
        }
        sortHandlesCanonically(neighbors, slots);
        // Stack LIFO oldugu icin canonical en kucuk komsu once ziyaret edilsin.
        for (auto iterator = neighbors.rbegin();
             iterator != neighbors.rend(); ++iterator) {
            stack.push_back(*iterator);
        }
    }

    sortHandlesCanonically(conflict, slots);
    return conflict;
}

bool sameHandleSequence(
    const std::vector<DelaunayCellHandle>& lhs,
    const std::vector<DelaunayCellHandle>& rhs) noexcept {
    return lhs == rhs;
}

std::optional<std::uint8_t> reciprocalOutsideFace(
    std::span<const DelaunayCellSlot> slots,
    DelaunayCellHandle inside,
    std::uint8_t insideFace,
    DelaunayCellHandle outside,
    const DelaunayFaceKey& key) {
    if (!liveHandle(slots, outside)) {
        return std::nullopt;
    }
    const auto outsideFace = matchingFace(slots[outside.slot].record, key);
    if (!outsideFace.has_value() ||
        *outsideFace > std::numeric_limits<std::uint8_t>::max()) {
        return std::nullopt;
    }
    if (slots[outside.slot].record.neighbors[*outsideFace] != inside ||
        slots[inside.slot].record.neighbors[insideFace] != outside) {
        return std::nullopt;
    }
    return static_cast<std::uint8_t>(*outsideFace);
}

DelaunayTransactionFailure extractCavity(
    std::span<const DelaunayCellSlot> slots,
    const std::vector<DelaunayCellHandle>& cavity,
    DelaunayInsertionPlan& plan,
    std::string& detail) {
    std::vector<bool> inCavity(slots.size(), false);
    for (const DelaunayCellHandle handle : cavity) {
        if (!liveHandle(slots, handle) || inCavity[handle.slot]) {
            detail = "P1D cavity contains stale or duplicate cell handle";
            return DelaunayTransactionFailure::InvalidTopology;
        }
        inCavity[handle.slot] = true;
    }

    std::map<DelaunayFaceKey, std::vector<CavityIncidence>> faces;
    for (const DelaunayCellHandle handle : cavity) {
        const DelaunayCellRecord& cell = slots[handle.slot].record;
        for (std::size_t face = 0U; face < 4U; ++face) {
            faces[canonicalDelaunayFaceKey(cell, face)].push_back(
                {handle, static_cast<std::uint8_t>(face)});
        }
    }

    plan.internalFacets.clear();
    plan.boundaryFacets.clear();

    for (const auto& [key, incidences] : faces) {
        if (incidences.size() > 2U || incidences.empty()) {
            detail = "P1D cavity face incidence is not one or two";
            return DelaunayTransactionFailure::InvalidFaceMultiplicity;
        }

        if (incidences.size() == 2U) {
            const CavityIncidence a = incidences[0];
            const CavityIncidence b = incidences[1];
            if (slots[a.cell.slot].record.neighbors[a.localFace] != b.cell ||
                slots[b.cell.slot].record.neighbors[b.localFace] != a.cell) {
                detail = "P1D internal cavity face is not reciprocal";
                return DelaunayTransactionFailure::InvalidFaceMultiplicity;
            }
            plan.internalFacets.push_back({
                key,
                a.cell,
                a.localFace,
                b.cell,
                b.localFace});
            continue;
        }

        const CavityIncidence inside = incidences.front();
        const DelaunayCellHandle outside =
            slots[inside.cell.slot].record.neighbors[inside.localFace];
        if (!liveHandle(slots, outside) || inCavity[outside.slot]) {
            detail = "P1D boundary face has no unique surviving outside cell";
            return DelaunayTransactionFailure::InvalidFaceMultiplicity;
        }
        const auto outsideFace = reciprocalOutsideFace(
            slots,
            inside.cell,
            inside.localFace,
            outside,
            key);
        if (!outsideFace.has_value()) {
            detail = "P1D boundary face outside relation is not reciprocal";
            return DelaunayTransactionFailure::InvalidFaceMultiplicity;
        }

        const auto oriented =
            localFaceVertices(slots[inside.cell.slot].record, inside.localFace);
        const bool finiteFacet = std::all_of(
            oriented.begin(), oriented.end(),
            [](const DelaunayVertexRef& vertex) {
                return vertex.isFinite();
            });

        plan.boundaryFacets.push_back({
            key,
            oriented,
            inside.cell,
            inside.localFace,
            outside,
            *outsideFace,
            finiteFacet});
    }

    std::sort(
        plan.internalFacets.begin(), plan.internalFacets.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });
    std::sort(
        plan.boundaryFacets.begin(), plan.boundaryFacets.end(),
        [](const auto& lhs, const auto& rhs) { return lhs.key < rhs.key; });

    const std::size_t c = cavity.size();
    const std::size_t i = plan.internalFacets.size();
    const std::size_t b = plan.boundaryFacets.size();
    if (c > (std::numeric_limits<std::size_t>::max() / 4U) ||
        i > (std::numeric_limits<std::size_t>::max() / 2U) ||
        4U * c != 2U * i + b) {
        detail = "P1D cavity violates 4C=2I+B";
        return DelaunayTransactionFailure::InvalidFaceMultiplicity;
    }

    std::set<DelaunayVertexRef> vertices;
    std::map<EdgeKey, std::vector<std::size_t>> edgeOwners;
    for (std::size_t faceIndex = 0U;
         faceIndex < plan.boundaryFacets.size(); ++faceIndex) {
        const auto& face = plan.boundaryFacets[faceIndex].key.vertices;
        for (const DelaunayVertexRef& vertex : face) {
            vertices.insert(vertex);
        }
        for (std::size_t a = 0U; a < 3U; ++a) {
            for (std::size_t d = a + 1U; d < 3U; ++d) {
                EdgeKey edge{{face[a], face[d]}};
                if (edge.vertices[1] < edge.vertices[0]) {
                    std::swap(edge.vertices[0], edge.vertices[1]);
                }
                edgeOwners[edge].push_back(faceIndex);
            }
        }
    }
    for (const auto& [edge, owners] : edgeOwners) {
        (void)edge;
        if (owners.size() != 2U) {
            detail = "P1D cavity boundary edge incidence is not two";
            return DelaunayTransactionFailure::InvalidBoundaryManifold;
        }
    }

    if (b == 0U || (b % 2U) != 0U) {
        detail = "P1D cavity boundary is empty or has odd triangle count";
        return DelaunayTransactionFailure::InvalidBoundaryManifold;
    }
    const std::int64_t euler =
        static_cast<std::int64_t>(vertices.size()) -
        static_cast<std::int64_t>(edgeOwners.size()) +
        static_cast<std::int64_t>(b);
    if (euler != 2) {
        detail = "P1D cavity boundary Euler characteristic is not two";
        return DelaunayTransactionFailure::InvalidBoundaryManifold;
    }

    std::vector<std::vector<std::size_t>> adjacency(b);
    for (const auto& [edge, owners] : edgeOwners) {
        (void)edge;
        adjacency[owners[0]].push_back(owners[1]);
        adjacency[owners[1]].push_back(owners[0]);
    }
    std::vector<bool> visited(b, false);
    std::vector<std::size_t> stack{0U};
    std::size_t visitedCount = 0U;
    while (!stack.empty()) {
        const std::size_t current = stack.back();
        stack.pop_back();
        if (visited[current]) continue;
        visited[current] = true;
        ++visitedCount;
        for (std::size_t next : adjacency[current]) {
            if (!visited[next]) stack.push_back(next);
        }
    }
    if (visitedCount != b) {
        detail = "P1D cavity boundary has more than one component";
        return DelaunayTransactionFailure::InvalidBoundaryManifold;
    }

    return DelaunayTransactionFailure::None;
}

std::optional<std::size_t> candidateMatchingFace(
    const DelaunayCellRecord& cell,
    const DelaunayFaceKey& key) {
    return matchingFace(cell, key);
}

DelaunayTransactionFailure buildCandidatePatch(
    std::span<const DelaunayCellSlot> slots,
    const std::map<PointId, geometry::Vec3>& points,
    DelaunayInsertionPlan& plan,
    std::string& detail) {
    const auto slotCheck = detail::checkedDelaunayRequiredSlots(
        slots.size(), plan.boundaryFacets.size());
    if (!slotCheck.ok()) {
        detail = "P1D candidate append exceeds the 32-bit handle domain";
        return slotCheck.failure;
    }
    plan.requiredSlotCount = slotCheck.required;
    plan.candidateCells.clear();
    plan.candidateCells.reserve(plan.boundaryFacets.size());

    for (std::size_t index = 0U;
         index < plan.boundaryFacets.size(); ++index) {
        const DelaunayBoundaryFacetRecord& boundary =
            plan.boundaryFacets[index];

        DelaunayCandidateCell candidate;
        candidate.baseFace = boundary.key;
        candidate.outsideCell = boundary.outsideCell;
        candidate.outsideLocalFace = boundary.outsideLocalFace;
        candidate.futureHandle = {
            static_cast<std::uint32_t>(slots.size() + index),
            1U};

        if (boundary.finiteFacet) {
            std::array<PointId, 3> ids{};
            for (std::size_t i = 0U; i < 3U; ++i) {
                const auto id = boundary.orientedVertices[i].finitePointId();
                if (!id.has_value()) {
                    detail = "P1D finite boundary facet contains Infinite";
                    return DelaunayTransactionFailure::InvalidStitching;
                }
                ids[i] = *id;
            }

            PredicateSign orientation = predicates::orient3d(
                pointFor(points, ids[0]),
                pointFor(points, ids[1]),
                pointFor(points, ids[2]),
                plan.queryPoint).sign;
            if (orientation == PredicateSign::Zero) {
                detail = "P1D finite boundary cone is exactly degenerate";
                return DelaunayTransactionFailure::DegenerateCandidate;
            }
            if (orientation == PredicateSign::Negative) {
                std::swap(ids[0], ids[1]);
            }

            candidate.record.vertices[0] = DelaunayVertexRef::finite(ids[0]);
            candidate.record.vertices[1] = DelaunayVertexRef::finite(ids[1]);
            candidate.record.vertices[2] = DelaunayVertexRef::finite(ids[2]);
            candidate.record.vertices[3] =
                DelaunayVertexRef::finite(plan.queryId);
        } else {
            std::array<PointId, 2> finite{};
            std::size_t count = 0U;
            for (const DelaunayVertexRef& vertex :
                 boundary.orientedVertices) {
                if (const auto id = vertex.finitePointId()) {
                    if (count >= finite.size()) {
                        detail = "P1D Infinite boundary has wrong finite multiplicity";
                        return DelaunayTransactionFailure::InvalidStitching;
                    }
                    finite[count++] = *id;
                }
            }
            if (count != 2U) {
                detail = "P1D Infinite boundary must contain exactly two finite vertices";
                return DelaunayTransactionFailure::InvalidStitching;
            }

            candidate.record.vertices[0] = DelaunayVertexRef::infinite();
            candidate.record.vertices[1] = DelaunayVertexRef::finite(finite[0]);
            candidate.record.vertices[2] = DelaunayVertexRef::finite(finite[1]);
            candidate.record.vertices[3] =
                DelaunayVertexRef::finite(plan.queryId);
        }
        plan.candidateCells.push_back(std::move(candidate));
    }

    // Ghost finite face-0 orientation is fixed only after its new finite
    // neighbor across {a,b,query} is known.
    for (std::size_t ghostIndex = 0U;
         ghostIndex < plan.candidateCells.size(); ++ghostIndex) {
        DelaunayCandidateCell& ghost = plan.candidateCells[ghostIndex];
        if (!ghostCell(ghost.record)) continue;

        const DelaunayFaceKey finiteFace =
            canonicalDelaunayFaceKey(ghost.record, 0U);
        std::optional<CandidateFaceOwner> finiteOwner;
        std::size_t ownerCount = 0U;
        for (std::size_t candidateIndex = 0U;
             candidateIndex < plan.candidateCells.size(); ++candidateIndex) {
            const DelaunayCellRecord& record =
                plan.candidateCells[candidateIndex].record;
            for (std::size_t face = 0U; face < 4U; ++face) {
                if (canonicalDelaunayFaceKey(record, face) != finiteFace) {
                    continue;
                }
                ++ownerCount;
                if (candidateIndex != ghostIndex && finiteCell(record)) {
                    finiteOwner = CandidateFaceOwner{
                        candidateIndex,
                        static_cast<std::uint8_t>(face)};
                }
            }
        }
        if (ownerCount != 2U || !finiteOwner.has_value()) {
            detail = "P1D ghost finite face does not have exactly one new finite neighbor";
            return DelaunayTransactionFailure::InvalidGhostOrientation;
        }

        const DelaunayCellRecord& finite =
            plan.candidateCells[finiteOwner->candidate].record;
        const auto witnessId =
            finite.vertices[finiteOwner->localFace].finitePointId();
        if (!witnessId.has_value()) {
            detail = "P1D ghost orientation witness is not finite";
            return DelaunayTransactionFailure::InvalidGhostOrientation;
        }

        const PointId a = *ghost.record.vertices[1].finitePointId();
        const PointId b = *ghost.record.vertices[2].finitePointId();
        const PointId c = *ghost.record.vertices[3].finitePointId();
        const PredicateSign sign = predicates::orient3d(
            pointFor(points, a),
            pointFor(points, b),
            pointFor(points, c),
            pointFor(points, *witnessId)).sign;
        if (sign == PredicateSign::Zero) {
            detail = "P1D ghost hull orientation witness is coplanar";
            return DelaunayTransactionFailure::InvalidGhostOrientation;
        }
        if (sign == PredicateSign::Positive) {
            std::swap(ghost.record.vertices[1], ghost.record.vertices[2]);
        }
    }

    std::map<DelaunayFaceKey, std::vector<CandidateFaceOwner>> faceOwners;
    for (std::size_t candidateIndex = 0U;
         candidateIndex < plan.candidateCells.size(); ++candidateIndex) {
        DelaunayCandidateCell& candidate =
            plan.candidateCells[candidateIndex];
        const auto baseFace =
            candidateMatchingFace(candidate.record, candidate.baseFace);
        if (!baseFace.has_value()) {
            detail = "P1D candidate does not contain its cavity base face";
            return DelaunayTransactionFailure::InvalidStitching;
        }
        candidate.baseLocalFace =
            static_cast<std::uint8_t>(*baseFace);

        for (std::size_t face = 0U; face < 4U; ++face) {
            faceOwners[canonicalDelaunayFaceKey(
                candidate.record, face)].push_back({
                    candidateIndex,
                    static_cast<std::uint8_t>(face)});
        }
    }

    std::map<DelaunayFaceKey, const DelaunayBoundaryFacetRecord*> boundaryByKey;
    for (const DelaunayBoundaryFacetRecord& boundary :
         plan.boundaryFacets) {
        if (!boundaryByKey.emplace(boundary.key, &boundary).second) {
            detail = "P1D boundary face key is duplicated";
            return DelaunayTransactionFailure::InvalidStitching;
        }
    }

    for (const auto& [key, owners] : faceOwners) {
        const auto boundary = boundaryByKey.find(key);
        if (boundary != boundaryByKey.end()) {
            if (owners.size() != 1U) {
                detail = "P1D base face has more than one candidate owner";
                return DelaunayTransactionFailure::InvalidStitching;
            }
            const CandidateFaceOwner owner = owners.front();
            DelaunayCandidateCell& candidate =
                plan.candidateCells[owner.candidate];
            if (candidate.baseFace != key ||
                owner.localFace != candidate.baseLocalFace) {
                detail = "P1D boundary key appears as a non-base candidate face";
                return DelaunayTransactionFailure::InvalidStitching;
            }
            candidate.record.neighbors[owner.localFace] =
                boundary->second->outsideCell;
            continue;
        }

        if (owners.size() != 2U ||
            owners[0].candidate == owners[1].candidate) {
            detail = "P1D new lateral face does not have exactly two owners";
            return DelaunayTransactionFailure::InvalidStitching;
        }
        const CandidateFaceOwner a = owners[0];
        const CandidateFaceOwner b = owners[1];
        plan.candidateCells[a.candidate].record.neighbors[a.localFace] =
            plan.candidateCells[b.candidate].futureHandle;
        plan.candidateCells[b.candidate].record.neighbors[b.localFace] =
            plan.candidateCells[a.candidate].futureHandle;
    }

    plan.externalRewires.clear();
    plan.externalRewires.reserve(plan.boundaryFacets.size());
    for (const DelaunayBoundaryFacetRecord& boundary :
         plan.boundaryFacets) {
        const auto candidate = std::find_if(
            plan.candidateCells.begin(),
            plan.candidateCells.end(),
            [&boundary](const DelaunayCandidateCell& value) {
                return value.baseFace == boundary.key;
            });
        if (candidate == plan.candidateCells.end()) {
            detail = "P1D boundary face has no replacement candidate";
            return DelaunayTransactionFailure::InvalidStitching;
        }
        plan.externalRewires.push_back({
            boundary.outsideCell,
            boundary.outsideLocalFace,
            boundary.insideCell,
            candidate->futureHandle});
    }

    std::sort(
        plan.externalRewires.begin(), plan.externalRewires.end(),
        [slots](const auto& lhs, const auto& rhs) {
            const auto lk = canonicalCellKey(slots[lhs.outsideCell.slot].record);
            const auto rk = canonicalCellKey(slots[rhs.outsideCell.slot].record);
            if (lk < rk) return true;
            if (rk < lk) return false;
            return lhs.outsideLocalFace < rhs.outsideLocalFace;
        });

    std::set<std::array<DelaunayVertexRef, 4>> candidateKeys;
    for (const DelaunayCandidateCell& candidate : plan.candidateCells) {
        if (!candidateKeys.insert(
                canonicalCellKey(candidate.record)).second) {
            detail = "P1D candidate patch contains duplicate cell connectivity";
            return DelaunayTransactionFailure::DuplicateCandidate;
        }
    }

    return DelaunayTransactionFailure::None;
}

DelaunayTransactionResult fail(
    DelaunayTransactionFailure failure,
    std::string detail) {
    return {failure, std::move(detail), false};
}

DelaunayTransactionResult validatePlanCore(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    const DelaunayInsertionPlan& plan) {
    if (arena.topologyVersion() != plan.sourceTopologyVersion ||
        arena.slots().size() != plan.sourceSlotCount ||
        arena.liveCount() != plan.sourceLiveCount) {
        return fail(
            DelaunayTransactionFailure::StalePlan,
            "P1D plan snapshot no longer matches the source arena");
    }

    std::map<PointId, geometry::Vec3> points;
    std::vector<DelaunaySiteSnapshotEntry> currentSiteSnapshot;
    try {
        points = validatedPoints(sites);
        currentSiteSnapshot = exactSiteSnapshot(sites);
    } catch (const std::exception& error) {
        return fail(DelaunayTransactionFailure::InvalidQuery, error.what());
    }
    if (currentSiteSnapshot != plan.sourceSiteSnapshot) {
        return fail(
            DelaunayTransactionFailure::StalePlan,
            "P1D canonical site snapshot no longer matches the planned geometry");
    }

    const auto query = points.find(plan.queryId);
    if (plan.queryId == InvalidPointId || query == points.end() ||
        !samePoint(query->second, plan.queryPoint)) {
        return fail(
            DelaunayTransactionFailure::InvalidQuery,
            "P1D plan query identity/coordinate does not match canonical sites");
    }
    if (queryIsLive(arena.slots(), plan.queryId)) {
        return fail(
            DelaunayTransactionFailure::DuplicateLiveSite,
            "P1D plan query is already a live explicit vertex");
    }

    try {
        if (!validateDelaunayTopology(arena.slots(), sites).ok()) {
            return fail(
                DelaunayTransactionFailure::InvalidTopology,
                "P1D plan source topology is invalid");
        }
    } catch (const std::exception& error) {
        return fail(DelaunayTransactionFailure::InvalidTopology, error.what());
    }

    std::vector<DelaunayCellHandle> currentConflictOracle;
    try {
        currentConflictOracle = globalConflictOracle(
            arena.slots(),
            points,
            {plan.queryId, plan.queryPoint});
    } catch (const std::exception& error) {
        return fail(DelaunayTransactionFailure::InvalidTopology, error.what());
    }
    if (currentConflictOracle != plan.conflictOracle) {
        return fail(
            DelaunayTransactionFailure::StalePlan,
            "P1D current exact conflict oracle does not match the stored plan");
    }
    if (plan.conflictOracle.empty() ||
        !sameHandleSequence(plan.conflictOracle, plan.conflictFlood)) {
        return fail(
            DelaunayTransactionFailure::DisconnectedCavity,
            "P1D stored flood does not equal stored global conflict oracle");
    }
    if (plan.candidateCells.size() != plan.boundaryFacets.size() ||
        plan.externalRewires.size() != plan.boundaryFacets.size()) {
        return fail(
            DelaunayTransactionFailure::InvalidStitching,
            "P1D candidate/base/rewire cardinalities disagree");
    }

    const auto slotCheck = detail::checkedDelaunayRequiredSlots(
        arena.slots().size(), plan.candidateCells.size());
    if (!slotCheck.ok() ||
        slotCheck.required != plan.requiredSlotCount) {
        return fail(
            DelaunayTransactionFailure::CapacityOverflow,
            "P1D required slot count is invalid");
    }

    std::vector<bool> cavity(arena.slots().size(), false);
    for (const DelaunayCellHandle handle : plan.conflictOracle) {
        if (!liveHandle(arena.slots(), handle) || cavity[handle.slot]) {
            return fail(
                DelaunayTransactionFailure::InvalidTopology,
                "P1D stored cavity handle is stale or duplicated");
        }
        cavity[handle.slot] = true;
    }

    std::set<DelaunayFaceKey> boundaryBaseKeys;
    for (const DelaunayBoundaryFacetRecord& boundary : plan.boundaryFacets) {
        if (!boundaryBaseKeys.insert(boundary.key).second) {
            return fail(
                DelaunayTransactionFailure::InvalidStitching,
                "P1D cavity boundary contains a duplicate canonical base face");
        }
    }

    std::set<std::array<DelaunayVertexRef, 4>> candidateKeys;
    std::set<DelaunayFaceKey> candidateBaseKeys;
    for (std::size_t index = 0U;
         index < plan.candidateCells.size(); ++index) {
        const DelaunayCandidateCell& candidate =
            plan.candidateCells[index];
        const DelaunayCellHandle expected{
            static_cast<std::uint32_t>(arena.slots().size() + index),
            1U};
        if (candidate.futureHandle != expected) {
            return fail(
                DelaunayTransactionFailure::InvalidStitching,
                "P1D candidate future handle is not deterministic append order");
        }
        if (!candidateKeys.insert(
                canonicalCellKey(candidate.record)).second) {
            return fail(
                DelaunayTransactionFailure::DuplicateCandidate,
                "P1D candidate connectivity is duplicated");
        }
        if (!candidateBaseKeys.insert(candidate.baseFace).second) {
            return fail(
                DelaunayTransactionFailure::InvalidStitching,
                "P1D more than one candidate claims the same cavity base face");
        }

        std::optional<std::size_t> queryVertex;
        for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
            if (candidate.record.vertices[vertex].finitePointId() !=
                plan.queryId) {
                continue;
            }
            if (queryVertex.has_value()) {
                return fail(
                    DelaunayTransactionFailure::InvalidCandidateTopology,
                    "P1D candidate contains the inserted query more than once");
            }
            queryVertex = vertex;
        }
        if (!queryVertex.has_value() ||
            canonicalDelaunayFaceKey(
                candidate.record, *queryVertex) != candidate.baseFace) {
            return fail(
                DelaunayTransactionFailure::InvalidCandidateTopology,
                "P1D candidate is not the query cone over its cavity base face");
        }

        if (finiteCell(candidate.record)) {
            std::array<PointId, 4> ids{};
            for (std::size_t i = 0U; i < 4U; ++i) {
                ids[i] = *candidate.record.vertices[i].finitePointId();
            }
            if (predicates::orient3d(
                    pointFor(points, ids[0]),
                    pointFor(points, ids[1]),
                    pointFor(points, ids[2]),
                    pointFor(points, ids[3])).sign !=
                PredicateSign::Positive) {
                return fail(
                    DelaunayTransactionFailure::DegenerateCandidate,
                    "P1D finite candidate is not exact-positive");
            }
        } else if (!ghostCell(candidate.record)) {
            return fail(
                DelaunayTransactionFailure::InvalidCandidateTopology,
                "P1D candidate has invalid finite/ghost vertex pattern");
        } else {
            // Frozen ghost convention: Infinite is slot 0 and finite face 0
            // must pair to a new finite candidate. Its finite triple is outward,
            // so the finite neighbor's opposite witness is exact-negative.
            const DelaunayCellHandle finiteNeighbor =
                candidate.record.neighbors[0];
            if (!finiteNeighbor.isValid() ||
                finiteNeighbor.generation != 1U ||
                finiteNeighbor.slot < plan.sourceSlotCount ||
                finiteNeighbor.slot >= plan.requiredSlotCount) {
                return fail(
                    DelaunayTransactionFailure::InvalidGhostOrientation,
                    "P1D ghost finite face is not paired to a new candidate");
            }
            const std::size_t neighborIndex =
                static_cast<std::size_t>(finiteNeighbor.slot) -
                plan.sourceSlotCount;
            if (neighborIndex >= plan.candidateCells.size() ||
                !finiteCell(plan.candidateCells[neighborIndex].record)) {
                return fail(
                    DelaunayTransactionFailure::InvalidGhostOrientation,
                    "P1D ghost finite face neighbor is not a finite candidate");
            }
            const DelaunayFaceKey ghostFiniteFace =
                canonicalDelaunayFaceKey(candidate.record, 0U);
            const auto finiteFace = matchingFace(
                plan.candidateCells[neighborIndex].record,
                ghostFiniteFace);
            if (!finiteFace.has_value()) {
                return fail(
                    DelaunayTransactionFailure::InvalidGhostOrientation,
                    "P1D ghost/finite candidate face identity mismatch");
            }
            const auto witnessId =
                plan.candidateCells[neighborIndex]
                    .record.vertices[*finiteFace].finitePointId();
            if (!witnessId.has_value()) {
                return fail(
                    DelaunayTransactionFailure::InvalidGhostOrientation,
                    "P1D ghost finite-neighbor witness is not finite");
            }
            const PointId a =
                *candidate.record.vertices[1].finitePointId();
            const PointId d =
                *candidate.record.vertices[2].finitePointId();
            const PointId c =
                *candidate.record.vertices[3].finitePointId();
            if (predicates::orient3d(
                    pointFor(points, a),
                    pointFor(points, d),
                    pointFor(points, c),
                    pointFor(points, *witnessId)).sign !=
                PredicateSign::Negative) {
                return fail(
                    DelaunayTransactionFailure::InvalidGhostOrientation,
                    "P1D ghost finite face is not outward-oriented");
            }
        }

        const auto baseFace = matchingFace(
            candidate.record, candidate.baseFace);
        if (!baseFace.has_value() ||
            *baseFace != candidate.baseLocalFace ||
            candidate.record.neighbors[*baseFace] != candidate.outsideCell) {
            return fail(
                DelaunayTransactionFailure::InvalidStitching,
                "P1D candidate base face/outside neighbor is inconsistent");
        }
    }

    if (candidateBaseKeys != boundaryBaseKeys) {
        return fail(
            DelaunayTransactionFailure::InvalidStitching,
            "P1D candidate base-face set does not equal the cavity boundary set");
    }

    for (const DelaunayExternalRewire& rewire :
         plan.externalRewires) {
        if (!liveHandle(arena.slots(), rewire.outsideCell) ||
            rewire.outsideLocalFace >= 4U ||
            arena.slots()[rewire.outsideCell.slot]
                    .record.neighbors[rewire.outsideLocalFace] !=
                rewire.expectedOldNeighbor ||
            !liveHandle(arena.slots(), rewire.expectedOldNeighbor) ||
            !cavity[rewire.expectedOldNeighbor.slot]) {
            return fail(
                DelaunayTransactionFailure::StalePlan,
                "P1D external rewire precondition is stale or non-reciprocal");
        }
    }

    // Full candidate topology is checked on a temporary snapshot before the
    // commit barrier. This may allocate and evaluate exact orientations; the
    // authoritative arena remains unchanged.
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
        simulated.push_back(std::move(slot));
    }
    for (const DelaunayExternalRewire& rewire :
         plan.externalRewires) {
        simulated[rewire.outsideCell.slot]
            .record.neighbors[rewire.outsideLocalFace] = rewire.newCell;
    }

    try {
        const auto validation =
            validateDelaunayTopology(simulated, sites);
        if (!validation.ok()) {
            return fail(
                DelaunayTransactionFailure::InvalidCandidateTopology,
                "P1D simulated post-state failed the typed topology validator");
        }
    } catch (const std::exception& error) {
        return fail(
            DelaunayTransactionFailure::InvalidCandidateTopology,
            error.what());
    }

    return {};
}

} // namespace

namespace detail {

void setDelaunayCommitBarrierAuditHooks(
    DelaunayCommitBarrierAuditHooks hooks) noexcept {
    commitBarrierAuditHooks = hooks;
}

DelaunayRequiredSlotCountCheck checkedDelaunayRequiredSlots(
    std::size_t current,
    std::size_t additional) noexcept {
    if (additional > std::numeric_limits<std::size_t>::max() - current) {
        return {DelaunayTransactionFailure::CapacityOverflow, 0U};
    }
    const std::size_t required = current + additional;
    if (required >
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max())) {
        return {DelaunayTransactionFailure::CapacityOverflow, 0U};
    }
    return {DelaunayTransactionFailure::None, required};
}

} // namespace detail

struct DelaunayTransactionAccess {
    // COMMIT BARRIER'in otesindeki tek mutation girisi.
    // Preconditions caller tarafinda tamamen dogrulanir ve capacity reserve edilir.
    static void applyPreparedCommit(
        DelaunayReferenceArena& arena,
        const DelaunayInsertionPlan& plan) noexcept {
        static_assert(std::is_nothrow_default_constructible_v<DelaunayCellSlot>);
        static_assert(std::is_nothrow_copy_assignable_v<DelaunayCellRecord>);

        for (const DelaunayCandidateCell& candidate :
             plan.candidateCells) {
            arena.slots_.emplace_back();
            DelaunayCellSlot& slot = arena.slots_.back();
            slot.generation = candidate.futureHandle.generation;
            slot.live = true;
            slot.record = candidate.record;
        }

        for (const DelaunayExternalRewire& rewire :
             plan.externalRewires) {
            arena.slots_[rewire.outsideCell.slot]
                .record.neighbors[rewire.outsideLocalFace] = rewire.newCell;
        }

        for (const DelaunayCellHandle handle :
             plan.conflictOracle) {
            arena.slots_[handle.slot].live = false;
        }

        arena.liveCount_ =
            arena.liveCount_ - plan.conflictOracle.size() +
            plan.candidateCells.size();
        ++arena.topologyVersion_;
    }
};

DelaunayPlanResult buildDelaunayInsertionPlan(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    PointId queryId) {
    DelaunayPlanResult result;
    std::map<PointId, geometry::Vec3> points;
    try {
        points = validatedPoints(sites);
    } catch (const std::exception& error) {
        result.failure = DelaunayTransactionFailure::InvalidQuery;
        result.detail = error.what();
        return result;
    }

    const auto queryIterator = points.find(queryId);
    if (queryId == InvalidPointId || queryIterator == points.end()) {
        result.failure = DelaunayTransactionFailure::InvalidQuery;
        result.detail = "M2 P1D query PointId is missing from canonical sites";
        return result;
    }
    if (queryIsLive(arena.slots(), queryId)) {
        result.failure = DelaunayTransactionFailure::DuplicateLiveSite;
        result.detail = "M2 P1D query PointId is already present in live topology";
        return result;
    }

    try {
        if (!validateDelaunayTopology(arena.slots(), sites).ok()) {
            result.failure = DelaunayTransactionFailure::InvalidTopology;
            result.detail = "M2 P1D requires a valid finite/ghost source topology";
            return result;
        }
    } catch (const std::exception& error) {
        result.failure = DelaunayTransactionFailure::InvalidTopology;
        result.detail = error.what();
        return result;
    }

    DelaunayInsertionPlan plan;
    plan.queryId = queryId;
    plan.queryPoint = queryIterator->second;
    plan.sourceTopologyVersion = arena.topologyVersion();
    plan.sourceSlotCount = arena.slots().size();
    plan.sourceLiveCount = arena.liveCount();
    try {
        plan.sourceSiteSnapshot = exactSiteSnapshot(sites);
    } catch (const std::exception& error) {
        result.failure = DelaunayTransactionFailure::InvalidQuery;
        result.detail = error.what();
        return result;
    }

    const IndexedPoint3 query{queryId, plan.queryPoint};
    try {
        // Independent correctness oracle: every live cell is semantically
        // classified, independent of adjacency traversal.
        plan.conflictOracle =
            globalConflictOracle(arena.slots(), points, query);
        if (plan.conflictOracle.empty()) {
            result.failure = DelaunayTransactionFailure::EmptyConflict;
            result.detail = "M2 P1D global conflict oracle returned an empty set";
            return result;
        }

        // Deterministic seed is the canonical-minimum cell in the oracle set.
        // It is reclassified by the flood before traversal, so exact conflict
        // is an executable seed precondition.
        plan.conflictFlood = adjacencyFlood(
            arena.slots(),
            points,
            query,
            plan.conflictOracle.front());
    } catch (const std::exception& error) {
        result.failure = DelaunayTransactionFailure::InvalidTopology;
        result.detail = error.what();
        return result;
    }

    if (!sameHandleSequence(
            plan.conflictOracle, plan.conflictFlood)) {
        result.failure = DelaunayTransactionFailure::DisconnectedCavity;
        result.detail =
            "M2 P1D adjacency flood disagrees with all-cell conflict oracle";
        return result;
    }

    DelaunayTransactionFailure failure = extractCavity(
        arena.slots(),
        plan.conflictOracle,
        plan,
        result.detail);
    if (failure != DelaunayTransactionFailure::None) {
        result.failure = failure;
        return result;
    }

    try {
        failure = buildCandidatePatch(
            arena.slots(), points, plan, result.detail);
    } catch (const std::exception& error) {
        result.failure = DelaunayTransactionFailure::InvalidStitching;
        result.detail = error.what();
        return result;
    }
    if (failure != DelaunayTransactionFailure::None) {
        result.failure = failure;
        return result;
    }

    plan.validated = true;
    const DelaunayTransactionResult validation =
        validatePlanCore(arena, sites, plan);
    if (!validation.ok()) {
        result.failure = validation.failure;
        result.detail = validation.detail;
        return result;
    }

    result.plan = std::move(plan);
    return result;
}

DelaunayTransactionResult validateDelaunayInsertionPlan(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    const DelaunayInsertionPlan& plan) {
    if (!plan.validated) {
        return fail(
            DelaunayTransactionFailure::InvalidCandidateTopology,
            "P1D plan never completed build-time validation");
    }
    return validatePlanCore(arena, sites, plan);
}

DelaunayTransactionResult reserveDelaunayInsertion(
    DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    DelaunayInsertionPlan& plan,
    DelaunayResourceLimits limits) {
    const DelaunayTransactionResult validation =
        validateDelaunayInsertionPlan(arena, sites, plan);
    if (!validation.ok()) {
        return validation;
    }

    if (plan.requiredSlotCount > limits.maxTotalSlots) {
        return fail(
            DelaunayTransactionFailure::ResourceLimit,
            "P1D configured resource limit is below required slot count");
    }
    try {
        arena.reserve(plan.requiredSlotCount);
    } catch (const std::length_error& error) {
        return fail(
            DelaunayTransactionFailure::CapacityOverflow,
            error.what());
    } catch (const std::bad_alloc& error) {
        return fail(
            DelaunayTransactionFailure::ResourceFailure,
            error.what());
    }

    plan.reserved = true;
    return {};
}

DelaunayTransactionResult commitDelaunayInsertion(
    DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    DelaunayInsertionPlan& plan) {
    const DelaunayTransactionResult validation =
        validateDelaunayInsertionPlan(arena, sites, plan);
    if (!validation.ok()) {
        return validation;
    }
    if (!plan.reserved ||
        arena.capacity() < plan.requiredSlotCount) {
        return fail(
            DelaunayTransactionFailure::NotReserved,
            "P1D commit requires successful pre-commit capacity reservation");
    }
    if (arena.topologyVersion() ==
        std::numeric_limits<std::uint64_t>::max()) {
        return fail(
            DelaunayTransactionFailure::TopologyVersionExhausted,
            "P1D topology version cannot advance");
    }

    // ==================== COMMIT BARRIER ====================
    // Buradan sonra predicate, symbolic tie, point-location, map/hash growth,
    // reserve/capacity request veya yeni topological decision yoktur.
    const std::size_t capacityBefore = arena.capacity();
    const detail::DelaunayCommitBarrierAuditHooks auditHooks =
        commitBarrierAuditHooks;
    if (auditHooks.onBarrierCrossed != nullptr) {
        auditHooks.onBarrierCrossed();
    }
    DelaunayTransactionAccess::applyPreparedCommit(arena, plan);
    if (auditHooks.onMechanicalCommitCompleted != nullptr) {
        auditHooks.onMechanicalCommitCompleted();
    }
    if (arena.capacity() != capacityBefore) {
        // Bu invariant reserve precondition ile ulasilamaz; noexcept commit
        // yolunda recovery topology uretmek de yasaktir.
        std::terminate();
    }

    plan.reserved = false;
    return {
        DelaunayTransactionFailure::None,
        {},
        true};
}

} // namespace femcae::meshing::m2
