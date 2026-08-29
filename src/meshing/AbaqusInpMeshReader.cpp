#include "femcae/meshing/AbaqusInpMeshReader.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace femcae::meshing {
namespace {
std::string trim(std::string s) {
    const auto first=s.find_first_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    const auto last=s.find_last_not_of(" \t\r\n");
    return s.substr(first,last-first+1);
}
std::string upper(std::string s){for(char& c:s)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));return s;}
std::vector<std::string> splitComma(const std::string& line){std::vector<std::string> v;std::stringstream ss(line);std::string p;while(std::getline(ss,p,','))v.push_back(trim(p));return v;}
}

SimulationMesh AbaqusInpMeshReader::read(const std::filesystem::path& path) const {
    std::ifstream in(path);
    if(!in) throw std::runtime_error("Abaqus INP dosyasi acilamadi: "+path.string());
    enum class Mode{None,Node,Hex8}; Mode mode=Mode::None;
    SimulationMesh mesh;
    std::string line;
    while(std::getline(in,line)){
        line=trim(line); if(line.empty()||line.rfind("**",0)==0) continue;
        if(line[0]=='*'){
            const auto u=upper(line);
            if(u.rfind("*NODE",0)==0) mode=Mode::Node;
            else if(u.rfind("*ELEMENT",0)==0 && u.find("TYPE=C3D8")!=std::string::npos) mode=Mode::Hex8;
            else mode=Mode::None;
            continue;
        }
        const auto p=splitComma(line);
        if(mode==Mode::Node){
            if(p.size()<4) throw std::runtime_error("Abaqus *NODE satiri en az id,x,y,z icermeli.");
            MeshNode n; n.id=std::stoll(p[0]); n.x={std::stod(p[1]),std::stod(p[2]),std::stod(p[3])};
            if(mesh.findNode(n.id)) throw std::runtime_error("Abaqus mesh duplicate node ID.");
            mesh.nodes.push_back(n);
        } else if(mode==Mode::Hex8){
            if(p.size()!=9) throw std::runtime_error("Abaqus C3D8 satiri id + 8 node ID icermeli.");
            MeshElement e; e.id=std::stoll(p[0]); e.topology=MeshTopology::Hex8;
            for(std::size_t i=0;i<8;++i)e.nodeIds[i]=std::stoll(p[i+1]);
            if(mesh.findElement(e.id)) throw std::runtime_error("Abaqus mesh duplicate element ID.");
            mesh.elements.push_back(e);
        }
    }
    if(mesh.nodes.empty()||mesh.elements.empty()) throw std::runtime_error("Abaqus INP icinde desteklenen node/C3D8 mesh bulunamadi.");
    for(const auto& e:mesh.elements) for(const auto id:e.nodeIds) if(!mesh.findNode(id)) throw std::runtime_error("Abaqus C3D8 connectivity tanimsiz node ID iceriyor.");
    return mesh;
}

} // namespace femcae::meshing
