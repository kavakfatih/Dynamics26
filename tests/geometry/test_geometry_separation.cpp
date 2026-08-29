#include "femcae/geometry/GeometryDocument.h"

#include <cstdlib>
#include <iostream>

using namespace femcae::geometry;
static void require(bool c, const char* m) { if (!c) { std::cerr << "FAIL: " << m << '\n'; std::exit(1); } }

int main() {
    GeometryDocument doc("separation-test");
    const auto body = doc.addEntity(GeometryEntityKind::Body, InvalidGeometryId, "Body", "body/1");
    const auto revisionBefore = doc.revision();

    GeometryTessellation display;
    display.sourceGeometryId = body;
    display.sourceRevision = revisionBefore;
    display.points = {{0,0,0},{1,0,0},{0,1,0}};
    display.triangles = {{0,1,2}};

    GeometryAssociationMap provenance;
    provenance.set(GeometryAssociation{body, {101, 102, 103}, {501}, {9001}});
    require(doc.revision() == revisionBefore, "display tessellation/provenance CAD document'i mutate etmez");
    require(display.sourceGeometryId == body, "tessellation CAD provenance");
    require(provenance.find(body) != nullptr, "FEM association ayrik registry");
    require(provenance.find(body)->femNodeIds.front() == 101, "FEM node IDs yalniz association map'te");

    display.points[0].x = 99.0;
    require(doc.revision() == revisionBefore, "display mesh degisikligi CAD revision'i degistirmez");

    std::cout << "PASS V0.12 CAD != tessellation != FEM mesh contract\n";
    return 0;
}
