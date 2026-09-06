#pragma once

#include "femcae/geometry/GeometryTypes.h"
#include "femcae/meshing/RobustGeometry.h"
#include "femcae/meshing/RobustPredicates.h"

#include <array>
#include <cstdint>

namespace femcae::meshing::m2 {

struct IndexedPoint2 {
    PointId id{InvalidPointId};
    geometry::Vec2 point;
};

struct IndexedPoint3 {
    PointId id{InvalidPointId};
    geometry::Vec3 point;
};

struct ResolvedDelaunaySign {
    predicates::PredicateSign geometricSign{predicates::PredicateSign::Zero};
    predicates::PredicateSign resolvedSign{predicates::PredicateSign::Zero};

    [[nodiscard]] bool usedSymbolicTie() const noexcept {
        return geometricSign == predicates::PredicateSign::Zero;
    }
};

enum class DelaunayConflict : std::uint8_t {
    NoConflict = 0,
    Conflict = 1
};

// D26LIFT1: ham InSphere sifiriysa yalniz formal lift koordinati
// PointId onceligine gore perturbe edilir. x/y/z koordinatlari degismez.
[[nodiscard]] ResolvedDelaunaySign resolveLiftOnlyInsphere(
    const std::array<IndexedPoint3, 5>& orderedSites);

// D26LIFT1'in 2B karsiligi. Bu fonksiyon determinant isaretini cozer;
// pozitif ucgen semantigi caller tarafinda kurulabilir.
[[nodiscard]] ResolvedDelaunaySign resolveLiftOnlyIncircle(
    const std::array<IndexedPoint2, 4>& orderedSites);

// Pozitif saklanan finite tetra icin InSphere Positive = conflict.
[[nodiscard]] DelaunayConflict classifyFiniteCellConflict(
    const std::array<IndexedPoint3, 4>& positiveTetrahedron,
    const IndexedPoint3& query);

// Query facet duzleminde olmak zorundadir. XY -> XZ -> YZ sabit sirasi
// ile ilk exact non-collinear projeksiyon secilir ve projected InCircle
// pozitif-orientasyon semantigine normalize edilir.
[[nodiscard]] ResolvedDelaunaySign classifyProjectedCoplanarCircumcircle(
    const std::array<IndexedPoint3, 3>& facet,
    const IndexedPoint3& query);

// Outward facet icin insideWitness kesin olarak negatif tarafta olmalidir.
// Query pozitif half-space'te ise ghost conflict; exact coplanar durumda
// projected circumdisk + D26LIFT1 karari kullanilir.
[[nodiscard]] DelaunayConflict classifyGhostCellConflict(
    const std::array<IndexedPoint3, 3>& outwardFacet,
    const IndexedPoint3& insideWitness,
    const IndexedPoint3& query);

} // namespace femcae::meshing::m2
