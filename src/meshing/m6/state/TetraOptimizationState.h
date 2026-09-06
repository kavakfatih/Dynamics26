#pragma once

#include "ConstraintView.h"
#include "../quality/MeanRatioExact.h"

#include "femcae/meshing/RobustGeometry.h"
#include "femcae/meshing/TetraTopology.h"

#include <map>
#include <optional>
#include <span>
#include <vector>

namespace femcae::meshing::m6::state {

struct QualityPointState {
    PointId id{InvalidPointId};
    geometry::Vec3 point{};
};

struct PointCoordinateOverride {
    PointId id{InvalidPointId};
    geometry::Vec3 point{};
};

class TetraOptimizationState {
public:
    TetraOptimizationState(
        std::vector<QualityPointState> points,
        std::vector<TetSlot> tetraSlots,
        ConstraintView constraints);

    [[nodiscard]] static TetraOptimizationState fromCanonicalSites(
        std::span<const CanonicalSite> sites,
        std::vector<TetSlot> tetraSlots,
        ConstraintView constraints);

    [[nodiscard]] const std::vector<QualityPointState>& points() const noexcept {
        return points_;
    }

    [[nodiscard]] const std::vector<TetSlot>& tetraSlots() const noexcept {
        return tetraSlots_;
    }

    [[nodiscard]] const ConstraintView& constraints() const noexcept {
        return constraints_;
    }

    [[nodiscard]] bool hasPoint(PointId id) const noexcept;
    [[nodiscard]] const geometry::Vec3& point(PointId id) const;

    [[nodiscard]] std::vector<TetHandle> liveTetrahedra() const;
    [[nodiscard]] std::vector<TetHandle> incidentTetrahedra(PointId id) const;
    [[nodiscard]] std::vector<PointId> oneRingNeighbors(PointId id) const;

    [[nodiscard]] quality::IndexedTetraCoordinates qualityCell(
        TetHandle tetra,
        std::optional<PointCoordinateOverride> coordinateOverride = std::nullopt) const;

private:
    [[nodiscard]] const TetSlot& checkedSlot(TetHandle tetra) const;
    void validate();

    std::vector<QualityPointState> points_;
    std::vector<TetSlot> tetraSlots_;
    ConstraintView constraints_;
    std::map<PointId, std::size_t> pointIndex_;
};

} // namespace femcae::meshing::m6::state
