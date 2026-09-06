#include "meshing/m2/DelaunayPredicates.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace {

using femcae::geometry::Vec2;
using femcae::geometry::Vec3;
using femcae::meshing::PointId;
using femcae::meshing::m2::DelaunayConflict;
using femcae::meshing::m2::IndexedPoint2;
using femcae::meshing::m2::IndexedPoint3;
using femcae::meshing::m2::ResolvedDelaunaySign;
using femcae::meshing::predicates::PredicateSign;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

IndexedPoint3 p3(PointId id, double x, double y, double z) {
    return {id, Vec3{x, y, z}};
}

IndexedPoint2 p2(PointId id, double x, double y) {
    return {id, Vec2{x, y}};
}

int toInt(PredicateSign sign) {
    return static_cast<int>(sign);
}

template <std::size_t N>
int permutationParity(const std::array<std::size_t, N>& order) {
    std::size_t inversions = 0U;
    for (std::size_t i = 0U; i < N; ++i) {
        for (std::size_t j = i + 1U; j < N; ++j) {
            if (order[i] > order[j]) {
                ++inversions;
            }
        }
    }
    return (inversions & 1U) == 0U ? 1 : -1;
}

template <typename Site, std::size_t N>
std::array<Site, N> permuted(
    const std::array<Site, N>& sites,
    const std::array<std::size_t, N>& order) {
    std::array<Site, N> result{};
    for (std::size_t i = 0U; i < N; ++i) {
        result[i] = sites[order[i]];
    }
    return result;
}

