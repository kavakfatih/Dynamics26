// D26INT1-E1 — subnormal downscale policy experiment.
//
// Question
// --------
// scalePowerOfTwo currently refuses to scale an interval whose endpoint would
// underflow or land in the subnormal range, and reports Range so the caller
// drops to the exact backend. That policy is frozen and asserted by
// test_m6_interval_backend (M6-R75..R108).
//
// The policy has a reachable cost. An outward step widens an exactly-zero
// relative coordinate to [-denorm_min, +denorm_min], and normalization then
// scales that interval down, so both endpoints underflow and the whole cell
// loses its certificate. A relative coordinate is exactly zero whenever the
// anchor vertex shares an axis value with another vertex of the cell -- an
// axis-aligned edge, or a face lying on a coordinate plane.
//
// This experiment measures, per cell population:
//   - how often the current policy yields a certificate,
//   - how often a proposed outward-step policy would,
//   - whether the proposed policy ever certifies an order that disagrees with
//     the exact backend D26QMRB1.
//
// It changes no production code. The proposed policy lives in `proposed` below
// as an experiment-local mirror of the frozen production trees, so a policy
// decision can be taken on measured coverage and measured agreement rather than
// on argument.
//
// Coverage is reported, never asserted: the experiment fails only on a
// soundness violation, because only soundness is a property the policy is
// required to have.

#include "meshing/m6/quality/IntervalArithmetic.h"
#include "meshing/m6/quality/MeanRatioFilter.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iomanip>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace q = femcae::meshing::m6::quality;
namespace iv = femcae::meshing::m6::quality::interval;

using femcae::geometry::Vec3;
using femcae::meshing::PointId;

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

