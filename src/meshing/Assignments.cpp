#include "femcae/meshing/Assignments.h"

#include <stdexcept>

namespace femcae::meshing {
std::uint64_t AssignmentStore::add(GeometryAssignment assignment){
    if(assignment.targetGeometryId==geometry::InvalidGeometryId) throw std::invalid_argument("Assignment geometry target ID gecersiz.");
    assignment.id=nextId_++;
    assignments_.push_back(std::move(assignment));
    return assignments_.back().id;
}
void AssignmentStore::clear(){assignments_.clear();nextId_=1;}
std::vector<GeometryAssignment> AssignmentStore::forGeometry(const geometry::GeometryEntityId id) const {std::vector<GeometryAssignment> r;for(const auto& a:assignments_)if(a.targetGeometryId==id)r.push_back(a);return r;}
std::vector<GeometryAssignment> AssignmentStore::ofKind(const AssignmentKind kind) const {std::vector<GeometryAssignment> r;for(const auto& a:assignments_)if(a.kind==kind)r.push_back(a);return r;}
} // namespace femcae::meshing
