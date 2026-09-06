#include "QualityVector.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace femcae::meshing::m6::quality {
namespace {

bool validPointIds(
    const std::array<PointId, 4>& ids) noexcept {
    for (std::size_t i = 0; i < ids.size(); ++i) {
        if (ids[i] == InvalidPointId) {
            return false;
        }
        for (std::size_t j = i + 1U; j < ids.size(); ++j) {
            if (ids[i] == ids[j]) {
                return false;
            }
        }
    }
    return true;
}

MeanRatioOrder compareEntries(
    const QualityVectorEntry& lhs,
    const QualityVectorEntry& rhs,
    QualityVectorTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->entryComparisons;
    }

    // D26QMRF1 first. It can only return a strict order it has proven, so a
    // certified answer is the answer D26QMRB1 would have produced; anything it
    // cannot prove -- including every exact tie -- falls through to the exact
    // backend below.
    if (lhs.filterReady && rhs.filterReady) {
        MeanRatioOrder certified{MeanRatioOrder::Equal};
        if (compareMeanRatioIntervalKeys(
                lhs.filter,
                rhs.filter,
                certified) ==
            MeanRatioIntervalStatus::Ready) {
            if (telemetry != nullptr) {
                ++telemetry->intervalCertifiedComparisons;
            }
            return certified;
        }
    }

    if (telemetry != nullptr) {
        ++telemetry->exactComparisons;
    }

    const MeanRatioOrder result =
        compareExactMeanRatioKeys(
            lhs.quality,
            rhs.quality,
            exactTelemetry);

    if (telemetry != nullptr &&
        result == MeanRatioOrder::Equal) {
        ++telemetry->exactQualityTies;
    }
    return result;
}

bool representationLess(
    const QualityVectorEntry& lhs,
    const QualityVectorEntry& rhs,
    QualityVectorTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    const MeanRatioOrder qualityOrder =
        compareEntries(
            lhs,
            rhs,
            telemetry,
            exactTelemetry);

    if (qualityOrder == MeanRatioOrder::Less) {
        return true;
    }
    if (qualityOrder == MeanRatioOrder::Greater) {
        return false;
    }

    // Canonical identity yalniz representation/replay tie-break'idir.
    // D26QV1 semantic comparison exact quality tie'da bunu kullanmaz.
    return lhs.tetra < rhs.tetra;
}

} // namespace

bool isExactPositiveQualityCell(
    const IndexedTetraCoordinates& tetra) noexcept {
    if (!validPointIds(tetra.pointIds)) {
        return false;
    }

    try {
        const predicates::PredicateEvaluation orientation =
            predicates::orient3d(
                tetra.coordinates[0],
                tetra.coordinates[1],
                tetra.coordinates[2],
                tetra.coordinates[3]);
        return orientation.sign ==
               predicates::PredicateSign::Positive;
    } catch (const std::invalid_argument&) {
        return false;
    }
}

CanonicalQualityTetKey canonicalQualityTetKey(
    const IndexedTetraCoordinates& tetra) {
    if (!validPointIds(tetra.pointIds)) {
        throw std::invalid_argument(
            "D26QV1 tetra identity requires four distinct non-zero PointIds");
    }

    CanonicalQualityTetKey key = tetra.pointIds;
    std::sort(key.begin(), key.end());
    return key;
}

QualityVector buildQualityVector(
    std::span<const IndexedTetraCoordinates> tetrahedra,
    QualityVectorTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->builds;
    }

    QualityVector result;
    result.entries.reserve(tetrahedra.size());

    for (const IndexedTetraCoordinates& tetra : tetrahedra) {
        if (!isExactPositiveQualityCell(tetra)) {
            throw std::invalid_argument(
                "D26QV1 requires exact-positive finite TET4 input");
        }

        QualityVectorEntry entry{
            canonicalQualityTetKey(tetra),
            buildExactMeanRatioKey(
                tetra.coordinates,
                exactTelemetry),
            {},
            false};

        entry.filterReady =
            buildMeanRatioIntervalKey(tetra, entry.filter) ==
            MeanRatioIntervalStatus::Ready;
        if (!entry.filterReady && telemetry != nullptr) {
            ++telemetry->filterUnavailableCells;
        }

        result.entries.push_back(std::move(entry));
    }

    std::sort(
        result.entries.begin(),
        result.entries.end(),
        [telemetry, exactTelemetry](
            const QualityVectorEntry& lhs,
            const QualityVectorEntry& rhs) {
            return representationLess(
                lhs,
                rhs,
                telemetry,
                exactTelemetry);
        });

    return result;
}

QualityVectorOrder compareQualityVectors(
    const QualityVector& lhs,
    const QualityVector& rhs,
    QualityVectorTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->vectorComparisons;
    }

    const std::size_t commonSize =
        std::min(lhs.entries.size(), rhs.entries.size());

    for (std::size_t i = 0; i < commonSize; ++i) {
        const MeanRatioOrder entryOrder =
            compareEntries(
                lhs.entries[i],
                rhs.entries[i],
                telemetry,
                exactTelemetry);

        if (entryOrder == MeanRatioOrder::Less) {
            return QualityVectorOrder::Worse;
        }
        if (entryOrder == MeanRatioOrder::Greater) {
            return QualityVectorOrder::Better;
        }
    }

    if (lhs.entries.size() == rhs.entries.size()) {
        return QualityVectorOrder::Equal;
    }

    // Frozen +infinity-padding semantics:
    // common exact prefix'ten sonra daha kisa vector daha iyidir.
    if (telemetry != nullptr) {
        ++telemetry->prefixDecisions;
    }
    return lhs.entries.size() < rhs.entries.size()
        ? QualityVectorOrder::Better
        : QualityVectorOrder::Worse;
}

QualityVector mergeQualityVectors(
    const QualityVector& lhs,
    const QualityVector& rhs,
    QualityVectorTelemetry* telemetry,
    ExactMeanRatioTelemetry* exactTelemetry) {
    if (telemetry != nullptr) {
        ++telemetry->mergeCalls;
        telemetry->mergedEntries +=
            lhs.entries.size() + rhs.entries.size();
    }

    QualityVector result;
    result.entries.reserve(
        lhs.entries.size() + rhs.entries.size());

    std::size_t i = 0U;
    std::size_t j = 0U;

    while (i < lhs.entries.size() &&
           j < rhs.entries.size()) {
        if (representationLess(
                rhs.entries[j],
                lhs.entries[i],
                telemetry,
                exactTelemetry)) {
            result.entries.push_back(rhs.entries[j]);
            ++j;
        } else {
            result.entries.push_back(lhs.entries[i]);
            ++i;
        }
    }

    result.entries.insert(
        result.entries.end(),
        lhs.entries.begin() + static_cast<std::ptrdiff_t>(i),
        lhs.entries.end());
    result.entries.insert(
        result.entries.end(),
        rhs.entries.begin() + static_cast<std::ptrdiff_t>(j),
        rhs.entries.end());

    return result;
}

} // namespace femcae::meshing::m6::quality
