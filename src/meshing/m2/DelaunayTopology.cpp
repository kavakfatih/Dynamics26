#include "DelaunayTopology.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <stdexcept>
#include <tuple>
#include <utility>
#include <vector>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

struct FaceIncidence {
    DelaunayCellHandle cell;
    std::size_t localFace{0};
};

struct EdgeKey {
    std::array<DelaunayVertexRef, 2> vertices{};

    friend bool operator<(const EdgeKey& lhs, const EdgeKey& rhs) noexcept {
        if (lhs.vertices[0] < rhs.vertices[0]) return true;
        if (rhs.vertices[0] < lhs.vertices[0]) return false;
        return lhs.vertices[1] < rhs.vertices[1];
    }
};

enum class CellKind : std::uint8_t {
    Invalid = 0,
    Finite = 1,
    Ghost = 2
};

struct BasisSelection {
    AffineDimension dimension{AffineDimension::Empty};
    std::array<const CanonicalSite*, 4> sites{nullptr, nullptr, nullptr, nullptr};
};

bool samePoint(const geometry::Vec3& lhs, const geometry::Vec3& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

bool collinear(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c) {
    if (predicates::orient2d({a.x, a.y}, {b.x, b.y}, {c.x, c.y}).sign !=
        PredicateSign::Zero) {
        return false;
    }
    if (predicates::orient2d({a.x, a.z}, {b.x, b.z}, {c.x, c.z}).sign !=
        PredicateSign::Zero) {
        return false;
    }
    return predicates::orient2d(
        {a.y, a.z}, {b.y, b.z}, {c.y, c.z}).sign == PredicateSign::Zero;
}

std::vector<const CanonicalSite*> validatedSitesById(
    std::span<const CanonicalSite> sites) {
    std::vector<const CanonicalSite*> ordered;
    ordered.reserve(sites.size());

    std::set<PointId> ids;
    for (const CanonicalSite& site : sites) {
        if (site.id == InvalidPointId) {
            throw std::invalid_argument("M2 bootstrap requires non-zero PointId values");
        }
        if (!std::isfinite(site.point.x) ||
            !std::isfinite(site.point.y) ||
            !std::isfinite(site.point.z)) {
            throw std::invalid_argument("M2 bootstrap requires finite coordinates");
        }
        if (!ids.insert(site.id).second) {
            throw std::invalid_argument("M2 bootstrap received duplicate PointId");
        }
        ordered.push_back(&site);
    }

    for (std::size_t i = 0U; i < ordered.size(); ++i) {
        for (std::size_t j = i + 1U; j < ordered.size(); ++j) {
            if (samePoint(ordered[i]->point, ordered[j]->point)) {
                throw std::invalid_argument(
                    "M2 bootstrap expects M1-canonicalized unique coordinates");
            }
        }
    }

    std::sort(
        ordered.begin(),
        ordered.end(),
        [](const CanonicalSite* lhs, const CanonicalSite* rhs) {
            return lhs->id < rhs->id;
        });
    return ordered;
}

BasisSelection selectDeterministicBasis(
    const std::vector<const CanonicalSite*>& sites) {
    BasisSelection selection;
    if (sites.empty()) {
        return selection;
    }

    selection.sites[0] = sites[0];
    if (sites.size() == 1U) {
        selection.dimension = AffineDimension::Zero;
        return selection;
    }

    selection.sites[1] = sites[1];

    for (std::size_t i = 2U; i < sites.size(); ++i) {
        if (!collinear(
                selection.sites[0]->point,
                selection.sites[1]->point,
                sites[i]->point)) {
            selection.sites[2] = sites[i];
            break;
        }
    }
    if (selection.sites[2] == nullptr) {
        selection.dimension = AffineDimension::One;
        return selection;
    }

    for (const CanonicalSite* site : sites) {
        if (site == selection.sites[0] ||
            site == selection.sites[1] ||
            site == selection.sites[2]) {
            continue;
        }
        if (predicates::orient3d(
                selection.sites[0]->point,
                selection.sites[1]->point,
                selection.sites[2]->point,
                site->point).sign != PredicateSign::Zero) {
            selection.sites[3] = site;
            break;
        }
    }
    if (selection.sites[3] == nullptr) {
        selection.dimension = AffineDimension::Two;
        return selection;
    }

    selection.dimension = AffineDimension::Three;
    return selection;
}

CellKind cellKind(const DelaunayCellRecord& cell) noexcept {
    std::size_t finiteCount = 0U;
    for (const DelaunayVertexRef& vertex : cell.vertices) {
        if (vertex.isFinite()) {
            ++finiteCount;
        }
    }

    if (finiteCount == 4U) {
        return CellKind::Finite;
    }
    if (finiteCount == 3U && cell.vertices[0].isInfinite()) {
        for (std::size_t i = 1U; i < 4U; ++i) {
            if (!cell.vertices[i].isFinite()) {
                return CellKind::Invalid;
            }
        }
        return CellKind::Ghost;
    }
    return CellKind::Invalid;
}

bool hasDuplicateFiniteVertex(const DelaunayCellRecord& cell) {
    std::vector<PointId> ids;
    for (const DelaunayVertexRef& vertex : cell.vertices) {
        if (const auto id = vertex.finitePointId()) {
            ids.push_back(*id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return std::adjacent_find(ids.begin(), ids.end()) != ids.end();
}

DelaunayCellHandle selfHandle(
    std::size_t slotIndex,
    const DelaunayCellSlot& slot) {
    return {
        static_cast<std::uint32_t>(slotIndex),
        slot.generation};
}

void addIssue(
    DelaunayTopologyValidationReport& report,
    DelaunayTopologyIssueCode code,
    DelaunayCellHandle cell,
    std::size_t localFace = 0U) {
    report.issues.push_back({
        code,
        cell,
        static_cast<std::uint8_t>(localFace)});
}

std::map<PointId, geometry::Vec3> pointMap(
    std::span<const CanonicalSite> sites) {
    std::map<PointId, geometry::Vec3> points;
    for (const CanonicalSite& site : sites) {
        if (site.id == InvalidPointId ||
            !std::isfinite(site.point.x) ||
            !std::isfinite(site.point.y) ||
            !std::isfinite(site.point.z) ||
            !points.emplace(site.id, site.point).second) {
            throw std::invalid_argument(
                "M2 topology validation requires unique finite canonical sites");
        }
    }
    return points;
}

std::optional<std::size_t> matchingFace(
    const DelaunayCellRecord& cell,
    const DelaunayFaceKey& key) {
    std::optional<std::size_t> match;
    for (std::size_t localFace = 0U; localFace < 4U; ++localFace) {
        if (canonicalDelaunayFaceKey(cell, localFace) == key) {
            if (match.has_value()) {
                return std::nullopt;
            }
            match = localFace;
        }
    }
    return match;
}

bool isHandleLive(
    std::span<const DelaunayCellSlot> slots,
    DelaunayCellHandle handle) noexcept {
    return handle.isValid() &&
           handle.slot < slots.size() &&
           slots[handle.slot].live &&
           slots[handle.slot].generation == handle.generation;
}

geometry::Vec3 lookupPoint(
    const std::map<PointId, geometry::Vec3>& points,
    PointId id) {
    const auto iterator = points.find(id);
    if (iterator == points.end()) {
        throw std::out_of_range("M2 topology references an unknown PointId");
    }
    return iterator->second;
}

std::array<PointId, 4> finitePointIds(const DelaunayCellRecord& cell) {
    std::array<PointId, 4> ids{};
    for (std::size_t i = 0U; i < 4U; ++i) {
        const auto id = cell.vertices[i].finitePointId();
        if (!id.has_value()) {
            throw std::invalid_argument("finite cell contains Infinite vertex");
        }
        ids[i] = *id;
    }
    return ids;
}

std::array<PointId, 3> ghostHullPointIds(const DelaunayCellRecord& cell) {
    if (cellKind(cell) != CellKind::Ghost) {
        throw std::invalid_argument("ghost hull face requested from non-ghost cell");
    }
    return {
        *cell.vertices[1].finitePointId(),
        *cell.vertices[2].finitePointId(),
        *cell.vertices[3].finitePointId()};
}

} // namespace

bool operator<(
    const DelaunayVertexRef& lhs,
    const DelaunayVertexRef& rhs) noexcept {
    const auto lhsId = lhs.finitePointId();
    const auto rhsId = rhs.finitePointId();

    if (lhsId.has_value() && rhsId.has_value()) {
        return *lhsId < *rhsId;
    }
    if (lhsId.has_value()) {
        return true; // Finite(id) < Infinite topolojik key sirasi.
    }
    return false;
}

DelaunayVertexRef DelaunayVertexRef::finite(PointId id) {
    if (id == InvalidPointId) {
        throw std::invalid_argument("Finite Delaunay vertex requires non-zero PointId");
    }
    DelaunayVertexRef result;
    result.storage_ = id;
    return result;
}

DelaunayVertexRef DelaunayVertexRef::infinite() noexcept {
    return DelaunayVertexRef{};
}

bool DelaunayVertexRef::isFinite() const noexcept {
    return std::holds_alternative<PointId>(storage_);
}

bool DelaunayVertexRef::isInfinite() const noexcept {
    return std::holds_alternative<InfiniteVertexTag>(storage_);
}

std::optional<PointId> DelaunayVertexRef::finitePointId() const noexcept {
    if (const auto* id = std::get_if<PointId>(&storage_)) {
        return *id;
    }
    return std::nullopt;
}

bool operator<(
    const DelaunayFaceKey& lhs,
    const DelaunayFaceKey& rhs) noexcept {
    for (std::size_t i = 0U; i < 3U; ++i) {
        if (lhs.vertices[i] < rhs.vertices[i]) return true;
        if (rhs.vertices[i] < lhs.vertices[i]) return false;
    }
    return false;
}

void DelaunayReferenceArena::reserve(std::size_t capacity) {
    slots_.reserve(capacity);
}

DelaunayCellHandle DelaunayReferenceArena::appendCell(
    const DelaunayCellRecord& record) {
    if (slots_.size() >=
        static_cast<std::size_t>(std::numeric_limits<std::uint32_t>::max())) {
        throw std::length_error("M2 reference arena exhausted 32-bit slot domain");
    }

    const DelaunayCellHandle handle{
        static_cast<std::uint32_t>(slots_.size()),
        1U};
    DelaunayCellSlot slot;
    slot.generation = handle.generation;
    slot.live = true;
    slot.record = record;
    slots_.push_back(std::move(slot));
    ++liveCount_;
    return handle;
}

const DelaunayCellRecord& DelaunayReferenceArena::cell(
    DelaunayCellHandle handle) const {
    if (!isHandleLive(slots_, handle)) {
        throw std::out_of_range("stale or invalid M2 cell handle");
    }
    return slots_[handle.slot].record;
}

DelaunayCellRecord& DelaunayReferenceArena::mutableCell(
    DelaunayCellHandle handle) {
    if (!isHandleLive(slots_, handle)) {
        throw std::out_of_range("stale or invalid M2 cell handle");
    }
    return slots_[handle.slot].record;
}

std::span<const DelaunayCellSlot> DelaunayReferenceArena::slots() const noexcept {
    return slots_;
}

std::size_t DelaunayReferenceArena::liveCount() const noexcept {
    return liveCount_;
}

DelaunayFaceKey canonicalDelaunayFaceKey(
    const DelaunayCellRecord& cell,
    std::size_t oppositeVertex) {
    if (oppositeVertex >= 4U) {
        throw std::out_of_range("M2 local face index must be in [0,3]");
    }

    DelaunayFaceKey key;
    std::size_t write = 0U;
    for (std::size_t vertex = 0U; vertex < 4U; ++vertex) {
        if (vertex != oppositeVertex) {
            key.vertices[write++] = cell.vertices[vertex];
        }
    }
    std::sort(key.vertices.begin(), key.vertices.end());
    return key;
}

std::array<PointId, 3> outwardFiniteFace(
    const DelaunayCellRecord& positiveFiniteCell,
    std::size_t oppositeVertex) {
    if (oppositeVertex >= 4U) {
        throw std::out_of_range("M2 finite face index must be in [0,3]");
    }
    if (cellKind(positiveFiniteCell) != CellKind::Finite) {
        throw std::invalid_argument("outward finite face requires a finite cell");
    }

    constexpr std::array<std::array<std::size_t, 3>, 4> faceTable{{
        {{1U, 2U, 3U}},
        {{0U, 3U, 2U}},
        {{0U, 1U, 3U}},
        {{0U, 2U, 1U}},
    }};

    const auto& indices = faceTable[oppositeVertex];
    return {
        *positiveFiniteCell.vertices[indices[0]].finitePointId(),
        *positiveFiniteCell.vertices[indices[1]].finitePointId(),
        *positiveFiniteCell.vertices[indices[2]].finitePointId()};
}

DelaunayComplexStats computeDelaunayComplexStats(
    std::span<const DelaunayCellSlot> slots) {
    std::set<DelaunayVertexRef> vertices;
    std::set<EdgeKey> edges;
    std::set<DelaunayFaceKey> faces;

    DelaunayComplexStats stats;
    for (const DelaunayCellSlot& slot : slots) {
        if (!slot.live) {
            continue;
        }
        ++stats.cells;

        const CellKind kind = cellKind(slot.record);
        if (kind == CellKind::Finite) ++stats.finiteCells;
        if (kind == CellKind::Ghost) ++stats.ghostCells;

        for (const DelaunayVertexRef& vertex : slot.record.vertices) {
            vertices.insert(vertex);
        }
        for (std::size_t i = 0U; i < 4U; ++i) {
            for (std::size_t j = i + 1U; j < 4U; ++j) {
                EdgeKey edge{{slot.record.vertices[i], slot.record.vertices[j]}};
                if (edge.vertices[1] < edge.vertices[0]) {
                    std::swap(edge.vertices[0], edge.vertices[1]);
                }
                edges.insert(std::move(edge));
            }
            faces.insert(canonicalDelaunayFaceKey(slot.record, i));
        }
    }

    stats.vertices = vertices.size();
    stats.edges = edges.size();
    stats.faces = faces.size();
    stats.eulerCharacteristic =
        static_cast<std::int64_t>(stats.vertices) -
        static_cast<std::int64_t>(stats.edges) +
        static_cast<std::int64_t>(stats.faces) -
        static_cast<std::int64_t>(stats.cells);
    return stats;
}

DelaunayTopologyValidationReport validateDelaunayTopology(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites) {
    DelaunayTopologyValidationReport report;
    const auto points = pointMap(sites);

    std::map<DelaunayFaceKey, std::vector<FaceIncidence>> faceIncidences;

    for (std::size_t slotIndex = 0U; slotIndex < slots.size(); ++slotIndex) {
        const DelaunayCellSlot& slot = slots[slotIndex];
        if (!slot.live) {
            continue;
        }

        const DelaunayCellHandle self = selfHandle(slotIndex, slot);
        if (slot.generation == 0U) {
            addIssue(report, DelaunayTopologyIssueCode::LiveSlotZeroGeneration, self);
        }

        const CellKind kind = cellKind(slot.record);
        if (kind == CellKind::Invalid) {
            addIssue(report, DelaunayTopologyIssueCode::InvalidCellVertexPattern, self);
            continue;
        }
        if (hasDuplicateFiniteVertex(slot.record)) {
            addIssue(report, DelaunayTopologyIssueCode::DuplicateFiniteVertex, self);
            continue;
        }

        bool missingPoint = false;
        for (const DelaunayVertexRef& vertex : slot.record.vertices) {
            if (const auto id = vertex.finitePointId()) {
                if (!points.contains(*id)) {
                    missingPoint = true;
                }
            }
        }
        if (missingPoint) {
            addIssue(report, DelaunayTopologyIssueCode::MissingFinitePoint, self);
            continue;
        }

        if (kind == CellKind::Finite) {
            const auto ids = finitePointIds(slot.record);
            const PredicateSign sign = predicates::orient3d(
                lookupPoint(points, ids[0]),
                lookupPoint(points, ids[1]),
                lookupPoint(points, ids[2]),
                lookupPoint(points, ids[3])).sign;
            if (sign != PredicateSign::Positive) {
                addIssue(report, DelaunayTopologyIssueCode::NonPositiveFiniteCell, self);
            }
        }

        for (std::size_t localFace = 0U; localFace < 4U; ++localFace) {
            faceIncidences[canonicalDelaunayFaceKey(
                slot.record, localFace)].push_back({self, localFace});

            const DelaunayCellHandle neighbor = slot.record.neighbors[localFace];
            if (!neighbor.isValid()) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::InvalidNeighborHandle,
                    self,
                    localFace);
                continue;
            }
            if (neighbor.slot >= slots.size()) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::NeighborOutOfRange,
                    self,
                    localFace);
                continue;
            }

            const DelaunayCellSlot& neighborSlot = slots[neighbor.slot];
            if (!neighborSlot.live) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::NeighborDead,
                    self,
                    localFace);
                continue;
            }
            if (neighborSlot.generation != neighbor.generation) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::StaleNeighborGeneration,
                    self,
                    localFace);
                continue;
            }

            const DelaunayFaceKey key =
                canonicalDelaunayFaceKey(slot.record, localFace);
            const auto neighborFace = matchingFace(neighborSlot.record, key);
            if (!neighborFace.has_value()) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::NeighborFaceMismatch,
                    self,
                    localFace);
                continue;
            }
            if (neighborSlot.record.neighbors[*neighborFace] != self) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::NonReciprocalNeighbor,
                    self,
                    localFace);
            }
        }

        if (kind == CellKind::Ghost) {
            const DelaunayCellHandle finiteNeighbor = slot.record.neighbors[0];
            if (!isHandleLive(slots, finiteNeighbor) ||
                cellKind(slots[finiteNeighbor.slot].record) != CellKind::Finite) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::GhostFiniteNeighborMissing,
                    self,
                    0U);
            } else {
                const DelaunayCellRecord& finiteCell =
                    slots[finiteNeighbor.slot].record;
                const DelaunayFaceKey hullKey =
                    canonicalDelaunayFaceKey(slot.record, 0U);
                const auto finiteFace = matchingFace(finiteCell, hullKey);
                if (!finiteFace.has_value()) {
                    addIssue(
                        report,
                        DelaunayTopologyIssueCode::GhostFiniteNeighborMissing,
                        self,
                        0U);
                } else {
                    const auto oppositeId =
                        finiteCell.vertices[*finiteFace].finitePointId();
                    const auto hull = ghostHullPointIds(slot.record);
                    if (!oppositeId.has_value() ||
                        predicates::orient3d(
                            lookupPoint(points, hull[0]),
                            lookupPoint(points, hull[1]),
                            lookupPoint(points, hull[2]),
                            lookupPoint(points, *oppositeId)).sign !=
                            PredicateSign::Negative) {
                        addIssue(
                            report,
                            DelaunayTopologyIssueCode::GhostHullOrientationInvalid,
                            self,
                            0U);
                    }
                }
            }

            for (std::size_t localFace = 1U; localFace < 4U; ++localFace) {
                const DelaunayCellHandle neighbor = slot.record.neighbors[localFace];
                if (!isHandleLive(slots, neighbor) ||
                    cellKind(slots[neighbor.slot].record) != CellKind::Ghost) {
                    addIssue(
                        report,
                        DelaunayTopologyIssueCode::GhostLateralNeighborNotGhost,
                        self,
                        localFace);
                }
            }
        }
    }

    for (const auto& [key, incidences] : faceIncidences) {
        (void)key;
        if (incidences.size() != 2U) {
            for (const FaceIncidence& incidence : incidences) {
                addIssue(
                    report,
                    DelaunayTopologyIssueCode::FaceIncidenceNotTwo,
                    incidence.cell,
                    incidence.localFace);
            }
        }
    }

    const DelaunayComplexStats stats = computeDelaunayComplexStats(slots);
    if (stats.cells > 0U && stats.eulerCharacteristic != 0) {
        addIssue(
            report,
            DelaunayTopologyIssueCode::EulerCharacteristicMismatch,
            InvalidDelaunayCellHandle);
    }

    return report;
}

