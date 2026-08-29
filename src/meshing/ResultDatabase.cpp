#include "femcae/meshing/ResultDatabase.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <stdexcept>

namespace femcae::meshing {
namespace {
double dot(const geometry::Vec3&a,const geometry::Vec3&b){return a.x*b.x+a.y*b.y+a.z*b.z;}
geometry::Vec3 sub(const geometry::Vec3&a,const geometry::Vec3&b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
double norm(const geometry::Vec3&a){return std::sqrt(dot(a,a));}
}
void ResultDatabase::clear(){displacement_.reset();elementScalars_.clear();}
void ResultDatabase::setDisplacement(NodeVectorField field){displacement_=std::move(field);}
void ResultDatabase::setElementScalar(ElementScalarField field){for(auto& f:elementScalars_)if(f.name==field.name){f=std::move(field);return;}elementScalars_.push_back(std::move(field));}
const NodeVectorField* ResultDatabase::displacement() const noexcept{return displacement_?&*displacement_:nullptr;}
const ElementScalarField* ResultDatabase::elementScalar(const std::string_view name) const noexcept{for(const auto& f:elementScalars_)if(f.name==name)return &f;return nullptr;}
std::optional<ProbeResult> ResultDatabase::probeNearestNode(const SimulationMesh& mesh,const geometry::Vec3& point) const{
    if(!displacement_||mesh.nodes.empty())return std::nullopt;
    ProbeResult best; best.distance=std::numeric_limits<double>::infinity();
    for(const auto& n:mesh.nodes){const double d=norm(sub(n.x,point));const auto it=displacement_->values.find(n.id);if(it==displacement_->values.end())continue;if(d<best.distance){best={n.id,n.x,it->second,d};}}
    return best.nodeId==InvalidMeshId?std::nullopt:std::optional<ProbeResult>(best);
}
std::vector<MeshEntityId> ResultDatabase::cutElements(const SimulationMesh& mesh,const PlaneCut& plane) const{
    std::vector<MeshEntityId> out; const double nn=norm(plane.normal); if(nn<=0.0)throw std::invalid_argument("Section cut plane normal sifir olamaz.");
    for(const auto&e:mesh.elements){bool pos=false,neg=false,on=false;for(const auto nid:e.nodeIds){const auto*n=mesh.findNode(nid);if(!n)continue;const double s=dot(sub(n->x,plane.point),plane.normal)/nn;if(std::abs(s)<=plane.tolerance)on=true;else if(s>0)pos=true;else neg=true;}if(on||(pos&&neg))out.push_back(e.id);}return out;
}
void ResultDatabase::exportCsv(const SimulationMesh& mesh,const std::filesystem::path& path) const{
    std::ofstream out(path);if(!out)throw std::runtime_error("Result CSV acilamadi.");out<<"node_id,x,y,z,ux,uy,uz\n";
    for(const auto& n:mesh.nodes){geometry::Vec3 u{};if(displacement_){const auto it=displacement_->values.find(n.id);if(it!=displacement_->values.end())u=it->second;}out<<n.id<<','<<n.x.x<<','<<n.x.y<<','<<n.x.z<<','<<u.x<<','<<u.y<<','<<u.z<<'\n';}
}
void ResultDatabase::exportLegacyVtk(const SimulationMesh& mesh,const std::filesystem::path& path,const double scale) const{
    std::ofstream out(path);if(!out)throw std::runtime_error("Legacy VTK dosyasi acilamadi.");
    out<<"# vtk DataFile Version 3.0\nFEMCAE V1.0 result\nASCII\nDATASET UNSTRUCTURED_GRID\nPOINTS "<<mesh.nodes.size()<<" double\n";
    std::unordered_map<MeshEntityId,std::size_t> index;
    for(std::size_t i=0;i<mesh.nodes.size();++i){index[mesh.nodes[i].id]=i;geometry::Vec3 u{};if(displacement_){const auto it=displacement_->values.find(mesh.nodes[i].id);if(it!=displacement_->values.end())u=it->second;}out<<mesh.nodes[i].x.x+scale*u.x<<' '<<mesh.nodes[i].x.y+scale*u.y<<' '<<mesh.nodes[i].x.z+scale*u.z<<'\n';}
    out<<"CELLS "<<mesh.elements.size()<<' '<<mesh.elements.size()*9<<"\n";for(const auto&e:mesh.elements){out<<8;for(const auto nid:e.nodeIds)out<<' '<<index.at(nid);out<<'\n';}
    out<<"CELL_TYPES "<<mesh.elements.size()<<"\n";for(std::size_t i=0;i<mesh.elements.size();++i)out<<12<<'\n';
    if(displacement_){out<<"POINT_DATA "<<mesh.nodes.size()<<"\nVECTORS displacement double\n";for(const auto&n:mesh.nodes){geometry::Vec3 u{};const auto it=displacement_->values.find(n.id);if(it!=displacement_->values.end())u=it->second;out<<u.x<<' '<<u.y<<' '<<u.z<<'\n';}}
    if(!elementScalars_.empty()){out<<"CELL_DATA "<<mesh.elements.size()<<"\n";for(const auto&f:elementScalars_){out<<"SCALARS "<<f.name<<" double 1\nLOOKUP_TABLE default\n";for(const auto&e:mesh.elements){const auto it=f.values.find(e.id);out<<(it==f.values.end()?0.0:it->second)<<'\n';}}}
}
} // namespace femcae::meshing
