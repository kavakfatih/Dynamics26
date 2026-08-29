#include "femcae/meshing/GeometryMeshBridge.h"

#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace femcae::meshing {
geometry::GeometryAssociationMap buildGeometryAssociationMap(const SimulationMesh& mesh){
    struct Tmp{std::unordered_set<std::int64_t> nodes,elements,facets;};
    std::unordered_map<geometry::GeometryEntityId,Tmp> tmp;
    for(const auto& e:mesh.elements) if(e.sourceGeometryId!=geometry::InvalidGeometryId) tmp[e.sourceGeometryId].elements.insert(e.id);
    for(const auto& f:mesh.boundaryFacets) if(f.sourceGeometryId!=geometry::InvalidGeometryId){auto&t=tmp[f.sourceGeometryId];t.facets.insert(f.id);for(auto id:f.nodeIds)t.nodes.insert(id);}
    for(const auto& n:mesh.nodes) if(n.sourceGeometryId!=geometry::InvalidGeometryId) tmp[n.sourceGeometryId].nodes.insert(n.id);
    geometry::GeometryAssociationMap result;
    for(auto& [gid,t]:tmp){geometry::GeometryAssociation a;a.geometryId=gid;a.femNodeIds.assign(t.nodes.begin(),t.nodes.end());a.femElementIds.assign(t.elements.begin(),t.elements.end());a.femFacetIds.assign(t.facets.begin(),t.facets.end());std::sort(a.femNodeIds.begin(),a.femNodeIds.end());std::sort(a.femElementIds.begin(),a.femElementIds.end());std::sort(a.femFacetIds.begin(),a.femFacetIds.end());result.set(std::move(a));}
    return result;
}
} // namespace femcae::meshing
