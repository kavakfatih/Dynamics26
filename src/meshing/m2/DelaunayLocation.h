#pragma once

#include "DelaunayTopology.h"

namespace femcae::meshing::m2 {

enum class DelaunayLocationKind : std::uint8_t {
    Cell = 0,
    Facet = 1,
    Edge = 2,
    Vertex = 3,
    OutsideConvexHull = 4
};

struct LocatedCellEvidence {
    // Kimlik allocation sirasindan bagimsizdir; handle yalniz bu snapshot icindir.
    std::array<DelaunayVertexRef, 4> canonicalVertices;
    DelaunayCellHandle handle;
};

struct ViolatedHullWitness {
    std::array<PointId, 3> canonicalFace;
    std::array<PointId, 3> outwardFace;
    DelaunayCellHandle finiteCell;
    DelaunayCellHandle ghostCell;
};

struct DelaunayLocationResult {
    DelaunayLocationKind kind{DelaunayLocationKind::OutsideConvexHull};
    // Artan PointId: Cell=4, Facet=3, Edge=2, Vertex=1, Outside=0.
    std::vector<PointId> entityVertices;
    // Entity'nin tum live finite/ghost incident hucreleri, canonical key sirasinda.
    std::vector<LocatedCellEvidence> incidentCells;
    std::optional<ViolatedHullWitness> outsideWitness;
    std::size_t finiteCellsTested{0};
};

// P1C correctness oracle: her live finite tetra exact Orient3D ile incelenir.
// Giris bir immutable, pozitif, convex-hull triangulation snapshot'i olmalidir.
// sites henuz eklenmemis canonical siteleri de icerebilir; konum yalniz live
// hucreler uzerindedir. NaN/Inf, gecersiz site/topoloji ve bos finite kompleks
// invalid_argument; tutarsiz containment veya hull kaniti logic_error uretir.
// Genel embedding/S3 ispatinin yerini almaz. Referanslar snapshot degisince atilir.
// Mutation, symbolic tie, walk, epsilon veya spatial acceleration yoktur.
[[nodiscard]] DelaunayLocationResult locateBruteForceExact(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites,
    const geometry::Vec3& query);

} // namespace femcae::meshing::m2
