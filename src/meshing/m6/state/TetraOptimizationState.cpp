#include "TetraOptimizationState.h"

#include "../quality/QualityVector.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <stdexcept>
#include <utility>

namespace femcae::meshing::m6::state {
namespace {

bool finitePoint(const geometry::Vec3& point) noexcept {
    return std::isfinite(point.x) &&
           std::isfinite(point.y) &&
           std::isfinite(point.z);
}

} // namespace

TetraOptimizationState::TetraOptimizationState(
    std::vector<QualityPointState> points,
    std::vector<TetSlot> tetraSlots,
    ConstraintView constraints)
    : points_(std::move(points)),
      tetraSlots_(std::move(tetraSlots)),
      constraints_(std::move(constraints)) {
    validate();
}

TetraOptimizationState TetraOptimizationState::fromCanonicalSites(
    std::span<const CanonicalSite> sites,
    std::vector<TetSlot> tetraSlots,
    ConstraintView constraints) {
    std::vector<QualityPointState> points;
    points.reserve(sites.size());
    for (const CanonicalSite& site : sites) {
        points.push_back({site.id, site.point});
    }

    return TetraOptimizationState(
        std::move(points),
        std::move(tetraSlots),
        std::move(constraints));
}

bool TetraOptimizationState::hasPoint(
    PointId id) const noexcept {
    return pointIndex_.find(id) != pointIndex_.end();
}

const geometry::Vec3& TetraOptimizationState::point(
    PointId id) const {
    const auto it = pointIndex_.find(id);
    if (it == pointIndex_.end()) {
        throw std::out_of_range(
            "M6 optimizer PointId is not present in state");
    }
    return points_[it->second].point;
}

bool TetraOptimizationState::applyValidatedPointCoordinate(
    PointId id,
    const geometry::Vec3& candidate) {
    const auto it = pointIndex_.find(id);
    if (it == pointIndex_.end() ||
        !finitePoint(candidate) ||
        !constraints_.isInteriorFree(id) ||
        constraints_.pointTouchesProtectedTopology(id)) {
        return false;
    }

    const geometry::Vec3 previous =
        points_[it->second].point;
    points_[it->second].point = candidate;

    bool valid = true;
    try {
        for (TetHandle tetra : incidentTetrahedra(id)) {
            if (!quality::isExactPositiveQualityCell(
                    qualityCell(tetra))) {
                valid = false;
                break;
            }
        }
    } catch (const std::exception&) {
        valid = false;
    }

    if (!valid) {
        points_[it->second].point = previous;
        return false;
    }
    return true;
}

std::vector<TetHandle> TetraOptimizationState::liveTetrahedra() const {
    std::vector<TetHandle> result;
    for (std::size_t slot = 0; slot < tetraSlots_.size(); ++slot) {
        if (!tetraSlots_[slot].live) {
            continue;
        }
        result.push_back({
            static_cast<std::uint32_t>(slot),
            tetraSlots_[slot].generation});
    }
    return result;
}

std::vector<TetHandle> TetraOptimizationState::incidentTetrahedra(
    PointId id) const {
    const auto it = pointIndex_.find(id);
    if (it == pointIndex_.end()) {
        // An unknown PointId and a known point with an empty star are different
        // answers: SmartSmoothing branches on the empty vector, and must not
        // receive it for a point that does not exist.
        throw std::out_of_range(
            "M6 incident-star PointId is not present in state");
    }

    if (incidenceOffsets_.size() != points_.size() + 1U) {
        throw std::logic_error(
            "M6 incidence index does not match the point set");
    }

    const std::size_t begin = incidenceOffsets_[it->second];
    const std::size_t end = incidenceOffsets_[it->second + 1U];

    std::vector<TetHandle> result;
    result.reserve(end - begin);
    for (std::size_t entry = begin; entry < end; ++entry) {
        const std::size_t slot = incidenceSlots_[entry];

        // Liveness and generation are read here, not stored in the row. Today
        // no live slot can die, so this never filters anything out; it is the
        // tripwire that keeps the reader correct when topological operators
        // start recycling slots.
        const TetSlot& tetra = tetraSlots_[slot];
        if (!tetra.live) {
            continue;
        }
        result.push_back({
            static_cast<std::uint32_t>(slot),
            tetra.generation});
    }
    return result;
}

std::vector<PointId> TetraOptimizationState::oneRingNeighbors(
    PointId id) const {
    std::set<PointId> neighbors;
    for (const TetHandle tetra : incidentTetrahedra(id)) {
        const TetSlot& slot = checkedSlot(tetra);
        for (PointId vertex : slot.record.vertices) {
            if (vertex != id) {
                neighbors.insert(vertex);
            }
        }
    }

    return {
        neighbors.begin(),
        neighbors.end()};
}

quality::IndexedTetraCoordinates TetraOptimizationState::qualityCell(
    TetHandle tetra,
    std::optional<PointCoordinateOverride> coordinateOverride) const {
    const TetSlot& slot = checkedSlot(tetra);

    if (coordinateOverride.has_value()) {
        if (coordinateOverride->id == InvalidPointId ||
            !hasPoint(coordinateOverride->id) ||
            !finitePoint(coordinateOverride->point)) {
            throw std::invalid_argument(
                "M6 candidate coordinate override is invalid");
        }
    }

    quality::IndexedTetraCoordinates result;
    result.pointIds = slot.record.vertices;

    for (std::size_t local = 0; local < 4U; ++local) {
        const PointId id = slot.record.vertices[local];
        if (coordinateOverride.has_value() &&
            coordinateOverride->id == id) {
            result.coordinates[local] =
                coordinateOverride->point;
        } else {
            result.coordinates[local] = point(id);
        }
    }

    return result;
}

void TetraOptimizationState::rebuildIncidence() {
    // Counting sort in two linear passes: count per point, prefix-sum into
    // offsets, then scatter.
    //
    // ORDER IS PART OF THE CONTRACT. The scan this replaces emitted handles in
    // ascending slot order, and commitSmartSmoothing compares the star it
    // recomputes against the one stored in the proposal with an element-wise
    // vector !=. A different permutation of the same set would turn every
    // commit into StaleProposal -- no exception, no wrong mesh, the optimizer
    // would just silently stop making progress. Slots are visited in ascending
    // order in the scatter pass and appended in that order, so each row comes
    // out ascending.
    //
    // Caller contract: this is correct for as long as tetraSlots_ is immutable
    // after construction, which it is today -- nothing in this class writes
    // TetSlot::live, TetSlot::generation or TetRecord::vertices, and the only
    // post-construction mutation is a coordinate write in
    // applyValidatedPointCoordinate. The first operator that creates, kills or
    // rewires a slot MUST re-run this (or validate()); there is deliberately no
    // lazy rebuild, because incidentTetrahedra is const and is called through a
    // const reference whose documented contract is that the state does not
    // change underneath it.
    incidenceOffsets_.assign(points_.size() + 1U, 0U);
    incidenceSlots_.clear();

    // validate() has already run validateTetTopology, so a live slot has four
    // distinct, resolvable vertices. Each slot therefore contributes exactly
    // one entry per row, matching the single std::find hit of the old scan.
    for (const TetSlot& slot : tetraSlots_) {
        if (!slot.live) {
            continue;
        }
        for (PointId vertex : slot.record.vertices) {
            ++incidenceOffsets_[pointIndex_.at(vertex) + 1U];
        }
    }

    for (std::size_t point = 0; point < points_.size(); ++point) {
        incidenceOffsets_[point + 1U] += incidenceOffsets_[point];
    }

    std::vector<std::size_t> cursor(
        incidenceOffsets_.begin(),
        incidenceOffsets_.end() - 1);
    incidenceSlots_.resize(incidenceOffsets_.back());

    for (std::size_t slot = 0; slot < tetraSlots_.size(); ++slot) {
        if (!tetraSlots_[slot].live) {
            continue;
        }
        if (slot > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument(
                "M6 optimizer state exceeds the addressable tetra slot range");
        }
        for (PointId vertex : tetraSlots_[slot].record.vertices) {
            incidenceSlots_[cursor[pointIndex_.at(vertex)]++] =
                static_cast<std::uint32_t>(slot);
        }
    }
}

const TetSlot& TetraOptimizationState::checkedSlot(
    TetHandle tetra) const {
    if (!tetra.isValid() ||
        tetra.slot >= tetraSlots_.size()) {
        throw std::out_of_range(
            "M6 optimizer tetra handle is invalid");
    }

    const TetSlot& slot = tetraSlots_[tetra.slot];
    if (!slot.live ||
        slot.generation != tetra.generation) {
        throw std::out_of_range(
            "M6 optimizer tetra handle is dead or stale");
    }
    return slot;
}

void TetraOptimizationState::validate() {
    pointIndex_.clear();

    for (std::size_t index = 0; index < points_.size(); ++index) {
        const QualityPointState& pointState = points_[index];
        if (pointState.id == InvalidPointId ||
            !finitePoint(pointState.point)) {
            throw std::invalid_argument(
                "M6 optimizer state contains invalid point");
        }

        if (!pointIndex_.emplace(
                pointState.id,
                index).second) {
            throw std::invalid_argument(
                "M6 optimizer state contains duplicate PointId");
        }
    }

    const TopologyValidationReport topology =
        validateTetTopology(tetraSlots_);
    if (!topology.ok()) {
        throw std::invalid_argument(
            "M6 optimizer state tetra topology is invalid");
    }

    for (const TetHandle tetra : liveTetrahedra()) {
        const TetSlot& slot = checkedSlot(tetra);
        for (PointId id : slot.record.vertices) {
            if (!hasPoint(id)) {
                throw std::invalid_argument(
                    "M6 live tetra references unknown PointId");
            }
        }
    }

    // Only now: rebuildIncidence resolves every live vertex through
    // pointIndex_.at, so it must run after the check above. Built earlier, a
    // malformed input would surface as std::out_of_range from .at() instead of
    // the std::invalid_argument this class promises.
    rebuildIncidence();

    for (const PointMobilityEntry& entry :
         constraints_.pointMobilityEntries()) {
        if (!hasPoint(entry.point)) {
            throw std::invalid_argument(
                "M6 constraint mobility references unknown PointId");
        }
    }
    for (const ProtectedEdgeKey& edge :
         constraints_.protectedEdges()) {
        if (!hasPoint(edge.vertices[0]) ||
            !hasPoint(edge.vertices[1])) {
            throw std::invalid_argument(
                "M6 protected edge references unknown PointId");
        }
    }
    for (const CanonicalFaceKey& face :
         constraints_.protectedFaces()) {
        for (PointId id : face.vertices) {
            if (!hasPoint(id)) {
                throw std::invalid_argument(
                    "M6 protected face references unknown PointId");
            }
        }
    }

    for (const TetHandle tetra : liveTetrahedra()) {
        const quality::IndexedTetraCoordinates cell =
            qualityCell(tetra);

        if (!quality::isExactPositiveQualityCell(cell)) {
            throw std::invalid_argument(
                "M6 optimizer state requires exact-positive live TET4");
        }
    }
}

} // namespace femcae::meshing::m6::state
