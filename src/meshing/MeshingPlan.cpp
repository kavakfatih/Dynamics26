#include "femcae/meshing/MeshingPlan.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace femcae::meshing {
namespace {
std::size_t divisions(const double length,const double h){return static_cast<std::size_t>(std::max(1.0,std::ceil(length/h)));}
double local(const MeshingPlan& p,const geometry::GeometryEntityId id,const double fallback){const auto it=p.localTargetSize.find(id);return it==p.localTargetSize.end()?fallback:it->second;}
void validateSize(const double h){if(!(h>0.0)||!std::isfinite(h))throw std::invalid_argument("Mesh target size pozitif ve sonlu olmali.");}
}
StructuredHexMesherOptions structuredOptionsFromSizing(const AxisAlignedBox& box,const BoxBoundaryGeometry& g,const MeshingPlan& p){
    if(p.preferredTopology!=MeshTopology::Hex8)throw std::invalid_argument("Structured box mesher yalniz HEX8 uretir.");
    validateSize(p.globalTargetSize);
    for(const auto& [id,h]:p.localTargetSize){(void)id;validateSize(h);}
    const double lx=box.max.x-box.min.x,ly=box.max.y-box.min.y,lz=box.max.z-box.min.z;
    if(!(lx>0&&ly>0&&lz>0))throw std::invalid_argument("Sizing icin box boyutlari gecersiz.");
    // A face size refines the two tangential directions of that face.
    double hx=p.globalTargetSize,hy=p.globalTargetSize,hz=p.globalTargetSize;
    hx=std::min({hx,local(p,g.yMin,hx),local(p,g.yMax,hx),local(p,g.zMin,hx),local(p,g.zMax,hx)});
    hy=std::min({hy,local(p,g.xMin,hy),local(p,g.xMax,hy),local(p,g.zMin,hy),local(p,g.zMax,hy)});
    hz=std::min({hz,local(p,g.xMin,hz),local(p,g.xMax,hz),local(p,g.yMin,hz),local(p,g.yMax,hz)});
    StructuredHexMesherOptions o;o.nx=divisions(lx,hx);o.ny=divisions(ly,hy);o.nz=divisions(lz,hz);return o;
}
} // namespace femcae::meshing
