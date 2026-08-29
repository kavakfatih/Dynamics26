#include "femcae/geometry/DxfSectionReader.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>

using namespace femcae::geometry;
static void require(bool c,const char*m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
static void near(double a,double b,double rel,const char*m){const double scale=std::max(1.0,std::abs(b));if(std::abs(a-b)>rel*scale){std::cerr<<"FAIL: "<<m<<" got="<<a<<" ref="<<b<<'\n';std::exit(1);}}

int main(){
    DxfSectionReader reader;
    const auto circlePath=std::filesystem::temp_directory_path()/"femcae_v120_circle.dxf";
    {
        std::ofstream f(circlePath);
        f<<"0\nSECTION\n2\nENTITIES\n0\nCIRCLE\n10\n0\n20\n0\n40\n0.025\n0\nENDSEC\n0\nEOF\n";
    }
    DxfSectionOptions opt; opt.circleSegments=512;
    const auto circle=reader.readFile(circlePath.string(),opt); std::filesystem::remove(circlePath);
    require(circle.success,circle.message.c_str());
    const auto cp=circle.profile.properties();
    near(cp.area,std::numbers::pi*0.025*0.025,2.0e-5,"circle area approximation");

    const auto linePath=std::filesystem::temp_directory_path()/"femcae_v120_lines.dxf";
    {
        std::ofstream f(linePath);
        f<<"0\nSECTION\n2\nENTITIES\n";
        auto line=[&](double x1,double y1,double x2,double y2){f<<"0\nLINE\n10\n"<<x1<<"\n20\n"<<y1<<"\n11\n"<<x2<<"\n21\n"<<y2<<"\n";};
        line(0,0,2,0); line(2,0,2,1); line(2,1,0,1); line(0,1,0,0);
        f<<"0\nENDSEC\n0\nEOF\n";
    }
    const auto lines=reader.readFile(linePath.string()); std::filesystem::remove(linePath);
    require(lines.success,lines.message.c_str());
    near(lines.profile.properties().area,2.0,1e-12,"LINE chain area");

    // Iki ARC ile tam daire: arc sampling + endpoint joining yolunu dogrular.
    const auto arcPath=std::filesystem::temp_directory_path()/"femcae_v120_arcs.dxf";
    {
        std::ofstream f(arcPath);
        f<<"0\nSECTION\n2\nENTITIES\n"
         <<"0\nARC\n10\n0\n20\n0\n40\n1\n50\n0\n51\n180\n"
         <<"0\nARC\n10\n0\n20\n0\n40\n1\n50\n180\n51\n360\n"
         <<"0\nENDSEC\n0\nEOF\n";
    }
    DxfSectionOptions arcOpt; arcOpt.arcMaxAngleDeg=1.0;
    const auto arcs=reader.readFile(arcPath.string(),arcOpt); std::filesystem::remove(arcPath);
    require(arcs.success,arcs.message.c_str());
    near(arcs.profile.properties().area,std::numbers::pi,1.0e-4,"ARC loop area approximation");

    std::cout<<"PASS V0.12 DXF LINE/CIRCLE/ARC paths\n";
}
