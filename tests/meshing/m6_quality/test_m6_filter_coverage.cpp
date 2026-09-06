// D26QMRF1 certificate coverage qualification.
//
// This began as D26INT1-E1, an experiment comparing the frozen subnormal
// downscale policy against an outward-step proposal. The policy question is
// settled -- see INTERVAL_DOWNSCALE_POLICY_EXPERIMENT.md -- so what remains is
// the property the decision rests on, kept as a regression guard.
//
// Two things are pinned:
//
//   1. Coverage. The old policy refused to scale any interval whose endpoint
//      underflowed, and an outward step widens an exactly-zero relative
//      coordinate to [-denorm_min, +denorm_min], so every cell with an
//      axis-aligned edge lost its certificate. Coverage on structured and
//      planar populations measured 0.0%. Those are precisely the cells
//      StructuredHexMesher emits, so the filter was pure overhead on this
//      repository's own output. A regression here would silently restore that,
//      with no test failing anywhere else.
//
//   2. Agreement. A certificate is only ever allowed to be the answer the exact
//      backend would have produced.
//
// Coverage thresholds are deliberately loose. They exist to catch a collapse,
// not to freeze a number.

#include "meshing/m6/quality/MeanRatioFilter.h"

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

struct Population {
    std::string name;
    double minimumCoverage{0.0};
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
    std::uniform_int_distribution<int> leg(1, 6);

    std::vector<Population> populations;

    Population randomCells{"random coordinates", 0.99, {}};
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
    // lengths vary so the population carries a spread of qualities; congruent
    // cells compare exactly equal, which correctly yields no certificate.
    Population latticeCells{"structured lattice corner", 0.99, {}};
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
    Population planarCells{"three vertices on a plane", 0.99, {}};
    for (std::size_t i = 0; i < count; ++i) {
        planarCells.cells.push_back(makeCell(
            i,
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 0.0},
            {unit(rng), unit(rng), 1.0}));
    }
    populations.push_back(std::move(planarCells));

    // One shared axis value between two vertices -- the mildest form of the
    // coordinate coincidence that used to cost the certificate.
    Population oneSharedAxis{"one shared axis value", 0.99, {}};
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
    Population gradedCells{"graded anisotropic", 0.99, {}};
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

struct Result {
    std::size_t total{0};
    std::size_t ready{0};
    std::size_t certified{0};
    std::size_t agreed{0};
};

Result measure(const Population& population) {
    Result result;
    result.total = population.cells.size();

    for (const q::IndexedTetraCoordinates& cell : population.cells) {
        q::MeanRatioIntervalKey key{};
        if (q::buildMeanRatioIntervalKey(cell, key) ==
            q::MeanRatioIntervalStatus::Ready) {
            ++result.ready;
        }
    }

    for (std::size_t i = 0; i + 1U < population.cells.size(); ++i) {
        const q::IndexedTetraCoordinates& lhs = population.cells[i];
        const q::IndexedTetraCoordinates& rhs = population.cells[i + 1U];

        q::MeanRatioFilterTelemetry telemetry{};
        const q::MeanRatioEvaluation evaluation =
            q::compareFilteredMeanRatio(lhs, rhs, &telemetry);
        if (evaluation.path !=
            q::MeanRatioEvaluationPath::IntervalCertified) {
            continue;
        }
        ++result.certified;

        const q::MeanRatioOrder exact =
            q::compareExactMeanRatio(lhs.coordinates, rhs.coordinates);
        require(
            evaluation.order == exact,
            "certified filter order disagrees with D26QMRB1 in population " +
                population.name);
        require(
            exact != q::MeanRatioOrder::Equal,
            "filter certified an exact tie, which it must never do, in "
            "population " + population.name);
        ++result.agreed;
    }

    return result;
}

} // namespace

int main() {
    try {
        require(
            iv::probeEnvironment().supported(),
            "D26QMRF1 coverage qualification requires a supported environment");

        const std::vector<Population> populations = buildPopulations(4000U);

        std::cout
            << std::left << std::setw(28) << "population"
            << std::right << std::setw(10) << "coverage"
            << std::setw(12) << "certified"
            << std::setw(9) << "agreed" << '\n';

        std::size_t totalAgreed = 0U;
        for (const Population& population : populations) {
            const Result result = measure(population);
            const double coverage =
                static_cast<double>(result.ready) /
                static_cast<double>(result.total);

            std::cout
                << std::left << std::setw(28) << population.name
                << std::right << std::setw(9)
                << std::fixed << std::setprecision(1) << (coverage * 100.0)
                << '%' << std::setw(12) << result.certified
                << std::setw(9) << result.agreed << '\n';

            require(
                coverage >= population.minimumCoverage,
                "D26QMRF1 certificate coverage collapsed for population " +
                    population.name);
            require(
                result.certified > result.total / 2U,
                "D26QMRF1 certified too few comparisons in population " +
                    population.name);
            totalAgreed += result.agreed;
        }

        std::cout
            << "\nM6 D26QMRF1 coverage PASS agreed=" << totalAgreed
            << " disagreements=0\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M6 D26QMRF1 coverage FAIL: " << error.what() << '\n';
        return 1;
    }
}
