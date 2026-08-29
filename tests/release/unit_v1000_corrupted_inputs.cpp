#include <femcae/geometry/DxfSectionReader.h>
#include <femcae/meshing/AbaqusInpMeshReader.h>

#include <cassert>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>

namespace {
bool throws(const std::function<void()>& f){try{f();}catch(...){return true;}return false;}
void write(const std::filesystem::path& p,const char* text){std::ofstream out(p);out<<text;}
}

int main(){
    namespace fs=std::filesystem;
    const auto dir=fs::temp_directory_path();
    femcae::meshing::AbaqusInpMeshReader inp;

    const auto duplicate=dir/"femcae_v1000_duplicate.inp";
    write(duplicate,"*NODE\n1,0,0,0\n1,1,0,0\n*ELEMENT, TYPE=C3D8\n10,1,1,1,1,1,1,1,1\n");
    assert(throws([&]{(void)inp.read(duplicate);})); fs::remove(duplicate);

    const auto undefined=dir/"femcae_v1000_undefined.inp";
    write(undefined,"*NODE\n1,0,0,0\n2,1,0,0\n3,1,1,0\n4,0,1,0\n5,0,0,1\n6,1,0,1\n7,1,1,1\n8,0,1,1\n*ELEMENT, TYPE=C3D8\n10,1,2,3,4,5,6,7,999\n");
    assert(throws([&]{(void)inp.read(undefined);})); fs::remove(undefined);

    const auto malformed=dir/"femcae_v1000_bad_count.inp";
    write(malformed,"*NODE\n1,0,0,0\n*ELEMENT, TYPE=C3D8\n10,1,1,1\n");
    assert(throws([&]{(void)inp.read(malformed);})); fs::remove(malformed);

    femcae::geometry::DxfSectionReader dxf;
    const auto openContour=dir/"femcae_v1000_open.dxf";
    write(openContour,"0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n20\n0\n11\n1\n21\n0\n0\nENDSEC\n0\nEOF\n");
    assert(!dxf.readFile(openContour.string()).success); fs::remove(openContour);

    const auto truncated=dir/"femcae_v1000_truncated.dxf";
    write(truncated,"0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n20\n");
    assert(!dxf.readFile(truncated.string()).success); fs::remove(truncated);

    const auto bulge=dir/"femcae_v1000_bulge.dxf";
    write(bulge,"0\nSECTION\n2\nENTITIES\n0\nLWPOLYLINE\n70\n1\n10\n0\n20\n0\n42\n1\n10\n1\n20\n0\n10\n0\n20\n1\n0\nENDSEC\n0\nEOF\n");
    assert(!dxf.readFile(bulge.string()).success); fs::remove(bulge);

    std::cout<<"PASS V1.0 corrupted Abaqus/DXF inputs rejected\n";
}
