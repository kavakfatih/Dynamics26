#include "femcae/meshing/StructuredHexMesher.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <unordered_map>

namespace femcae::meshing {
namespace {

std::size_t index3(const std::size_t i, const std::size_t j, const std::size_t k,
                   const std::size_t nx, const std::size_t ny) {
    return i + (nx + 1) * (j + (ny + 1) * k);
}

double distance(const geometry::Vec3& a, const geometry::Vec3& b) {
    const double dx = a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}

double det3(const std::array<std::array<double,3>,3>& m) {
    return m[0][0]*(m[1][1]*m[2][2]-m[1][2]*m[2][1])
         - m[0][1]*(m[1][0]*m[2][2]-m[1][2]*m[2][0])
         + m[0][2]*(m[1][0]*m[2][1]-m[1][1]*m[2][0]);
}

} // namespace

SimulationMesh StructuredHexMesher::meshBox(const AxisAlignedBox& box,
                                             const BoxBoundaryGeometry& geometry,
                                             const std::uint64_t sourceGeometryRevision,
                                             const StructuredHexMesherOptions& options) const {
    if (options.nx == 0 || options.ny == 0 || options.nz == 0) {
        throw std::invalid_argument("Structured HEX8 element sayilari sifirdan buyuk olmali.");
    }
    if (!(box.max.x > box.min.x && box.max.y > box.min.y && box.max.z > box.min.z)) {
        throw std::invalid_argument("Structured HEX8 box min/max sinirlari gecersiz.");
    }
    SimulationMesh mesh;
    mesh.sourceGeometryRevision = sourceGeometryRevision;
    const double dx = (box.max.x-box.min.x)/static_cast<double>(options.nx);
    const double dy = (box.max.y-box.min.y)/static_cast<double>(options.ny);
    const double dz = (box.max.z-box.min.z)/static_cast<double>(options.nz);
    const std::size_t nodeCount = (options.nx+1)*(options.ny+1)*(options.nz+1);
    mesh.nodes.reserve(nodeCount);
    std::vector<MeshEntityId> gridNodeIds(nodeCount, InvalidMeshId);
    MeshEntityId nextNode = options.firstNodeId;
    for (std::size_t k=0; k<=options.nz; ++k) {
        for (std::size_t j=0; j<=options.ny; ++j) {
            for (std::size_t i=0; i<=options.nx; ++i) {
                geometry::GeometryEntityId source = geometry.body;
                const int boundaryCount = static_cast<int>(i==0 || i==options.nx) + static_cast<int>(j==0 || j==options.ny) + static_cast<int>(k==0 || k==options.nz);
                if (boundaryCount == 1) {
                    if (i==0) source=geometry.xMin; else if (i==options.nx) source=geometry.xMax;
                    else if (j==0) source=geometry.yMin; else if (j==options.ny) source=geometry.yMax;
                    else if (k==0) source=geometry.zMin; else source=geometry.zMax;
                }
                MeshNode node;
                node.id = nextNode++;
                node.x = {box.min.x+dx*static_cast<double>(i), box.min.y+dy*static_cast<double>(j), box.min.z+dz*static_cast<double>(k)};
                node.sourceGeometryId = source;
                const auto idx=index3(i,j,k,options.nx,options.ny);
                gridNodeIds[idx]=node.id;
                mesh.nodes.push_back(node);
            }
        }
    }

    MeshEntityId nextElement=options.firstElementId;
    MeshEntityId nextFacet=options.firstFacetId;
    auto nid=[&](std::size_t i,std::size_t j,std::size_t k){return gridNodeIds[index3(i,j,k,options.nx,options.ny)];};
    auto addFacet=[&](std::array<MeshEntityId,4> nodes, MeshEntityId owner, geometry::GeometryEntityId gid){
        mesh.boundaryFacets.push_back(MeshFacet{nextFacet++,nodes,owner,gid});
    };
    for (std::size_t k=0; k<options.nz; ++k) {
        for (std::size_t j=0; j<options.ny; ++j) {
            for (std::size_t i=0; i<options.nx; ++i) {
                MeshElement e;
                e.id=nextElement++;
                e.topology=MeshTopology::Hex8;
                // FEMCAE/standard HEX8 natural-coordinate ordering.
                e.nodeIds={nid(i,j,k),nid(i+1,j,k),nid(i+1,j+1,k),nid(i,j+1,k),
                           nid(i,j,k+1),nid(i+1,j,k+1),nid(i+1,j+1,k+1),nid(i,j+1,k+1)};
                e.sourceGeometryId=geometry.body;
                mesh.elements.push_back(e);
                if (i==0) addFacet({e.nodeIds[0],e.nodeIds[3],e.nodeIds[7],e.nodeIds[4]},e.id,geometry.xMin);
                if (i+1==options.nx) addFacet({e.nodeIds[1],e.nodeIds[5],e.nodeIds[6],e.nodeIds[2]},e.id,geometry.xMax);
                if (j==0) addFacet({e.nodeIds[0],e.nodeIds[4],e.nodeIds[5],e.nodeIds[1]},e.id,geometry.yMin);
                if (j+1==options.ny) addFacet({e.nodeIds[3],e.nodeIds[2],e.nodeIds[6],e.nodeIds[7]},e.id,geometry.yMax);
                if (k==0) addFacet({e.nodeIds[0],e.nodeIds[1],e.nodeIds[2],e.nodeIds[3]},e.id,geometry.zMin);
                if (k+1==options.nz) addFacet({e.nodeIds[4],e.nodeIds[7],e.nodeIds[6],e.nodeIds[5]},e.id,geometry.zMax);
            }
        }
    }
    return mesh;
}

MeshQuality evaluateHexMeshQuality(const SimulationMesh& mesh) {
    MeshQuality q;
    q.minimumScaledJacobian=std::numeric_limits<double>::infinity();
    q.maximumAspectRatio=0.0;
    for (const auto& e : mesh.elements) {
        if (e.topology != MeshTopology::Hex8) continue;
        std::array<geometry::Vec3,8> x{};
        bool missing=false;
        for (std::size_t a=0;a<8;++a) {
            const auto* n=mesh.findNode(e.nodeIds[a]);
            if (!n) { missing=true; break; }
            x[a]=n->x;
        }
        if (missing) { ++q.degenerateElementCount; q.minimumScaledJacobian=std::min(q.minimumScaledJacobian,0.0); continue; }
        // Center Jacobian; dN/dxi at (0,0,0) = sign/8.
        constexpr double s[8][3]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
        std::array<std::array<double,3>,3> j{};
        for (int a=0;a<8;++a) {
            const double coord[3]={x[a].x,x[a].y,x[a].z};
            for (int r=0;r<3;++r) for (int c=0;c<3;++c) j[r][c]+=coord[r]*s[a][c]/8.0;
        }
        const double det=det3(j);
        const double c0=std::sqrt(j[0][0]*j[0][0]+j[1][0]*j[1][0]+j[2][0]*j[2][0]);
        const double c1=std::sqrt(j[0][1]*j[0][1]+j[1][1]*j[1][1]+j[2][1]*j[2][1]);
        const double c2=std::sqrt(j[0][2]*j[0][2]+j[1][2]*j[1][2]+j[2][2]*j[2][2]);
        const double denom=c0*c1*c2;
        const double scaled=denom>0.0?det/denom:0.0;
        q.minimumScaledJacobian=std::min(q.minimumScaledJacobian,scaled);
        if (det<0.0) ++q.invertedElementCount;
        if (std::abs(det)<=std::numeric_limits<double>::epsilon()*std::max({1.0,denom})) ++q.degenerateElementCount;
        constexpr int edges[12][2]={{0,1},{1,2},{2,3},{3,0},{4,5},{5,6},{6,7},{7,4},{0,4},{1,5},{2,6},{3,7}};
        double minEdge=std::numeric_limits<double>::infinity(), maxEdge=0.0;
        for (const auto& edge:edges) { const double l=distance(x[edge[0]],x[edge[1]]); minEdge=std::min(minEdge,l); maxEdge=std::max(maxEdge,l); }
        const double aspect=minEdge>0.0?maxEdge/minEdge:std::numeric_limits<double>::infinity();
        q.maximumAspectRatio=std::max(q.maximumAspectRatio,aspect);
    }
    if (!std::isfinite(q.minimumScaledJacobian)) q.minimumScaledJacobian=0.0;
    return q;
}

} // namespace femcae::meshing
