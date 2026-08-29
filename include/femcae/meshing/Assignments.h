#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace femcae::meshing {

enum class AssignmentKind : std::uint8_t { Material, Section, Constraint, Load, Contact };

struct GeometryAssignment {
    std::uint64_t id{0};
    AssignmentKind kind{AssignmentKind::Material};
    geometry::GeometryEntityId targetGeometryId{geometry::InvalidGeometryId};
    std::string name;
    std::int64_t referencedEntityId{-1};
    std::array<double, 3> vectorValue{0.0, 0.0, 0.0};
    std::array<bool, 3> constrained{false, false, false};
};

class AssignmentStore {
public:
    std::uint64_t add(GeometryAssignment assignment);
    void clear();
    [[nodiscard]] std::vector<GeometryAssignment> forGeometry(geometry::GeometryEntityId geometryId) const;
    [[nodiscard]] std::vector<GeometryAssignment> ofKind(AssignmentKind kind) const;
    [[nodiscard]] const std::vector<GeometryAssignment>& all() const noexcept { return assignments_; }
private:
    std::uint64_t nextId_{1};
    std::vector<GeometryAssignment> assignments_;
};

} // namespace femcae::meshing
