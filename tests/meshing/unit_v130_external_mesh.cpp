#include <femcae/meshing/AbaqusInpMeshReader.h>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(){
    const auto p=std::filesystem::temp_directory_path()/"femcae_v130_mesh.inp";
    std::ofstream o(p);
    o<<"*NODE\n1,0,0,0\n2,1,0,0\n3,1,1,0\n4,0,1,0\n5,0,0,1\n6,1,0,1\n7,1,1,1\n8,0,1,1\n";
    o<<"*ELEMENT, TYPE=C3D8\n20,1,2,3,4,5,6,7,8\n";o.close();
    femcae::meshing::AbaqusInpMeshReader reader; const auto m=reader.read(p);
    assert(m.nodes.size()==8 && m.elements.size()==1 && m.elements[0].id==20);
    std::filesystem::remove(p);
    std::cout<<"V0.13 external Abaqus C3D8 mesh PASS\n";
}