template <typename Callable>
void requireInvalid(Callable&& callable, const std::string& message) {
    bool rejected = false;
    try {
        std::forward<Callable>(callable)();
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    require(rejected, message);
}

void verifyFiniteConflictSemantic() {
    const std::array<IndexedPoint3, 4> tetra{
        p3(2, 1.0, 0.0, 0.0),
        p3(3, 0.0, 1.0, 0.0),
        p3(4, 0.0, 0.0, 1.0),
        p3(1, 0.0, 0.0, 0.0)};

    require(
        femcae::meshing::predicates::orient3d(
            tetra[0].point, tetra[1].point, tetra[2].point, tetra[3].point).sign ==
            PredicateSign::Positive,
        "finite semantic fixture tetra is not positive");

    const IndexedPoint3 inside = p3(5, 0.25, 0.25, 0.25);
    const IndexedPoint3 outside = p3(6, 2.0, 2.0, 2.0);

    require(
        femcae::meshing::predicates::insphere(
            tetra[0].point,
            tetra[1].point,
            tetra[2].point,
            tetra[3].point,
            inside.point).sign == PredicateSign::Positive,
        "Dynamics26 positive-cell interior InSphere sign convention mismatch");
    require(
        femcae::meshing::predicates::insphere(
            tetra[0].point,
            tetra[1].point,
            tetra[2].point,
            tetra[3].point,
            outside.point).sign == PredicateSign::Negative,
        "Dynamics26 positive-cell outside InSphere sign convention mismatch");

    require(
        femcae::meshing::m2::classifyFiniteCellConflict(tetra, inside) ==
            DelaunayConflict::Conflict,
        "positive-cell interior InSphere did not conflict");

    require(
        femcae::meshing::m2::classifyFiniteCellConflict(tetra, outside) ==
            DelaunayConflict::NoConflict,
        "positive-cell outside InSphere conflicted");
}

void verifyLiftOnlyInsphere() {
    const std::array<IndexedPoint3, 5> sites{
        p3(1, 0.0, 0.0, 0.0),
        p3(2, 0.0, 0.0, 1.0),
        p3(3, 0.0, 1.0, 0.0),
        p3(4, 1.0, 0.0, 0.0),
        p3(5, 1.0, 1.0, 1.0)};

    const ResolvedDelaunaySign base =
        femcae::meshing::m2::resolveLiftOnlyInsphere(sites);
    require(base.geometricSign == PredicateSign::Zero,
            "co-spherical golden set is not exact InSphere zero");
    require(base.resolvedSign == PredicateSign::Negative,
            "D26LIFT1 co-spherical golden sign mismatch");

    const std::array<IndexedPoint3, 4> tetra{
        sites[0], sites[1], sites[2], sites[3]};
    require(
        femcae::meshing::m2::classifyFiniteCellConflict(tetra, sites[4]) ==
            DelaunayConflict::NoConflict,
        "D26LIFT1 golden finite conflict semantic mismatch");

    std::array<std::size_t, 5> order{0U, 1U, 2U, 3U, 4U};
    std::size_t count = 0U;
    do {
        const ResolvedDelaunaySign actual =
            femcae::meshing::m2::resolveLiftOnlyInsphere(
                permuted(sites, order));
        require(actual.geometricSign == PredicateSign::Zero,
                "row permutation changed exact co-spherical truth");
        require(
            toInt(actual.resolvedSign) ==
                toInt(base.resolvedSign) * permutationParity(order),
            "D26LIFT1 InSphere row-permutation parity mismatch");
        ++count;
    } while (std::next_permutation(order.begin(), order.end()));
    require(count == 120U, "did not exercise all 120 five-site permutations");
}

void verifyLiftOnlyIncircle() {
    const std::array<IndexedPoint2, 4> square{
        p2(1, 0.0, 0.0),
        p2(3, 1.0, 0.0),
        p2(2, 0.0, 1.0),
        p2(4, 1.0, 1.0)};

    const ResolvedDelaunaySign base =
        femcae::meshing::m2::resolveLiftOnlyIncircle(square);
    require(base.geometricSign == PredicateSign::Zero,
            "co-circular square is not exact InCircle zero");
    require(base.resolvedSign == PredicateSign::Negative,
            "D26LIFT1 co-circular golden sign mismatch");

    std::array<std::size_t, 4> order{0U, 1U, 2U, 3U};
    std::size_t count = 0U;
    do {
        const ResolvedDelaunaySign actual =
            femcae::meshing::m2::resolveLiftOnlyIncircle(
                permuted(square, order));
        require(actual.geometricSign == PredicateSign::Zero,
                "row permutation changed exact co-circular truth");
        require(
            toInt(actual.resolvedSign) ==
                toInt(base.resolvedSign) * permutationParity(order),
            "D26LIFT1 InCircle row-permutation parity mismatch");
        ++count;
    } while (std::next_permutation(order.begin(), order.end()));
    require(count == 24U, "did not exercise all 24 four-site permutations");
}

void verifyGhostConflictSemantic() {
    const std::array<IndexedPoint3, 3> facet{
        p3(1, 0.0, 0.0, 0.0),
        p3(3, 1.0, 0.0, 0.0),
        p3(2, 0.0, 1.0, 0.0)};
    const IndexedPoint3 insideWitness = p3(4, 0.0, 0.0, 1.0);

    require(
        femcae::meshing::predicates::orient3d(
            facet[0].point,
            facet[1].point,
            facet[2].point,
            insideWitness.point).sign == PredicateSign::Negative,
        "ghost fixture facet is not outward oriented");

    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            facet, insideWitness, p3(5, 0.2, 0.2, -1.0)) ==
            DelaunayConflict::Conflict,
        "exterior half-space did not conflict with ghost");

    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            facet, insideWitness, p3(5, 0.2, 0.2, 0.2)) ==
            DelaunayConflict::NoConflict,
        "triangulation-side point conflicted with ghost");

    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            facet, insideWitness, p3(5, 0.25, 0.25, 0.0)) ==
            DelaunayConflict::Conflict,
        "coplanar circumdisk interior did not conflict");

    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            facet, insideWitness, p3(5, 2.0, 2.0, 0.0)) ==
            DelaunayConflict::NoConflict,
        "coplanar circumdisk exterior conflicted");

    const ResolvedDelaunaySign circle =
        femcae::meshing::m2::classifyProjectedCoplanarCircumcircle(
            facet, p3(5, 1.0, 1.0, 0.0));
    require(circle.geometricSign == PredicateSign::Zero,
            "coplanar square-circle fixture is not exact zero");
    require(circle.resolvedSign == PredicateSign::Negative,
            "projected D26LIFT1 tie sign mismatch");
    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            facet, insideWitness, p3(5, 1.0, 1.0, 0.0)) ==
            DelaunayConflict::NoConflict,
        "symbolic coplanar circle tie conflict mismatch");
}