// ---------------------------------------------------------------------------
// Proposed policy: an experiment-local mirror of the frozen D26QMRF1 pipeline
// in which only scalePowerOfTwo differs. Every other tree is copied shape for
// shape from MeanRatioFilter.cpp, so a coverage difference can only come from
// the downscale policy under test.
// ---------------------------------------------------------------------------
namespace proposed {

using Interval = iv::Interval;
using IntervalResult = iv::IntervalResult;
using IntervalVec3 = std::array<Interval, 3>;
using RelativeTetra = std::array<IntervalVec3, 4>;

// The proposal. scalbn is exact while the result stays normal, so a normal
// result needs no step. When the result is subnormal, scalbn is correctly
// rounded and one outward step covers the rounding. When the result underflows
// to zero from a non-zero input, the true magnitude is below denorm_min, so one
// outward step still encloses it. Widening is always sound for an enclosure;
// the current policy discards the enclosure instead of widening it.
IntervalResult scalePowerOfTwoOutward(
    Interval value,
    int exponent) noexcept {
    if (!std::isfinite(value.lo) ||
        !std::isfinite(value.hi) ||
        value.lo > value.hi) {
        return {{}, iv::IntervalFailure::InvalidInput};
    }

    const double lo = std::scalbn(value.lo, exponent);
    const double hi = std::scalbn(value.hi, exponent);
    if (!std::isfinite(lo) || !std::isfinite(hi)) {
        return {{}, iv::IntervalFailure::Range};
    }

    const auto exact = [](double before, double after) noexcept {
        if (before == 0.0) {
            return true;
        }
        return after != 0.0 &&
               std::fpclassify(after) != FP_SUBNORMAL;
    };

    const double widenedLo =
        exact(value.lo, lo) ? lo : iv::nextDown(lo);
    const double widenedHi =
        exact(value.hi, hi) ? hi : iv::nextUp(hi);

    if (!std::isfinite(widenedLo) || !std::isfinite(widenedHi)) {
        return {{}, iv::IntervalFailure::Range};
    }
    return {{widenedLo, widenedHi}, iv::IntervalFailure::None};
}

std::array<std::size_t, 4> canonicalPositiveOrder(
    const std::array<PointId, 4>& ids) noexcept {
    std::array<std::size_t, 4> order{0U, 1U, 2U, 3U};
    std::sort(
        order.begin(),
        order.end(),
        [&ids](std::size_t lhs, std::size_t rhs) {
            return ids[lhs] < ids[rhs];
        });

    std::size_t inversions = 0U;
    for (std::size_t i = 0; i < order.size(); ++i) {
        for (std::size_t j = i + 1U; j < order.size(); ++j) {
            if (order[i] > order[j]) {
                ++inversions;
            }
        }
    }
    if ((inversions & 1U) != 0U) {
        std::swap(order[2], order[3]);
    }
    return order;
}

IntervalResult subChecked(
    const IntervalResult& lhs,
    const IntervalResult& rhs) noexcept {
    if (!lhs.ok()) {
        return lhs;
    }
    if (!rhs.ok()) {
        return rhs;
    }
    return iv::subtract(lhs.value, rhs.value);
}

IntervalResult addChecked(
    const IntervalResult& lhs,
    const IntervalResult& rhs) noexcept {
    if (!lhs.ok()) {
        return lhs;
    }
    if (!rhs.ok()) {
        return rhs;
    }
    return iv::add(lhs.value, rhs.value);
}

IntervalResult determinant3(
    const std::array<IntervalVec3, 3>& relative) noexcept {
    const IntervalResult ei = iv::multiply(relative[1][1], relative[2][2]);
    const IntervalResult fh = iv::multiply(relative[1][2], relative[2][1]);
    const IntervalResult m0 = subChecked(ei, fh);

    const IntervalResult di = iv::multiply(relative[1][0], relative[2][2]);
    const IntervalResult fg = iv::multiply(relative[1][2], relative[2][0]);
    const IntervalResult m1 = subChecked(di, fg);

    const IntervalResult dh = iv::multiply(relative[1][0], relative[2][1]);
    const IntervalResult eg = iv::multiply(relative[1][1], relative[2][0]);
    const IntervalResult m2 = subChecked(dh, eg);

    const IntervalResult t0 =
        m0.ok() ? iv::multiply(relative[0][0], m0.value) : m0;
    const IntervalResult t1 =
        m1.ok() ? iv::multiply(relative[0][1], m1.value) : m1;
    const IntervalResult t2 =
        m2.ok() ? iv::multiply(relative[0][2], m2.value) : m2;

    return addChecked(subChecked(t0, t1), t2);
}

IntervalResult squaredDistance(
    const IntervalVec3& lhs,
    const IntervalVec3& rhs) noexcept {
    std::array<IntervalResult, 3> squares{};
    for (std::size_t axis = 0; axis < 3U; ++axis) {
        const IntervalResult difference =
            iv::subtract(lhs[axis], rhs[axis]);
        if (!difference.ok()) {
            return difference;
        }
        squares[axis] = iv::square(difference.value);
        if (!squares[axis].ok()) {
            return squares[axis];
        }
    }

    const IntervalResult xy =
        iv::add(squares[0].value, squares[1].value);
    if (!xy.ok()) {
        return xy;
    }
    return iv::add(xy.value, squares[2].value);
}

IntervalResult balancedEdgeSum(const RelativeTetra& points) noexcept {
    std::array<IntervalResult, 6> edges{};
    std::size_t edge = 0U;
    for (std::size_t i = 0; i < 4U; ++i) {
        for (std::size_t j = i + 1U; j < 4U; ++j) {
            edges[edge] = squaredDistance(points[i], points[j]);
            if (!edges[edge].ok()) {
                return edges[edge];
            }
            ++edge;
        }
    }

    const IntervalResult s01 = iv::add(edges[0].value, edges[1].value);
    const IntervalResult s23 = iv::add(edges[2].value, edges[3].value);
    const IntervalResult s45 = iv::add(edges[4].value, edges[5].value);
    if (!s01.ok()) {
        return s01;
    }
    if (!s23.ok()) {
        return s23;
    }
    if (!s45.ok()) {
        return s45;
    }

    const IntervalResult s0123 = iv::add(s01.value, s23.value);
    if (!s0123.ok()) {
        return s0123;
    }
    return iv::add(s0123.value, s45.value);
}

bool buildKey(
    const q::IndexedTetraCoordinates& tetra,
    q::MeanRatioIntervalKey& output) noexcept {
    if (!iv::probeEnvironment().supported()) {
        return false;
    }
    for (const Vec3& point : tetra.coordinates) {
        if (!std::isfinite(point.x) ||
            !std::isfinite(point.y) ||
            !std::isfinite(point.z)) {
            return false;
        }
    }

    const std::array<std::size_t, 4> order =
        canonicalPositiveOrder(tetra.pointIds);
    const Vec3& anchor = tetra.coordinates[order[0]];

    const Interval zero{0.0, 0.0};
    RelativeTetra normalized{};
    normalized[0] = {zero, zero, zero};

    double maximumMagnitude = 0.0;
    std::array<IntervalVec3, 3> relative{};

    for (std::size_t row = 0; row < 3U; ++row) {
        const Vec3& point = tetra.coordinates[order[row + 1U]];
        const double p[3] = {point.x, point.y, point.z};
        const double a[3] = {anchor.x, anchor.y, anchor.z};

        for (std::size_t axis = 0; axis < 3U; ++axis) {
            const IntervalResult difference =
                iv::subtract({p[axis], p[axis]}, {a[axis], a[axis]});
            if (!difference.ok()) {
                return false;
            }
            relative[row][axis] = difference.value;
            maximumMagnitude = std::max(
                maximumMagnitude,
                std::max(
                    std::abs(difference.value.lo),
                    std::abs(difference.value.hi)));
        }
    }

    int exponent = 0;
    if (!iv::normalizationExponent(maximumMagnitude, exponent)) {
        return false;
    }

    for (std::size_t row = 0; row < 3U; ++row) {
        for (std::size_t axis = 0; axis < 3U; ++axis) {
            const IntervalResult scaled =
                scalePowerOfTwoOutward(relative[row][axis], exponent);
            if (!scaled.ok()) {
                return false;
            }
            normalized[row + 1U][axis] = scaled.value;
        }
    }

    const std::array<IntervalVec3, 3> rows{
        normalized[1], normalized[2], normalized[3]};

    const IntervalResult determinant = determinant3(rows);
    if (!determinant.ok()) {
        return false;
    }
    const IntervalResult edgeSum = balancedEdgeSum(normalized);
    if (!edgeSum.ok()) {
        return false;
    }
    if (!(edgeSum.value.lo > 0.0)) {
        return false;
    }

    output = {determinant.value, edgeSum.value, exponent};
    return true;
}

} // namespace proposed

