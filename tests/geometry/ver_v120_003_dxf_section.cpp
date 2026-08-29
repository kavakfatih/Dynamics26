#include "femcae/geometry/DxfSectionReader.h"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace femcae::geometry;
static void require(bool c,const char*m){if(!c){std::cerr<<"FAIL: "<<m<<'\n';std::exit(1);}}
static void near(double a,double b,double tol,const char*m){if(std::abs(a-b)>tol){std::cerr<<"FAIL: "<<m<<" got="<<a<<" ref="<<b<<'\n';std::exit(1);}}

static void lwpoly(std::ofstream& f, double x0,double y0,double x1,double y1) {
    f << "0\nLWPOLYLINE\n90\n4\n70\n1\n"
      << "10\n"<<x0<<"\n20\n"<<y0<<"\n"
      << "10\n"<<x1<<"\n20\n"<<y0<<"\n"
      << "10\n"<<x1<<"\n20\n"<<y1<<"\n"
      << "10\n"<<x0<<"\n20\n"<<y1<<"\n";
}

int main(){
    const auto path=std::filesystem::temp_directory_path()/"femcae_v120_section.dxf";
    {
        std::ofstream f(path);
        f << "0\nSECTION\n2\nENTITIES\n";
        lwpoly(f,-0.05,-0.03,0.05,0.03);
        lwpoly(f,-0.04,-0.02,0.04,0.02);
        f << "0\nENDSEC\n0\nEOF\n";
    }
    DxfSectionReader reader;
    const auto result=reader.readFile(path.string());
    std::filesystem::remove(path);
    require(result.success, result.message.c_str());
    require(result.profile.contours().size()==2,"DXF two contours");
    const auto p=result.profile.properties();
    near(p.area,0.1*0.06-0.08*0.04,1e-12,"DXF hollow area");
    near(p.ixx,(0.1*std::pow(0.06,3)-0.08*std::pow(0.04,3))/12.0,1e-12,"DXF hollow Ixx");

    const auto openPath=std::filesystem::temp_directory_path()/"femcae_v120_open.dxf";
    {
        std::ofstream f(openPath);
        f<<"0\nSECTION\n2\nENTITIES\n0\nLINE\n10\n0\n20\n0\n11\n1\n21\n0\n0\nENDSEC\n0\nEOF\n";
    }
    const auto open=reader.readFile(openPath.string());
    std::filesystem::remove(openPath);
    require(!open.success,"open DXF contour rejected");
    std::cout<<"PASS VER-V120-003 DXF custom section import\n";
}
