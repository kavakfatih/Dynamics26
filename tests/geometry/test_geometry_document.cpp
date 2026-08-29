#include "femcae/geometry/GeometryDocument.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

using namespace femcae::geometry;

static void require(bool condition, const char* message) {
    if (!condition) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

int main() {
    GeometryDocument a("project-guid-001");
    const auto asmId = a.addEntity(GeometryEntityKind::Assembly, InvalidGeometryId, "Assembly", "root/A");
    const auto bodyId = a.addEntity(GeometryEntityKind::Body, asmId, "Body", "root/A/body/1");
    const auto faceId = a.addEntity(GeometryEntityKind::Face, bodyId, "Face", "root/A/body/1/face/1");
    require(a.size() == 3, "geometry entity count");
    require(a.childrenOf(bodyId).size() == 1, "body child face");
    require(a.entitiesOfKind(GeometryEntityKind::Face).front() == faceId, "face lookup");

    GeometryDocument b("project-guid-001");
    const auto asmId2 = b.addEntity(GeometryEntityKind::Assembly, InvalidGeometryId, "Assembly", "root/A");
    const auto bodyId2 = b.addEntity(GeometryEntityKind::Body, asmId2, "Body", "root/A/body/1");
    const auto faceId2 = b.addEntity(GeometryEntityKind::Face, bodyId2, "Face", "root/A/body/1/face/1");
    require(asmId == asmId2 && bodyId == bodyId2 && faceId == faceId2, "persistent ids deterministic");

    bool duplicateRejected = false;
    try { (void)b.addEntity(GeometryEntityKind::Face, bodyId2, "Duplicate", "root/A/body/1/face/1"); }
    catch (const std::invalid_argument&) { duplicateRejected = true; }
    require(duplicateRejected, "duplicate persistent key rejected");

    GeometryDocument different("other-project-guid");
    const auto other = different.addEntity(GeometryEntityKind::Assembly, InvalidGeometryId, "Assembly", "root/A");
    require(other != asmId, "document namespace participates in ID");

    std::cout << "PASS V0.12 geometry document/persistent IDs\n";
    return 0;
}
