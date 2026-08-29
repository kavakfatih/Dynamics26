#include "femcae/meshing/AssignmentResolver.h"

#include <algorithm>
#include <unordered_set>

namespace femcae::meshing {
std::vector<MeshEntityId> boundaryNodeIdsForGeometry(const SimulationMesh& mesh,const geometry::GeometryEntityId gid){
    std::unordered_set<MeshEntityId> unique;
    for(const auto& f:mesh.boundaryFacets)if(f.sourceGeometryId==gid)for(const auto id:f.nodeIds)unique.insert(id);
    std::vector<MeshEntityId> ids(unique.begin(),unique.end());std::sort(ids.begin(),ids.end());return ids;
}
std::vector<ResolvedAssignment> resolveAssignments(const SimulationMesh& mesh,const AssignmentStore& store){
    std::vector<ResolvedAssignment> result;
    for(const auto& a:store.all()){
        ResolvedAssignment r;r.source=a;
        r.facetIds=mesh.facetIdsForGeometry(a.targetGeometryId);
        r.nodeIds=boundaryNodeIdsForGeometry(mesh,a.targetGeometryId);
        r.elementIds=mesh.elementIdsForGeometry(a.targetGeometryId);
        // Body material/section assignment naturally targets elements. Face
        // loads/constraints/contact target facet nodes/facets through provenance.
        result.push_back(std::move(r));
    }
    return result;
}
} // namespace femcae::meshing