void verifyProjectionFallback() {
    const std::array<IndexedPoint3, 3> verticalFacet{
        p3(1, 0.0, 0.0, 0.0),
        p3(3, 1.0, 0.0, 0.0),
        p3(2, 0.0, 0.0, 1.0)};
    const IndexedPoint3 insideWitness = p3(4, 0.0, -1.0, 0.0);

    require(
        femcae::meshing::m2::classifyGhostCellConflict(
            verticalFacet,
            insideWitness,
            p3(5, 0.25, 0.0, 0.25)) ==
            DelaunayConflict::Conflict,
        "XY-degenerate facet did not fall back to XZ projection");
}

void verifyInvalidInputs() {
    const std::array<IndexedPoint3, 4> positive{
        p3(2, 1.0, 0.0, 0.0),
        p3(3, 0.0, 1.0, 0.0),
        p3(4, 0.0, 0.0, 1.0),
        p3(1, 0.0, 0.0, 0.0)};

    auto negative = positive;
    std::swap(negative[0], negative[1]);
    requireInvalid(
        [&] {
            (void)femcae::meshing::m2::classifyFiniteCellConflict(
                negative, p3(5, 0.25, 0.25, 0.25));
        },
        "negative stored tetra entered finite conflict semantic");

    const std::array<IndexedPoint3, 3> facet{
        p3(1, 0.0, 0.0, 0.0),
        p3(3, 1.0, 0.0, 0.0),
        p3(2, 0.0, 1.0, 0.0)};
    requireInvalid(
        [&] {
            (void)femcae::meshing::m2::classifyGhostCellConflict(
                facet,
                p3(4, 0.0, 0.0, -1.0),
                p3(5, 0.2, 0.2, -2.0));
        },
        "inward-oriented hull facet entered ghost conflict semantic");

    requireInvalid(
        [&] {
            (void)femcae::meshing::m2::classifyProjectedCoplanarCircumcircle(
                facet, p3(5, 0.2, 0.2, 1.0));
        },
        "non-coplanar query entered projected circumcircle semantic");

    const std::array<IndexedPoint3, 5> duplicateId{
        p3(1, 0.0, 0.0, 0.0),
        p3(2, 0.0, 0.0, 1.0),
        p3(3, 0.0, 1.0, 0.0),
        p3(4, 1.0, 0.0, 0.0),
        p3(4, 1.0, 1.0, 1.0)};
    requireInvalid(
        [&] {
            (void)femcae::meshing::m2::resolveLiftOnlyInsphere(duplicateId);
        },
        "duplicate PointId entered D26LIFT1");
}

} // namespace

int main() {
    try {
        verifyFiniteConflictSemantic();
        verifyLiftOnlyInsphere();
        verifyLiftOnlyIncircle();
        verifyGhostConflictSemantic();
        verifyProjectionFallback();
        verifyInvalidInputs();

        std::cout
            << "M2.1-A semantic predicates PASS"
            << " insphere_permutations=120"
            << " incircle_permutations=24"
            << " ghost=yes"
            << " projection_fallback=yes"
            << " invalid=yes\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-A semantic predicates FAIL: "
            << error.what()
            << '\n';
        return 1;
    }
}