// ---------------------------------------------------------------------------
// Cell populations
// ---------------------------------------------------------------------------

struct Population {
    std::string name;
    std::vector<q::IndexedTetraCoordinates> cells;
};

q::IndexedTetraCoordinates makeCell(
    std::size_t index,
    const Vec3& a,
    const Vec3& b,
    const Vec3& c,
    const Vec3& d) {
    const PointId base = static_cast<PointId>(4U * index + 1U);
    q::IndexedTetraCoordinates cell;
    cell.pointIds = {base, base + 1U, base + 2U, base + 3U};
    cell.coordinates = {a, b, c, d};
    return cell;
}

std::vector<Population> buildPopulations(std::size_t count) {
    std::mt19937_64 rng(20260906U);
    std::uniform_real_distribution<double> unit(-1.0, 1.0);
    std::uniform_int_distribution<int> lattice(0, 16);

    std::vector<Population> populations;

    Population randomCells{"random coordinates", {}};
    for (std::size_t i = 0; i < count; ++i) {
        randomCells.cells.push_back(makeCell(
            i,
            {unit(rng), unit(rng), unit(rng)},
            {unit(rng), unit(rng), unit(rng)},
            {unit(rng), unit(rng), unit(rng)},
            {unit(rng), unit(rng), unit(rng)}));
    }
    populations.push_back(std::move(randomCells));

    // What StructuredHexMesher and any box-compatible CAD body produce. Leg
    // lengths vary per cell so the population carries a spread of qualities
    // rather than one congruent shape; congruent cells compare exactly equal,
    // which correctly yields no certificate and would leave the agreement check
    // with nothing to measure.
    std::uniform_int_distribution<int> leg(1, 6);
    Population latticeCells{"structured lattice corner", {}};
    for (std::size_t i = 0; i < count; ++i) {
        const double x = lattice(rng);
        const double y = lattice(rng);
        const double z = lattice(rng);
        latticeCells.cells.push_back(makeCell(
            i,
            {x, y, z},
            {x + leg(rng), y, z},
            {x, y + leg(rng), z},
            {x, y, z + leg(rng)}));
    }
    populations.push_back(std::move(latticeCells));

    // A boundary cell with three vertices on a planar CAD face.
    Population planarCells{"three vertices on a plane", {}};
    for (std::size_t i = 0; i < count; ++i) {
        planarCells.cells.push_back(makeCell(
            i,
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 1.0}));
    }
    populations.push_back(std::move(planarCells));

    // One shared axis value only, the mildest form of the same situation.
    Population oneSharedAxis{"one shared axis value", {}};
    for (std::size_t i = 0; i < count; ++i) {
        const double shared = unit(rng);
        oneSharedAxis.cells.push_back(makeCell(
            i,
            {shared, unit(rng), unit(rng)},
            {shared, unit(rng), unit(rng)},
            {unit(rng), unit(rng), unit(rng)},
            {unit(rng), unit(rng), unit(rng)}));
    }
    populations.push_back(std::move(oneSharedAxis));

    // Anisotropic but general: no exact coordinate coincidences.
    Population gradedCells{"graded anisotropic", {}};
    for (std::size_t i = 0; i < count; ++i) {
        gradedCells.cells.push_back(makeCell(
            i,
            {unit(rng), unit(rng) * 1e-3, unit(rng) * 1e-6},
            {unit(rng), unit(rng) * 1e-3, unit(rng) * 1e-6},
            {unit(rng), unit(rng) * 1e-3, unit(rng) * 1e-6},
            {unit(rng), unit(rng) * 1e-3, unit(rng) * 1e-6}));
    }
    populations.push_back(std::move(gradedCells));

    return populations;
}

