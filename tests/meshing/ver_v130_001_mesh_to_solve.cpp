#include <femcae/femcae.h>
#include <femcae/meshing/StructuredHexMesher.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
int main(){
    using namespace femcae::meshing;
    StructuredHexMesher mesher;BoxBoundaryGeometry g{100,101,102,103,104,105,106};
    auto m=mesher.meshBox({{0,0,0},{1,1,1}},g,9,{});
    std::vector<long long> nids;std::vector<double> xyz;for(const auto&n:m.nodes){nids.push_back(n.id);xyz.insert(xyz.end(),{n.x.x,n.x.y,n.x.z});}
    std::vector<long long> eids,conn;for(const auto&e:m.elements){eids.push_back(e.id);for(auto id:e.nodeIds)conn.push_back(id);}
    std::vector<long long> cn;std::vector<int> cc;std::vector<double> cv;
    // x-min face: ux=0. Minimum additional constraints remove rigid y/z motion and x-axis rotation.
    for(const auto&n:m.nodes) if(std::abs(n.x.x)<1e-12){cn.push_back(n.id);cc.push_back(1);cv.push_back(0.0);}
    const auto n000=m.nodes.front().id;cn.push_back(n000);cc.push_back(2);cv.push_back(0);cn.push_back(n000);cc.push_back(3);cv.push_back(0);
    long long n010=-1;for(const auto&n:m.nodes)if(std::abs(n.x.x)<1e-12&&std::abs(n.x.y-1)<1e-12&&std::abs(n.x.z)<1e-12)n010=n.id;
    assert(n010>0);cn.push_back(n010);cc.push_back(3);cv.push_back(0);
    std::vector<long long> ln;std::vector<int> lc;std::vector<double> lv;int faceCount=0;for(const auto&n:m.nodes)if(std::abs(n.x.x-1)<1e-12)++faceCount;
    for(const auto&n:m.nodes)if(std::abs(n.x.x-1)<1e-12){ln.push_back(n.id);lc.push_back(1);lv.push_back(100.0/faceCount);}
    std::vector<double> u(3*m.nodes.size()),r(3*m.nodes.size()),vm(m.elements.size());
    const int rc=fem_solve_linear_hex8_mesh((int)m.nodes.size(),nids.data(),xyz.data(),(int)m.elements.size(),eids.data(),conn.data(),1000.0,0.3,
        (int)cn.size(),cn.data(),cc.data(),cv.data(),(int)ln.size(),ln.data(),lc.data(),lv.data(),u.data(),r.data(),vm.data());
    assert(rc==0);
    double tip=0,reaction=0;int ntip=0;for(std::size_t i=0;i<m.nodes.size();++i){if(std::abs(m.nodes[i].x.x-1)<1e-12){tip+=u[3*i];++ntip;}reaction+=r[3*i];}
    tip/=ntip;
    assert(std::abs(tip-0.1)<5e-8); // F L /(E A) = 100*1/(1000*1)
    assert(std::abs(reaction+100.0)<1e-7);
    assert(std::abs(vm[0]-100.0)<1e-6);
    std::cout<<"V0.13 CAD-independent mesh -> Fortran solve -> stress PASS\n";
}
