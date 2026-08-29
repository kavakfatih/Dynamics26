#include <femcae/femcae.h>
#include <femcae/meshing/StructuredHexMesher.h>

#include <cassert>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>

int main(){
    using namespace femcae::meshing;
    StructuredHexMesher mesher; BoxBoundaryGeometry g{200,201,202,203,204,205,206};
    StructuredHexMesherOptions o; o.nx=12;o.ny=3;o.nz=3;
    auto mesh=mesher.meshBox({{0,0,0},{4,1,1}},g,91,o);
    std::vector<long long> nids,eids,conn,cn,ln; std::vector<int> cc,lc; std::vector<double> xyz,cv,lv;
    for(const auto& n:mesh.nodes){nids.push_back(n.id);xyz.insert(xyz.end(),{n.x.x,n.x.y,n.x.z});}
    for(const auto& e:mesh.elements){eids.push_back(e.id);for(auto id:e.nodeIds)conn.push_back(id);}
    int endCount=0; for(const auto& n:mesh.nodes)if(std::abs(n.x.x-4)<1e-12)++endCount;
    long long origin=-1, yCorner=-1;
    for(const auto& n:mesh.nodes){
        if(std::abs(n.x.x)<1e-12){cn.push_back(n.id);cc.push_back(1);cv.push_back(0);}
        if(std::abs(n.x.x-4)<1e-12){ln.push_back(n.id);lc.push_back(1);lv.push_back(100.0/endCount);}
        if(std::abs(n.x.x)<1e-12&&std::abs(n.x.y)<1e-12&&std::abs(n.x.z)<1e-12)origin=n.id;
        if(std::abs(n.x.x)<1e-12&&std::abs(n.x.y-1)<1e-12&&std::abs(n.x.z)<1e-12)yCorner=n.id;
    }
    assert(origin>0&&yCorner>0);
    cn.push_back(origin);cc.push_back(2);cv.push_back(0); cn.push_back(origin);cc.push_back(3);cv.push_back(0);
    cn.push_back(yCorner);cc.push_back(3);cv.push_back(0);
    std::vector<double> u(3*mesh.nodes.size()),r(3*mesh.nodes.size()),vm(mesh.elements.size());
    const auto begin=std::chrono::steady_clock::now();
    const int rc=fem_solve_linear_hex8_mesh((int)mesh.nodes.size(),nids.data(),xyz.data(),(int)mesh.elements.size(),eids.data(),conn.data(),1000,0.3,
        (int)cn.size(),cn.data(),cc.data(),cv.data(),(int)ln.size(),ln.data(),lc.data(),lv.data(),u.data(),r.data(),vm.data());
    const double seconds=std::chrono::duration<double>(std::chrono::steady_clock::now()-begin).count();
    assert(rc==0);
    double tip=0,reaction=0;int nt=0;for(std::size_t i=0;i<mesh.nodes.size();++i){if(std::abs(mesh.nodes[i].x.x-4)<1e-12){tip+=u[3*i];++nt;}reaction+=r[3*i];}
    tip/=nt; assert(std::abs((tip-0.4)/0.4)<0.05); assert(std::abs(reaction+100)<1e-7);
    // Bu esik bir performans iddiasi degildir; yalnizca ciddi runtime regresyonunu yakalayan genis smoke guard'dir.
    assert(seconds<30.0);
    std::cout<<"V1.0 performance smoke nodes="<<mesh.nodes.size()<<" elements="<<mesh.elements.size()
             <<" active_dof_approx="<<3*mesh.nodes.size()-cn.size()<<" elapsed_seconds="<<seconds<<'\n';
}
