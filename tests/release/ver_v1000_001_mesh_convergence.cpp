#include <femcae/femcae.h>
#include <femcae/meshing/StructuredHexMesher.h>

#include <array>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {
struct Result { double tip{}; double reaction{}; std::size_t elements{}; };

Result solveCantilever(int nx, int ny, int nz) {
    using namespace femcae::meshing;
    StructuredHexMesher mesher;
    BoxBoundaryGeometry geometry{100,101,102,103,104,105,106};
    StructuredHexMesherOptions options; options.nx=nx; options.ny=ny; options.nz=nz;
    auto mesh=mesher.meshBox({{0,0,0},{4,1,1}},geometry,77,options);

    std::vector<long long> nodeIds, elementIds, connectivity;
    std::vector<double> xyz;
    for(const auto& n:mesh.nodes){nodeIds.push_back(n.id);xyz.insert(xyz.end(),{n.x.x,n.x.y,n.x.z});}
    for(const auto& e:mesh.elements){elementIds.push_back(e.id);for(auto id:e.nodeIds)connectivity.push_back(id);}

    std::vector<long long> constraintNodes, loadNodes;
    std::vector<int> constraintComponents, loadComponents;
    std::vector<double> constraintValues, loadValues;
    int endNodeCount=0;
    for(const auto& n:mesh.nodes) if(std::abs(n.x.x-4.0)<1e-12) ++endNodeCount;
    for(const auto& n:mesh.nodes){
        if(std::abs(n.x.x)<1e-12){
            for(int component=1;component<=3;++component){
                constraintNodes.push_back(n.id);constraintComponents.push_back(component);constraintValues.push_back(0.0);
            }
        }
        if(std::abs(n.x.x-4.0)<1e-12){
            loadNodes.push_back(n.id);loadComponents.push_back(3);loadValues.push_back(-1.0/endNodeCount);
        }
    }

    std::vector<double> u(3*mesh.nodes.size()), reactions(3*mesh.nodes.size()), vm(mesh.elements.size());
    const int rc=fem_solve_linear_hex8_mesh(static_cast<int>(mesh.nodes.size()),nodeIds.data(),xyz.data(),
        static_cast<int>(mesh.elements.size()),elementIds.data(),connectivity.data(),1000.0,0.30,
        static_cast<int>(constraintNodes.size()),constraintNodes.data(),constraintComponents.data(),constraintValues.data(),
        static_cast<int>(loadNodes.size()),loadNodes.data(),loadComponents.data(),loadValues.data(),
        u.data(),reactions.data(),vm.data());
    assert(rc==0);

    double tip=0.0,reaction=0.0;int tipCount=0;
    for(std::size_t i=0;i<mesh.nodes.size();++i){
        if(std::abs(mesh.nodes[i].x.x-4.0)<1e-12){tip+=u[3*i+2];++tipCount;}
        reaction+=reactions[3*i+2];
    }
    return {tip/tipCount,reaction,mesh.elements.size()};
}
}

int main(){
    // Euler-Bernoulli cantilever reference: delta = F L^3 /(3 E I), I=b h^3/12.
    constexpr double exact=-0.256; // F=-1, L=4, E=1000, b=h=1.
    const std::array<std::array<int,3>,3> levels{{{{4,1,1}},{{8,2,2}},{{12,3,3}}}};
    std::array<double,3> errors{};
    for(std::size_t i=0;i<levels.size();++i){
        const auto r=solveCantilever(levels[i][0],levels[i][1],levels[i][2]);
        errors[i]=std::abs((r.tip-exact)/exact);
        assert(std::abs(r.reaction-1.0)<1e-8);
        std::cout<<"mesh="<<levels[i][0]<<"x"<<levels[i][1]<<"x"<<levels[i][2]
                 <<" elements="<<r.elements<<" tip="<<r.tip<<" rel_error="<<errors[i]<<'\n';
    }
    assert(errors[1] < errors[0]);
    assert(errors[2] < errors[1]);
    assert(errors[2] < 0.05); // fine structured mesh: <5% against independent beam reference.
    std::cout<<"PASS V1.0 HEX8 cantilever mesh convergence\n";
}
