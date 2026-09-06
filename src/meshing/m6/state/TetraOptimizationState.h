#pragma once

#include "ConstraintView.h"
#include "../quality/MeanRatioExact.h"

#include "femcae/meshing/RobustGeometry.h"
#include "femcae/meshing/TetraTopology.h"

#include <cstddef>
#include <cstdint>
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

    // Operation commit katmani bu primitive'i ancak stale/constraint/quality
    // revalidation sonrasinda cagirir. Method finite/exact-positive state
    // invariant'ini transactional rollback ile korur; kalite iyilesmesine
    // kendi basina authority vermez.
    [[nodiscard]] bool applyValidatedPointCoordinate(
        PointId id,
        const geometry::Vec3& candidate);

    [[nodiscard]] std::vector<TetHandle> liveTetrahedra() const;
    [[nodiscard]] std::vector<TetHandle> incidentTetrahedra(PointId id) const;
    [[nodiscard]] std::vector<PointId> oneRingNeighbors(PointId id) const;

    [[nodiscard]] quality::IndexedTetraCoordinates qualityCell(
        TetHandle tetra,
        std::optional<PointCoordinateOverride> coordinateOverride = std::nullopt) const;

private:
    [[nodiscard]] const TetSlot& checkedSlot(TetHandle tetra) const;
    void validate();
    void rebuildIncidence();

    std::vector<QualityPointState> points_;
    std::vector<TetSlot> tetraSlots_;
    ConstraintView constraints_;
    std::map<PointId, std::size_t> pointIndex_;

    // Vertex -> incident slot index, in compressed-row form: row i of
    // incidenceSlots_ spans [incidenceOffsets_[i], incidenceOffsets_[i + 1])
    // and is ascending, keyed by the points_ index.
    //
    // Rows hold bare slot indices, never TetHandle. A handle carries a
    // generation, and a stored generation is a second source of truth that goes
    // stale the moment slot recycling exists; incidentTetrahedra reads liveness
    // and generation from tetraSlots_ at call time instead.
    //
    // Built once by rebuildIncidence() from validate(). That is sound only
    // because tetraSlots_ is immutable after construction -- see the note on
    // rebuildIncidence().
    std::vector<std::size_t> incidenceOffsets_;
    std::vector<std::uint32_t> incidenceSlots_;
};

} // namespace femcae::meshing::m6::state