struct Coverage {
    std::size_t total{0};
    std::size_t current{0};
    std::size_t proposedReady{0};
};

Coverage measureCoverage(const Population& population) {
    Coverage coverage;
    coverage.total = population.cells.size();

    for (const q::IndexedTetraCoordinates& cell : population.cells) {
        q::MeanRatioIntervalKey currentKey{};
        if (q::buildMeanRatioIntervalKey(cell, currentKey) ==
            q::MeanRatioIntervalStatus::Ready) {
            ++coverage.current;
        }

        q::MeanRatioIntervalKey proposedKey{};
        if (proposed::buildKey(cell, proposedKey)) {
            ++coverage.proposedReady;
        }
    }
    return coverage;
}

struct Agreement {
    std::size_t certified{0};
    std::size_t checked{0};
};

// Soundness evidence: every order the proposed policy certifies must be the
// order the exact backend reaches. This is the property the qualification gate
// would have to re-establish, so it is the one thing this experiment asserts.
Agreement verifyProposedAgreement(const Population& population) {
    Agreement agreement;

    for (std::size_t i = 0; i + 1U < population.cells.size(); ++i) {
        const q::IndexedTetraCoordinates& lhs = population.cells[i];
        const q::IndexedTetraCoordinates& rhs = population.cells[i + 1U];

        q::MeanRatioIntervalKey lhsKey{};
        q::MeanRatioIntervalKey rhsKey{};
        if (!proposed::buildKey(lhs, lhsKey) ||
            !proposed::buildKey(rhs, rhsKey)) {
            continue;
        }

        q::MeanRatioOrder certified{q::MeanRatioOrder::Equal};
        if (q::compareMeanRatioIntervalKeys(lhsKey, rhsKey, certified) !=
            q::MeanRatioIntervalStatus::Ready) {
            continue;
        }
        ++agreement.certified;

        q::MeanRatioOrder exact{q::MeanRatioOrder::Equal};
        try {
            exact = q::compareExactMeanRatio(
                lhs.coordinates,
                rhs.coordinates);
        } catch (const std::invalid_argument&) {
            // A degenerate cell has no exact quality key, so there is no order
            // to agree with. The proposed policy must not have certified one.
            fail(
                "proposed policy certified an order for a cell the exact "
                "backend rejects, population " + population.name);
        }

        ++agreement.checked;
        require(
            certified == exact,
            "proposed downscale policy certified an order that disagrees with "
            "D26QMRB1, population " + population.name);
    }
    return agreement;
}

void reportPercent(std::size_t part, std::size_t whole) {
    if (whole == 0U) {
        std::cout << std::setw(7) << "n/a";
        return;
    }
    const double percent =
        100.0 * static_cast<double>(part) / static_cast<double>(whole);
    std::cout
        << std::setw(6) << std::fixed << std::setprecision(1) << percent
        << '%';
}

} // namespace

int main() {
    try {
        require(
            iv::probeEnvironment().supported(),
            "D26INT1-E1 requires a supported binary64 environment");

        const std::vector<Population> populations =
            buildPopulations(4000U);

        std::cout
            << "D26INT1-E1 subnormal downscale policy experiment\n"
            << "cells per population: 4000\n\n"
            << std::left << std::setw(28) << "population"
            << std::right << std::setw(9) << "current"
            << std::setw(10) << "proposed"
            << std::setw(12) << "certified"
            << std::setw(10) << "checked" << '\n';

        std::size_t totalCertified = 0U;
        std::size_t totalChecked = 0U;

        for (const Population& population : populations) {
            const Coverage coverage = measureCoverage(population);
            const Agreement agreement =
                verifyProposedAgreement(population);

            totalCertified += agreement.certified;
            totalChecked += agreement.checked;

            std::cout << std::left << std::setw(28) << population.name
                      << std::right;
            reportPercent(coverage.current, coverage.total);
            std::cout << "   ";
            reportPercent(coverage.proposedReady, coverage.total);
            std::cout << std::setw(12) << agreement.certified
                      << std::setw(10) << agreement.checked << '\n';
        }

        std::cout
            << "\nD26INT1-E1 PASS"
            << " certified=" << totalCertified
            << " agreed=" << totalChecked
            << " disagreements=0\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "D26INT1-E1 FAIL: " << error.what() << '\n';
        return 1;
    }
}
