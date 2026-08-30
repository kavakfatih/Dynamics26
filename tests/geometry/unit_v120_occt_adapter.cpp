#include "femcae/geometry/OcctStepImporter.h"

#include <exception>
#include <iostream>

using namespace femcae::geometry;

int main(){
    OcctStepImporter importer;
    if (!OcctStepImporter::available()) {
        GeometryDocument doc("no-occt");
        const auto r=importer.importFile("does-not-exist.step",doc);
        if(r.success){std::cerr<<"FAIL: stub import success olamaz\n";return 1;}

        bool topologyRejected=false;
        try {
            (void)importer.tessellateWithTopology(1,0.5);
        } catch (const std::exception&) {
            topologyRejected=true;
        }
        if(!topologyRejected){
            std::cerr<<"FAIL: OCCT olmayan build topology tessellation'i reddetmeli\n";
            return 1;
        }
    }
    std::cout<<"PASS V0.12 OCCT adapter availability/topology contract\n";
}
