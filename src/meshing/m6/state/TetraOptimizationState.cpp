#include "TetraOptimizationState.h"

#include "../quality/QualityVector.h"

#include <algorithm>
#include <cmath>
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
    if (!hasPoint(id)) {
        throw std::out_of_range(
            "M6 incident-star PointId is not present in state");
    }

    std::vector<TetHandle> result;
    for (std::size_t slot = 0; slot < tetraSlots_.size(); ++slot) {
        const TetSlot& tetra = tetraSlots_[slot];
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