struct DelaunayBootstrapBuilderAccess {
    static DelaunayCellRecord& mutableCell(
        DelaunayReferenceArena& arena,
        DelaunayCellHandle handle) {
        return arena.mutableCell(handle);
    }
};

DelaunayBootstrapResult buildDelaunayBootstrap(
    std::span<const CanonicalSite> sites) {
    const auto ordered = validatedSitesById(sites);
    const BasisSelection selection = selectDeterministicBasis(ordered);

    DelaunayBootstrapResult result;
    result.affineDimension = selection.dimension;
    for (std::size_t i = 0U; i < 4U; ++i) {
        if (selection.sites[i] != nullptr) {
            result.basisPointIds[i] = selection.sites[i]->id;
        }
    }

    if (selection.dimension != AffineDimension::Three) {
        result.status = DelaunayBootstrapStatus::LowerDimensional;
        return result;
    }

    std::array<const CanonicalSite*, 4> stored = selection.sites;
    PredicateSign orientation = predicates::orient3d(
        stored[0]->point,
        stored[1]->point,
        stored[2]->point,
        stored[3]->point).sign;
    if (orientation == PredicateSign::Negative) {
        std::swap(stored[0], stored[1]);
        orientation = PredicateSign::Positive;
    }
    if (orientation != PredicateSign::Positive) {
        throw std::logic_error("M2 basis selection produced a zero-volume tetrahedron");
    }

    DelaunayCellRecord finiteCell;
    for (std::size_t i = 0U; i < 4U; ++i) {
        finiteCell.vertices[i] = DelaunayVertexRef::finite(stored[i]->id);
    }

    result.arena.reserve(5U);
    const DelaunayCellHandle finiteHandle =
        result.arena.appendCell(finiteCell);

    std::array<DelaunayCellHandle, 4> ghosts{};
    for (std::size_t localFace = 0U; localFace < 4U; ++localFace) {
        const auto face = outwardFiniteFace(finiteCell, localFace);

        DelaunayCellRecord ghost;
        ghost.vertices[0] = DelaunayVertexRef::infinite();
        ghost.vertices[1] = DelaunayVertexRef::finite(face[0]);
        ghost.vertices[2] = DelaunayVertexRef::finite(face[1]);
        ghost.vertices[3] = DelaunayVertexRef::finite(face[2]);

        ghosts[localFace] = result.arena.appendCell(ghost);
        DelaunayBootstrapBuilderAccess::mutableCell(
            result.arena, finiteHandle).neighbors[localFace] = ghosts[localFace];
        DelaunayBootstrapBuilderAccess::mutableCell(
            result.arena, ghosts[localFace]).neighbors[0] = finiteHandle;
    }

    std::map<DelaunayFaceKey, std::pair<DelaunayCellHandle, std::size_t>>
        unmatched;
    for (DelaunayCellHandle ghostHandle : ghosts) {
        for (std::size_t localFace = 1U; localFace < 4U; ++localFace) {
            const DelaunayFaceKey key = canonicalDelaunayFaceKey(
                result.arena.cell(ghostHandle), localFace);
            const auto [iterator, inserted] =
                unmatched.emplace(key, std::make_pair(ghostHandle, localFace));
            if (inserted) {
                continue;
            }

            const auto [otherHandle, otherFace] = iterator->second;
            DelaunayBootstrapBuilderAccess::mutableCell(
                result.arena, ghostHandle).neighbors[localFace] = otherHandle;
            DelaunayBootstrapBuilderAccess::mutableCell(
                result.arena, otherHandle).neighbors[otherFace] = ghostHandle;
            unmatched.erase(iterator);
        }
    }
    if (!unmatched.empty()) {
        throw std::logic_error("M2 bootstrap ghost lateral faces did not pair exactly");
    }

    const auto validation =
        validateDelaunayTopology(result.arena.slots(), sites);
    if (!validation.ok()) {
        throw std::logic_error("M2 bootstrap failed its own topology validator");
    }

    result.status = DelaunayBootstrapStatus::Ready;
    return result;
}

} // namespace femcae::meshing::m2
