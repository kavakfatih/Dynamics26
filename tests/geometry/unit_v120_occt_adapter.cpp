#include "femcae/geometry/OcctStepImporter.h"
#include <cstdlib>
#include <iostream>
using namespace femcae::geometry;
int main(){
    OcctStepImporter importer;
    if (!OcctStepImporter::available()) {
        GeometryDocument doc("no-occt");
        const auto r=importer.importFile("does-not-exist.step",doc);
        if(r.success){std::cerr<<"FAIL: stub import success olamaz\n";return 1;}
    }
    std::cout<<"PASS V0.12 OCCT adapter availability contract\n";
}
