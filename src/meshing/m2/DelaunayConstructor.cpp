#include "DelaunayConstructor.h"

#include "../internal/PredicateCallAudit.h"

#include <algorithm>
#include <array>
#include <bit>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <utility>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

bool finiteCell(const DelaunayCellRecord& cell) noexcept {
    return std::all_of(
        cell.vertices.begin(), cell.vertices.end(),
        [](const DelaunayVertexRef& vertex) {
            return vertex.isFinite();
        });
}

bool ghostCell(const DelaunayCellRecord& cell) noexcept {
    return cell.vertices[0].isInfinite() &&
           cell.vertices[1].isFinite() &&
           cell.vertices[2].isFinite() &&
           cell.vertices[3].isFinite();
}

bool liveHandle(
    std::span<const DelaunayCellSlot> slots,
    DelaunayCellHandle handle) noexcept {
    return handle.isValid() &&
           handle.slot < slots.size() &&
           slots[handle.slot].live &&
           slots[handle.slot].generation == handle.generation;
}

std::uint64_t canonicalCoordinateBits(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument(
            "D26DT1 requires finite canonical coordinates");
    }
    std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0ULL) {
        bits = 0ULL;
    }
    return bits;
}

std::size_t checkedSizeAdd(
    std::size_t lhs,
    std::size_t rhs,
    const char* context) {
    if (rhs > std::numeric_limits<std::size_t>::max() - lhs) {
        throw std::length_error(context);
    }
    return lhs + rhs;
}

std::size_t checkedSizeMultiply(
    std::size_t lhs,
    std::size_t rhs,
    const char* context) {
    if (lhs != 0U &&
        rhs > std::numeric_limits<std::size_t>::max() / lhs) {
        throw std::length_error(context);
    }
    return lhs * rhs;
}

void checkedCounterAdd(
    std::uint64_t& value,
    std::uint64_t add,
    const char* context) {
    if (add > std::numeric_limits<std::uint64_t>::max() - value) {
        throw std::overflow_error(context);
    }
    value += add;
}

std::array<DelaunayVertexRef, 4> canonicalCellIdentity(
    const DelaunayCellRecord& cell) {
    auto identity = cell.vertices;
    std::sort(identity.begin(), identity.end());
    return identity;
}

std::vector<PointId> finiteIds(
    const std::array<DelaunayVertexRef, 4>& identity) {
    std::vector<PointId> ids;
    ids.reserve(4U);
    for (const auto& vertex : identity) {
        if (const auto id = vertex.finitePointId()) {
            ids.push_back(*id);
        }
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::array<PointId, 4> finiteTetIds(
    const DelaunayCellRecord& cell) {
    if (!finiteCell(cell)) {
        throw std::invalid_argument(
            "D26DT1 finite tetra record received non-finite cell");
    }
    std::array<PointId, 4> ids{};
    for (std::size_t i = 0U; i < 4U; ++i) {
        ids[i] = *cell.vertices[i].finitePointId();
    }
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::array<PointId, 3> ghostHullIds(
    const DelaunayCellRecord& cell) {
    if (!ghostCell(cell)) {
        throw std::invalid_argument(
            "D26DT1 hull record received invalid ghost cell");
    }
    std::array<PointId, 3> ids{
        *cell.vertices[1].finitePointId(),
        *cell.vertices[2].finitePointId(),
        *cell.vertices[3].finitePointId()};
    std::sort(ids.begin(), ids.end());
    return ids;
}

std::optional<std::size_t> matchingFace(
    const DelaunayCellRecord& cell,
    const DelaunayFaceKey& key) {
    std::optional<std::size_t> found;
    for (std::size_t face = 0U; face < 4U; ++face) {
        if (canonicalDelaunayFaceKey(cell, face) != key) {
            continue;
        }
        if (found.has_value()) {
            return std::nullopt;
        }
        found = face;
    }
    return found;
}

std::map<PointId, geometry::Vec3> pointMap(
    std::span<const CanonicalSite> sites) {
    std::map<PointId, geometry::Vec3> result;
    for (const auto& site : sites) {
        if (site.id == InvalidPointId ||
            !result.emplace(site.id, site.point).second) {
            throw std::invalid_argument(
                "P1F requires unique non-zero canonical PointIds");
        }
    }
    return result;
}

std::vector<std::array<DelaunayVertexRef, 4>>
incidentIdentities(const DelaunayLocationResult& location) {
    std::vector<std::array<DelaunayVertexRef, 4>> result;
    result.reserve(location.incidentCells.size());
    for (const auto& item : location.incidentCells) {
        result.push_back(item.canonicalVertices);
    }
    std::sort(result.begin(), result.end());
    return result;
}

bool locationSemanticAgreement(
    const DelaunayLocationResult& walk,
    const DelaunayLocationResult& oracle) {
    if (walk.kind != oracle.kind ||
        walk.entityVertices != oracle.entityVertices) {
        return false;
    }
    if (walk.kind == DelaunayLocationKind::OutsideConvexHull) {
        return walk.outsideWitness.has_value() &&
               oracle.outsideWitness.has_value();
    }
    return incidentIdentities(walk) ==
           incidentIdentities(oracle);
}

std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0')
        << std::setw(16) << value;
    return out.str();
}

std::uint64_t fnv1a64(std::string_view text) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (unsigned char byte : text) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string digestHex(std::string_view record) {
    return hex64(fnv1a64(record));
}

DelaunayPredicatePathTelemetry copyPredicateTelemetry(
    const predicates::PredicateTelemetry& telemetry) {
    return {
        telemetry.calls,
        telemetry.fastCertified,
        telemetry.exactFallback,
        telemetry.exactZero,
        telemetry.invalidInput};
}

class PredicateAuditScope {
public:
    PredicateAuditScope(
        bool enabled,
        DelaunayConstructorTelemetry& destination)
        : enabled_(enabled), destination_(destination) {
        if (!enabled_) {
            return;
        }
        predicates::internal::setPredicateDetailedAuditSink(
            &predicate_);
        detail::setDelaunaySemanticTelemetrySink(
            &semantic_);
    }

    ~PredicateAuditScope() {
        flush();
    }

    void flush() noexcept {
        if (!enabled_) {
            return;
        }
        detail::setDelaunaySemanticTelemetrySink(nullptr);
        predicates::internal::setPredicateDetailedAuditSink(nullptr);
        destination_.orient2d =
            copyPredicateTelemetry(predicate_.orient2d);
        destination_.orient3d =
            copyPredicateTelemetry(predicate_.orient3d);
        destination_.incircle =
            copyPredicateTelemetry(predicate_.incircle);
        destination_.insphere =
            copyPredicateTelemetry(predicate_.insphere);
        destination_.symbolicInsphereTies =
            semantic_.symbolicInsphereTies;
        destination_.symbolicIncircleTies =
            semantic_.symbolicIncircleTies;
        enabled_ = false;
    }

private:
    bool enabled_{false};
    DelaunayConstructorTelemetry& destination_;
    predicates::internal::PredicateDetailedTelemetry predicate_;
    DelaunaySemanticTelemetry semantic_;
};

std::vector<PointId> defaultFullOrder(
    std::span<const CanonicalSite> sites) {
    std::vector<PointId> order;
    order.reserve(sites.size());
    for (const auto& site : sites) {
        order.push_back(site.id);
    }
    std::sort(order.begin(), order.end());
    return order;
}

bool validateFullOrder(
    std::span<const CanonicalSite> sites,
    const std::vector<PointId>& order) {
    if (order.size() != sites.size()) {
        return false;
    }
    std::set<PointId> expected;
    for (const auto& site : sites) {
        expected.insert(site.id);
    }
    std::set<PointId> actual;
    for (PointId id : order) {
        if (id == InvalidPointId ||
            !actual.insert(id).second) {
            return false;
        }
    }
    return actual == expected;
}

std::vector<PointId> effectiveOrder(
    const std::vector<PointId>& full,
    const std::array<PointId, 4>& basis) {
    const std::set<PointId> basisSet(
        basis.begin(), basis.end());
    std::vector<PointId> result;
    result.reserve(full.size());
    for (PointId id : full) {
        if (!basisSet.contains(id)) {
            result.push_back(id);
        }
    }
    return result;
}

const CanonicalSite& siteById(
    std::span<const CanonicalSite> sites,
    PointId id) {
    const auto iterator = std::find_if(
        sites.begin(), sites.end(),
        [id](const CanonicalSite& site) {
            return site.id == id;
        });
    if (iterator == sites.end()) {
        throw std::out_of_range(
            "P1F insertion order references missing canonical PointId");
    }
    return *iterator;
}

DelaunayValidatedStateEvidence validatedState(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    std::size_t completedInsertions) {
    const auto validation =
        validateDelaunayTopology(arena.slots(), sites);
    if (!validation.ok()) {
        throw std::logic_error(
            "P1F serial state failed typed finite/ghost topology validation");
    }
    const auto stats =
        computeDelaunayComplexStats(arena.slots());
    return {
        completedInsertions,
        stats.finiteCells,
        stats.ghostCells,
        arena.liveCount(),
        arena.slots().size(),
        stats.ghostCells,
        arena.topologyVersion()};
}

void updateFinalStorageTelemetry(
    DelaunayConstructorResult& result,
    bool telemetryEnabled) {
    if (!telemetryEnabled) {
        return;
    }
    const auto stats =
        computeDelaunayComplexStats(result.arena.slots());
    result.telemetry.finiteLive = stats.finiteCells;
    result.telemetry.ghostLive = stats.ghostCells;
    result.telemetry.slotsTotal = result.arena.slots().size();
    result.telemetry.slotsDead =
        result.arena.slots().size() - result.arena.liveCount();
    result.telemetry.hullFacets = stats.ghostCells;
    if (!result.canonicalSites.empty()) {
        result.telemetry.finiteTetsPerSite =
            static_cast<double>(stats.finiteCells) /
            static_cast<double>(result.canonicalSites.size());
        result.telemetry.unifiedCellsPerSite =
            static_cast<double>(result.arena.liveCount()) /
            static_cast<double>(result.canonicalSites.size());
    }
}

void updatePeakLive(
    DelaunayConstructorResult& result,
    bool telemetryEnabled) {
    if (!telemetryEnabled) {
        return;
    }
    result.telemetry.peakLive = std::max<std::uint64_t>(
        result.telemetry.peakLive,
        static_cast<std::uint64_t>(result.arena.liveCount()));
}

void recordLocationDecisions(
    DelaunayConstructorResult& result,
    const DelaunayWalkResult& walk) {
    DelaunayReplayDecision location;
    location.kind = DelaunayReplayDecisionKind::Location;
    location.value =
        static_cast<std::uint64_t>(walk.location->kind);
    location.ids = walk.location->entityVertices;
    result.replayDecisions.push_back(std::move(location));

    for (const auto& face : walk.trace.crossedFacets) {
        DelaunayReplayDecision crossing;
        crossing.kind =
            DelaunayReplayDecisionKind::WalkCrossing;
        crossing.geometricSign = PredicateSign::Positive;
        crossing.resolvedSign = PredicateSign::Positive;
        for (const auto& vertex : face.vertices) {
            const auto id = vertex.finitePointId();
            if (!id.has_value()) {
                throw std::logic_error(
                    "P1F walk crossing unexpectedly contains Infinite");
            }
            crossing.ids.push_back(*id);
        }
        std::sort(crossing.ids.begin(), crossing.ids.end());
        result.replayDecisions.push_back(std::move(crossing));
    }

    if (walk.conflictSeed.has_value()) {
        DelaunayReplayDecision seed;
        seed.kind =
            DelaunayReplayDecisionKind::ConflictSeed;
        seed.value = static_cast<std::uint64_t>(
            walk.conflictSeed->semantic);
        seed.ids = finiteIds(
            walk.conflictSeed->canonicalCell);
        result.replayDecisions.push_back(std::move(seed));
    }
}

void recordConflictDecisions(
    DelaunayConstructorResult& result,
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> sites,
    PointId queryId) {
    const auto points = pointMap(sites);
    const geometry::Vec3 queryPoint =
        points.at(queryId);
    const IndexedPoint3 query{queryId, queryPoint};

    for (std::size_t slotIndex = 0U;
         slotIndex < arena.slots().size(); ++slotIndex) {
        const auto& slot = arena.slots()[slotIndex];
        if (!slot.live) {
            continue;
        }
        const auto& cell = slot.record;
        if (finiteCell(cell)) {
            std::array<IndexedPoint3, 5> data{};
            for (std::size_t i = 0U; i < 4U; ++i) {
                const PointId id =
                    *cell.vertices[i].finitePointId();
                data[i] = {id, points.at(id)};
            }
            data[4] = query;
            const PredicateSign raw =
                predicates::insphere(
                    data[0].point,
                    data[1].point,
                    data[2].point,
                    data[3].point,
                    data[4].point).sign;
            PredicateSign resolved = raw;
            if (raw == PredicateSign::Zero) {
                resolved =
                    resolveLiftOnlyInsphere(data).resolvedSign;
            }

            DelaunayReplayDecision decision;
            decision.kind =
                DelaunayReplayDecisionKind::FiniteConflict;
            decision.value =
                resolved == PredicateSign::Positive ? 1U : 0U;
            decision.geometricSign = raw;
            decision.resolvedSign = resolved;
            decision.ids = finiteIds(
                canonicalCellIdentity(cell));
            decision.ids.push_back(queryId);
            std::sort(
                decision.ids.begin(),
                decision.ids.end());
            result.replayDecisions.push_back(
                std::move(decision));
            continue;
        }

        if (!ghostCell(cell)) {
            throw std::logic_error(
                "P1F replay observer found invalid live cell");
        }

        const DelaunayCellHandle finiteNeighbor =
            cell.neighbors[0];
        if (!liveHandle(
                arena.slots(), finiteNeighbor) ||
            !finiteCell(
                arena.slots()[finiteNeighbor.slot].record)) {
            throw std::logic_error(
                "P1F replay observer found invalid ghost finite neighbor");
        }

        const DelaunayFaceKey hullKey =
            canonicalDelaunayFaceKey(cell, 0U);
        const auto finiteFace = matchingFace(
            arena.slots()[finiteNeighbor.slot].record,
            hullKey);
        if (!finiteFace.has_value()) {
            throw std::logic_error(
                "P1F replay observer found ghost/finite face mismatch");
        }
        const PointId witnessId =
            *arena.slots()[finiteNeighbor.slot]
                 .record.vertices[*finiteFace]
                 .finitePointId();

        const std::array<IndexedPoint3, 3> outward{{
            {*cell.vertices[1].finitePointId(),
             points.at(*cell.vertices[1].finitePointId())},
            {*cell.vertices[2].finitePointId(),
             points.at(*cell.vertices[2].finitePointId())},
            {*cell.vertices[3].finitePointId(),
             points.at(*cell.vertices[3].finitePointId())}}};

        const PredicateSign halfspace =
            predicates::orient3d(
                outward[0].point,
                outward[1].point,
                outward[2].point,
                queryPoint).sign;

        DelaunayReplayDecision halfspaceDecision;
        halfspaceDecision.kind =
            DelaunayReplayDecisionKind::GhostHalfspace;
        halfspaceDecision.value =
            halfspace == PredicateSign::Positive ? 1U : 0U;
        halfspaceDecision.geometricSign = halfspace;
        halfspaceDecision.resolvedSign = halfspace;
        for (const auto& site : outward) {
            halfspaceDecision.ids.push_back(site.id);
        }
        halfspaceDecision.ids.push_back(witnessId);
        halfspaceDecision.ids.push_back(queryId);
        std::sort(
            halfspaceDecision.ids.begin(),
            halfspaceDecision.ids.end());
        result.replayDecisions.push_back(
            std::move(halfspaceDecision));

        if (halfspace == PredicateSign::Zero) {
            const auto circle =
                classifyProjectedCoplanarCircumcircle(
                    outward, query);
            DelaunayReplayDecision circleDecision;
            circleDecision.kind =
                DelaunayReplayDecisionKind::GhostCoplanarCircle;
            circleDecision.value =
                circle.resolvedSign ==
                        PredicateSign::Positive
                    ? 1U
                    : 0U;
            circleDecision.geometricSign =
                circle.geometricSign;
            circleDecision.resolvedSign =
                circle.resolvedSign;
            for (const auto& site : outward) {
                circleDecision.ids.push_back(site.id);
            }
            circleDecision.ids.push_back(queryId);
            std::sort(
                circleDecision.ids.begin(),
                circleDecision.ids.end());
            result.replayDecisions.push_back(
                std::move(circleDecision));
        }
    }
}

std::string bytesToHex(std::string_view bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    std::string result;
    if (bytes.size() >
        std::numeric_limits<std::size_t>::max() / 2U) {
        throw std::length_error(
            "P1F replay hex serialization length overflow");
    }
    result.reserve(bytes.size() * 2U);
    for (unsigned char byte : bytes) {
        result.push_back(digits[(byte >> 4U) & 0xFU]);
        result.push_back(digits[byte & 0xFU]);
    }
    return result;
}

int hexDigit(char value) noexcept {
    if (value >= '0' && value <= '9') {
        return value - '0';
    }
    if (value >= 'a' && value <= 'f') {
        return 10 + value - 'a';
    }
    if (value >= 'A' && value <= 'F') {
        return 10 + value - 'A';
    }
    return -1;
}

std::string hexToBytes(std::string_view text) {
    if ((text.size() & 1U) != 0U) {
        throw std::invalid_argument(
            "P1F replay hex field has odd length");
    }
    std::string result;
    result.reserve(text.size() / 2U);
    for (std::size_t i = 0U; i < text.size(); i += 2U) {
        const int hi = hexDigit(text[i]);
        const int lo = hexDigit(text[i + 1U]);
        if (hi < 0 || lo < 0) {
            throw std::invalid_argument(
                "P1F replay hex field contains invalid digit");
        }
        result.push_back(
            static_cast<char>((hi << 4U) | lo));
    }
    return result;
}

std::string joinIds(const std::vector<PointId>& ids) {
    std::ostringstream out;
    for (std::size_t i = 0U; i < ids.size(); ++i) {
        if (i != 0U) {
            out << ';';
        }
        out << ids[i];
    }
    return out.str();
}

std::vector<PointId> parseIds(std::string_view text) {
    std::vector<PointId> ids;
    if (text.empty()) {
        return ids;
    }
    std::size_t begin = 0U;
    while (begin <= text.size()) {
        const std::size_t end = text.find(';', begin);
        const std::string_view token =
            end == std::string_view::npos
                ? text.substr(begin)
                : text.substr(begin, end - begin);
        PointId value = 0;
        const auto [ptr, error] =
            std::from_chars(
                token.data(),
                token.data() + token.size(),
                value);
        if (error != std::errc{} ||
            ptr != token.data() + token.size()) {
            throw std::invalid_argument(
                "P1F replay PointId list is invalid");
        }
        ids.push_back(value);
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return ids;
}

std::uint64_t parseU64(
    std::string_view text,
    int base = 10) {
    std::uint64_t value = 0U;
    const auto [ptr, error] =
        std::from_chars(
            text.data(),
            text.data() + text.size(),
            value,
            base);
    if (error != std::errc{} ||
        ptr != text.data() + text.size()) {
        throw std::invalid_argument(
            "P1F replay integer field is invalid");
    }
    return value;
}

DelaunayReplayRecord makeReplayRecord(
    const DelaunayConstructorResult& result,
    const DelaunayConstructorOptions& options) {
    DelaunayReplayRecord record;
    record.fullInsertionOrder =
        result.requestedInsertionOrder;
    record.maxTotalSlots =
        options.resourceLimits.maxTotalSlots;
    record.expectedStatus = result.status;
    record.failurePointId = result.failurePointId;
    record.failureInsertionIndex =
        result.failureInsertionIndex;
    record.decisions = result.replayDecisions;

    record.canonicalSites.reserve(
        result.canonicalSites.size());
    for (const auto& site : result.canonicalSites) {
        record.canonicalSites.push_back({
            site.id,
            canonicalCoordinateBits(site.point.x),
            canonicalCoordinateBits(site.point.y),
            canonicalCoordinateBits(site.point.z)});
    }

    if (result.finalFingerprint.has_value()) {
        record.expectedPartialRecord =
            result.finalFingerprint->canonicalRecord;
    } else if (result.partialFingerprint.has_value()) {
        record.expectedPartialRecord =
            result.partialFingerprint->canonicalRecord;
    }
    return record;
}

void setFailure(
    DelaunayConstructorResult& result,
    DelaunayConstructorStatus status,
    std::string detailText,
    PointId pointId,
    std::size_t insertionIndex,
    const DelaunayConstructorOptions& options) {
    result.status = status;
    result.detail = std::move(detailText);
    result.failurePointId = pointId;
    result.failureInsertionIndex = insertionIndex;
    if (options.telemetryEnabled) {
        ++result.telemetry.insertionFailures;
    }

    try {
        if (!result.arena.slots().empty() &&
            validateDelaunayTopology(
                result.arena.slots(),
                result.canonicalSites).ok()) {
            result.partialFingerprint =
                buildDelaunayFingerprint(
                    result.arena,
                    result.canonicalSites);
        }
    } catch (...) {
        // Primary typed failure is preserved. A diagnostic fingerprint is
        // optional and never changes failure semantics.
    }

    updateFinalStorageTelemetry(
        result, options.telemetryEnabled);
    if (options.captureReplayDecisions) {
        result.replayRecord =
            makeReplayRecord(result, options);
    }
}

} // namespace

const char* delaunayConstructorStatusName(
    DelaunayConstructorStatus status) noexcept {
    switch (status) {
    case DelaunayConstructorStatus::Success:
        return "Success";
    case DelaunayConstructorStatus::LowerDimensional:
        return "LowerDimensional";
    case DelaunayConstructorStatus::InvalidInput:
        return "InvalidInput";
    case DelaunayConstructorStatus::InvalidInsertionOrder:
        return "InvalidInsertionOrder";
    case DelaunayConstructorStatus::WalkFailure:
        return "WalkFailure";
    case DelaunayConstructorStatus::LocationOracleMismatch:
        return "LocationOracleMismatch";
    case DelaunayConstructorStatus::VertexStateInconsistency:
        return "VertexStateInconsistency";
    case DelaunayConstructorStatus::SeedPlanFailure:
        return "SeedPlanFailure";
    case DelaunayConstructorStatus::PlanFailure:
        return "PlanFailure";
    case DelaunayConstructorStatus::ResourceLimit:
        return "ResourceLimit";
    case DelaunayConstructorStatus::ResourceFailure:
        return "ResourceFailure";
    case DelaunayConstructorStatus::CommitFailure:
        return "CommitFailure";
    case DelaunayConstructorStatus::PostValidationFailure:
        return "PostValidationFailure";
    }
    return "Unknown";
}

DelaunayFingerprint buildDelaunayFingerprint(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> canonicalSites) {
    DelaunayFingerprint result;

    result.canonicalSites.reserve(
        canonicalSites.size());
    std::set<PointId> siteIds;
    for (const auto& site : canonicalSites) {
        if (site.id == InvalidPointId ||
            !siteIds.insert(site.id).second) {
            throw std::invalid_argument(
                "D26DT1 requires unique non-zero canonical PointIds");
        }
        result.canonicalSites.push_back({
            site.id,
            canonicalCoordinateBits(site.point.x),
            canonicalCoordinateBits(site.point.y),
            canonicalCoordinateBits(site.point.z)});
    }
    std::sort(
        result.canonicalSites.begin(),
        result.canonicalSites.end(),
        [](const auto& lhs, const auto& rhs) {
            return lhs.id < rhs.id;
        });

    std::set<std::array<PointId, 3>> uniqueHull;
    std::size_t ghostCount = 0U;
    for (const auto& slot : arena.slots()) {
        if (!slot.live) {
            continue;
        }
        if (finiteCell(slot.record)) {
            result.finiteTets.push_back(
                finiteTetIds(slot.record));
            continue;
        }
        if (!ghostCell(slot.record)) {
            throw std::invalid_argument(
                "D26DT1 encountered invalid live M2 cell");
        }
        ++ghostCount;
        const auto hull = ghostHullIds(slot.record);
        if (!uniqueHull.insert(hull).second) {
            throw std::logic_error(
                "D26DT1 duplicate hull triangle from ghost topology");
        }
        result.hullFacets.push_back(hull);
    }

    std::sort(
        result.finiteTets.begin(),
        result.finiteTets.end());
    std::sort(
        result.hullFacets.begin(),
        result.hullFacets.end());
    if (ghostCount != result.hullFacets.size()) {
        throw std::logic_error(
            "D26DT1 ghost count does not equal canonical hull facet count");
    }

    // Checked record-size envelope before serialization. The actual stream
    // remains the human-diffable correctness authority.
    std::size_t recordBudget = 256U;
    recordBudget = checkedSizeAdd(
        recordBudget,
        checkedSizeMultiply(
            result.canonicalSites.size(),
            96U,
            "D26DT1 site serialization size overflow"),
        "D26DT1 site serialization size overflow");
    recordBudget = checkedSizeAdd(
        recordBudget,
        checkedSizeMultiply(
            result.finiteTets.size(),
            96U,
            "D26DT1 tetra serialization size overflow"),
        "D26DT1 tetra serialization size overflow");
    recordBudget = checkedSizeAdd(
        recordBudget,
        checkedSizeMultiply(
            result.hullFacets.size(),
            80U,
            "D26DT1 hull serialization size overflow"),
        "D26DT1 hull serialization size overflow");

    std::ostringstream out;
    out << D26FingerprintSchemaId << '\n'
        << "site_policy=" << D26SitePolicyId << '\n'
        << "symbolic_policy=" << D26SymbolicPolicyId << '\n'
        << "fingerprint_schema=" << D26FingerprintSchemaId << '\n'
        << "canonical_sites="
        << result.canonicalSites.size() << '\n'
        << "finite_tets="
        << result.finiteTets.size() << '\n'
        << "hull_facets="
        << result.hullFacets.size() << '\n';

    for (const auto& site : result.canonicalSites) {
        out << "site " << site.id << ' '
            << hex64(site.xBits) << ' '
            << hex64(site.yBits) << ' '
            << hex64(site.zBits) << '\n';
    }
    for (const auto& tet : result.finiteTets) {
        out << "tet " << tet[0] << ' ' << tet[1]
            << ' ' << tet[2] << ' ' << tet[3] << '\n';
    }
    for (const auto& hull : result.hullFacets) {
        out << "hull " << hull[0] << ' '
            << hull[1] << ' ' << hull[2] << '\n';
    }

    result.canonicalRecord = out.str();
    if (result.canonicalRecord.size() > recordBudget) {
        // Budget is conservative for decimal uint64 text. If it ever stops
        // dominating, fail rather than silently weaken checked-size policy.
        throw std::length_error(
            "D26DT1 serialization exceeded checked size envelope");
    }
    result.digestHex =
        digestHex(result.canonicalRecord);
    return result;
}

DelaunayConstructorResult constructDelaunayReference(
    std::span<const InputSite> input,
    const DelaunayConstructorOptions& options) {
    DelaunayConstructorResult result;
    result.telemetry.sitesInput =
        static_cast<std::uint64_t>(input.size());
    PredicateAuditScope predicateScope(
        options.telemetryEnabled,
        result.telemetry);
    const auto finish = [&]() -> DelaunayConstructorResult {
        predicateScope.flush();
        return std::move(result);
    };

    try {
        result.canonicalSites =
            canonicalizeSites(input);
        result.telemetry.sitesCanonical =
            static_cast<std::uint64_t>(
                result.canonicalSites.size());
        result.affineDimension =
            classifyAffineDimension(
                result.canonicalSites);
    } catch (const std::exception& error) {
        result.status =
            DelaunayConstructorStatus::InvalidInput;
        result.detail = error.what();
        return finish();
    }

    result.requestedInsertionOrder =
        options.fullInsertionOrder.empty()
            ? defaultFullOrder(result.canonicalSites)
            : options.fullInsertionOrder;
    if (!validateFullOrder(
            result.canonicalSites,
            result.requestedInsertionOrder)) {
        result.status =
            DelaunayConstructorStatus::InvalidInsertionOrder;
        result.detail =
            "P1F explicit insertion order is not an exact full canonical PointId permutation";
        return finish();
    }

    DelaunayBootstrapResult bootstrap;
    try {
        bootstrap = buildDelaunayBootstrap(
            result.canonicalSites);
    } catch (const std::exception& error) {
        result.status =
            DelaunayConstructorStatus::InvalidInput;
        result.detail = error.what();
        return finish();
    }

    result.affineDimension =
        bootstrap.affineDimension;
    result.bootstrapBasis =
        bootstrap.basisPointIds;
    result.arena = std::move(bootstrap.arena);

    if (bootstrap.status !=
        DelaunayBootstrapStatus::Ready) {
        result.status =
            DelaunayConstructorStatus::LowerDimensional;
        result.detail =
            "P1F input canonical affine dimension is below three";
        updateFinalStorageTelemetry(
            result, options.telemetryEnabled);
        return finish();
    }

    result.effectiveInsertionOrder =
        effectiveOrder(
            result.requestedInsertionOrder,
            result.bootstrapBasis);

    try {
        result.validatedStates.push_back(
            validatedState(
                result.arena,
                result.canonicalSites,
                0U));
        if (options.telemetryEnabled) {
            ++result.telemetry.topologyValidations;
        }
        updatePeakLive(
            result, options.telemetryEnabled);
    } catch (const std::exception& error) {
        result.status =
            DelaunayConstructorStatus::PostValidationFailure;
        result.detail = error.what();
        return finish();
    }

    for (std::size_t insertionIndex = 0U;
         insertionIndex <
             result.effectiveInsertionOrder.size();
         ++insertionIndex) {
        const PointId queryId =
            result.effectiveInsertionOrder[insertionIndex];
        const auto& querySite =
            siteById(result.canonicalSites, queryId);

        if (options.telemetryEnabled) {
            ++result.telemetry.insertionCalls;
        }

        DelaunayWalkTelemetry walkTelemetry;
        const DelaunayWalkResult walk =
            locateDeterministicWalk(
                result.arena.slots(),
                result.canonicalSites,
                querySite.point,
                std::nullopt,
                options.telemetryEnabled
                    ? &walkTelemetry
                    : nullptr);

        if (options.telemetryEnabled) {
            checkedCounterAdd(
                result.telemetry.locateCalls,
                walkTelemetry.locateCalls,
                "P1F locate telemetry overflow");
            checkedCounterAdd(
                result.telemetry.walkSteps,
                walkTelemetry.walkSteps,
                "P1F walk-step telemetry overflow");
            checkedCounterAdd(
                result.telemetry.walkStalls,
                walkTelemetry.walkStalls,
                "P1F walk-stall telemetry overflow");
            result.telemetry.maxWalk =
                std::max(
                    result.telemetry.maxWalk,
                    walkTelemetry.maxWalkSteps);
        }

        if (!walk.ok()) {
            setFailure(
                result,
                DelaunayConstructorStatus::WalkFailure,
                walk.detail,
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        if (options.compareBruteForceLocation) {
            try {
                const DelaunayLocationResult oracle =
                    locateBruteForceExact(
                        result.arena.slots(),
                        result.canonicalSites,
                        querySite.point);
                if (options.telemetryEnabled) {
                    ++result.telemetry.bruteForceLocationCalls;
                    checkedCounterAdd(
                        result.telemetry
                            .bruteForceLocationFiniteCellsTested,
                        static_cast<std::uint64_t>(
                            oracle.finiteCellsTested),
                        "P1F brute-force location telemetry overflow");
                }
                if (!locationSemanticAgreement(
                        *walk.location, oracle)) {
                    if (options.telemetryEnabled) {
                        ++result.telemetry.locationMismatches;
                    }
                    setFailure(
                        result,
                        DelaunayConstructorStatus::LocationOracleMismatch,
                        "P1F P1E walk disagrees with independent P1C location oracle",
                        queryId,
                        insertionIndex,
                        options);
                    return finish();
                }
            } catch (const std::exception& error) {
                setFailure(
                    result,
                    DelaunayConstructorStatus::LocationOracleMismatch,
                    error.what(),
                    queryId,
                    insertionIndex,
                    options);
                return finish();
            }
        }

        if (walk.location->kind ==
                DelaunayLocationKind::Vertex ||
            !walk.conflictSeed.has_value()) {
            setFailure(
                result,
                DelaunayConstructorStatus::VertexStateInconsistency,
                "P1F uninserted canonical site resolved as existing vertex or lacked a verified seed",
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        if (options.captureReplayDecisions) {
            try {
                recordLocationDecisions(result, walk);
                recordConflictDecisions(
                    result,
                    result.arena,
                    result.canonicalSites,
                    queryId);
            } catch (const std::exception& error) {
                setFailure(
                    result,
                    DelaunayConstructorStatus::PlanFailure,
                    error.what(),
                    queryId,
                    insertionIndex,
                    options);
                return finish();
            }
        }

        DelaunayPlanResult planResult =
            buildDelaunayInsertionPlanFromVerifiedSeed(
                result.arena,
                result.canonicalSites,
                queryId,
                walk.conflictSeed->cell);

        if (!planResult.ok()) {
            if (options.telemetryEnabled) {
                ++result.telemetry.transactionPlanFailures;
                if (planResult.failure ==
                    DelaunayTransactionFailure::DisconnectedCavity) {
                    ++result.telemetry.conflictOracleMismatches;
                }
            }
            const DelaunayConstructorStatus status =
                planResult.failure ==
                        DelaunayTransactionFailure::VerifiedSeedNotConflict
                    ? DelaunayConstructorStatus::SeedPlanFailure
                    : DelaunayConstructorStatus::PlanFailure;
            setFailure(
                result,
                status,
                planResult.detail,
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        DelaunayInsertionPlan plan =
            std::move(*planResult.plan);
        if (!plan.verifiedConflictSeed.has_value() ||
            *plan.verifiedConflictSeed !=
                walk.conflictSeed->cell ||
            plan.conflictOracle != plan.conflictFlood) {
            if (options.telemetryEnabled) {
                ++result.telemetry.conflictOracleMismatches;
            }
            setFailure(
                result,
                DelaunayConstructorStatus::SeedPlanFailure,
                "P1F seed-driven plan evidence does not equal P1D exact conflict oracle",
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        if (options.telemetryEnabled) {
            checkedCounterAdd(
                result.telemetry.conflictOracleCellsTested,
                static_cast<std::uint64_t>(
                    plan.sourceLiveCount),
                "P1F conflict-oracle telemetry overflow");
            checkedCounterAdd(
                result.telemetry.cavityCells,
                static_cast<std::uint64_t>(
                    plan.conflictOracle.size()),
                "P1F cavity telemetry overflow");
            checkedCounterAdd(
                result.telemetry.internalFacets,
                static_cast<std::uint64_t>(
                    plan.internalFacets.size()),
                "P1F internal-facet telemetry overflow");
            checkedCounterAdd(
                result.telemetry.boundaryFacets,
                static_cast<std::uint64_t>(
                    plan.boundaryFacets.size()),
                "P1F boundary-facet telemetry overflow");
            checkedCounterAdd(
                result.telemetry.candidateCells,
                static_cast<std::uint64_t>(
                    plan.candidateCells.size()),
                "P1F candidate telemetry overflow");
            result.telemetry.maxCavity =
                std::max<std::uint64_t>(
                    result.telemetry.maxCavity,
                    static_cast<std::uint64_t>(
                        plan.conflictOracle.size()));
        }

        const DelaunayTransactionResult reserve =
            reserveDelaunayInsertion(
                result.arena,
                result.canonicalSites,
                plan,
                options.resourceLimits);
        if (!reserve.ok()) {
            if (options.telemetryEnabled) {
                ++result.telemetry
                      .transactionReserveFailures;
            }
            const bool resourceLimit =
                reserve.failure ==
                    DelaunayTransactionFailure::ResourceLimit ||
                reserve.failure ==
                    DelaunayTransactionFailure::CapacityOverflow;
            setFailure(
                result,
                resourceLimit
                    ? DelaunayConstructorStatus::ResourceLimit
                    : DelaunayConstructorStatus::ResourceFailure,
                reserve.detail,
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        const DelaunayTransactionResult commit =
            commitDelaunayInsertion(
                result.arena,
                result.canonicalSites,
                plan);
        if (!commit.ok() ||
            !commit.commitBarrierCrossed) {
            if (options.telemetryEnabled) {
                ++result.telemetry
                      .transactionCommitFailures;
            }
            setFailure(
                result,
                DelaunayConstructorStatus::CommitFailure,
                commit.detail.empty()
                    ? "P1F commit did not cross the qualified transaction barrier"
                    : commit.detail,
                queryId,
                insertionIndex,
                options);
            return finish();
        }

        ++result.completedInsertions;
        if (options.telemetryEnabled) {
            ++result.telemetry.insertionSuccesses;
        }
        if (options.captureReplayDecisions) {
            DelaunayReplayDecision decision;
            decision.kind =
                DelaunayReplayDecisionKind::Commit;
            decision.value =
                result.completedInsertions;
            decision.ids = {queryId};
            result.replayDecisions.push_back(
                std::move(decision));
        }

        try {
            result.validatedStates.push_back(
                validatedState(
                    result.arena,
                    result.canonicalSites,
                    result.completedInsertions));
            if (options.telemetryEnabled) {
                ++result.telemetry.topologyValidations;
            }
            updatePeakLive(
                result, options.telemetryEnabled);
        } catch (const std::exception& error) {
            setFailure(
                result,
                DelaunayConstructorStatus::PostValidationFailure,
                error.what(),
                queryId,
                insertionIndex,
                options);
            return finish();
        }
    }

    try {
        result.finalFingerprint =
            buildDelaunayFingerprint(
                result.arena,
                result.canonicalSites);
    } catch (const std::exception& error) {
        result.status =
            DelaunayConstructorStatus::PostValidationFailure;
        result.detail = error.what();
        updateFinalStorageTelemetry(
            result, options.telemetryEnabled);
        return finish();
    }

    result.status = DelaunayConstructorStatus::Success;
    updateFinalStorageTelemetry(
        result, options.telemetryEnabled);
    if (options.captureReplayDecisions) {
        result.replayRecord =
            makeReplayRecord(result, options);
    }
    return finish();
}

std::string serializeDelaunayReplay(
    const DelaunayReplayRecord& record) {
    std::ostringstream out;
    out << record.replaySchema << '\n'
        << "site_policy=" << record.sitePolicy << '\n'
        << "symbolic_policy="
        << record.symbolicPolicy << '\n'
        << "fingerprint_schema="
        << record.fingerprintSchema << '\n'
        << "max_total_slots="
        << record.maxTotalSlots << '\n'
        << "expected_status="
        << static_cast<unsigned>(record.expectedStatus)
        << '\n'
        << "failure_point="
        << (record.failurePointId.has_value()
                ? std::to_string(*record.failurePointId)
                : std::string("none"))
        << '\n'
        << "failure_index="
        << (record.failureInsertionIndex.has_value()
                ? std::to_string(
                      *record.failureInsertionIndex)
                : std::string("none"))
        << '\n'
        << "site_count="
        << record.canonicalSites.size() << '\n';

    for (const auto& site : record.canonicalSites) {
        out << "site=" << site.id << ','
            << hex64(site.xBits) << ','
            << hex64(site.yBits) << ','
            << hex64(site.zBits) << '\n';
    }

    out << "order_count="
        << record.fullInsertionOrder.size() << '\n'
        << "order="
        << joinIds(record.fullInsertionOrder) << '\n'
        << "expected_record_hex="
        << bytesToHex(record.expectedPartialRecord)
        << '\n'
        << "decision_count="
        << record.decisions.size() << '\n';

    for (const auto& decision : record.decisions) {
        out << "decision="
            << static_cast<unsigned>(decision.kind)
            << ',' << decision.value
            << ',' << static_cast<int>(
                decision.geometricSign)
            << ',' << static_cast<int>(
                decision.resolvedSign)
            << ',' << joinIds(decision.ids)
            << '\n';
    }
    return out.str();
}

DelaunayReplayResult replaySerializedDelaunay(
    const std::string& serialized) {
    DelaunayReplayResult replay;
    DelaunayReplayRecord record;

    try {
        std::istringstream input(serialized);
        std::string line;
        if (!std::getline(input, line) ||
            line.empty()) {
            replay.detail =
                "P1F replay record has no schema line";
            return replay;
        }
        record.replaySchema = line;

        std::size_t expectedSiteCount = 0U;
        std::size_t expectedOrderCount = 0U;
        std::size_t expectedDecisionCount = 0U;

        while (std::getline(input, line)) {
            const std::size_t equals =
                line.find('=');
            if (equals == std::string::npos) {
                throw std::invalid_argument(
                    "P1F replay line has no '=' separator");
            }
            const std::string key =
                line.substr(0U, equals);
            const std::string value =
                line.substr(equals + 1U);

            if (key == "site_policy") {
                record.sitePolicy = value;
            } else if (key == "symbolic_policy") {
                record.symbolicPolicy = value;
            } else if (key == "fingerprint_schema") {
                record.fingerprintSchema = value;
            } else if (key == "max_total_slots") {
                record.maxTotalSlots =
                    static_cast<std::size_t>(
                        parseU64(value));
            } else if (key == "expected_status") {
                const auto numeric =
                    parseU64(value);
                if (numeric >
                    static_cast<std::uint64_t>(
                        DelaunayConstructorStatus::PostValidationFailure)) {
                    throw std::invalid_argument(
                        "P1F replay constructor status is out of range");
                }
                record.expectedStatus =
                    static_cast<DelaunayConstructorStatus>(
                        numeric);
            } else if (key == "failure_point") {
                if (value != "none") {
                    record.failurePointId =
                        parseU64(value);
                }
            } else if (key == "failure_index") {
                if (value != "none") {
                    record.failureInsertionIndex =
                        static_cast<std::size_t>(
                            parseU64(value));
                }
            } else if (key == "site_count") {
                expectedSiteCount =
                    static_cast<std::size_t>(
                        parseU64(value));
            } else if (key == "site") {
                std::array<std::string, 4> fields;
                std::size_t begin = 0U;
                for (std::size_t field = 0U;
                     field < 4U; ++field) {
                    const std::size_t comma =
                        value.find(',', begin);
                    if (field < 3U &&
                        comma == std::string::npos) {
                        throw std::invalid_argument(
                            "P1F replay site field is incomplete");
                    }
                    fields[field] =
                        comma == std::string::npos
                            ? value.substr(begin)
                            : value.substr(
                                  begin,
                                  comma - begin);
                    begin =
                        comma == std::string::npos
                            ? value.size()
                            : comma + 1U;
                }
                record.canonicalSites.push_back({
                    parseU64(fields[0]),
                    parseU64(fields[1], 16),
                    parseU64(fields[2], 16),
                    parseU64(fields[3], 16)});
            } else if (key == "order_count") {
                expectedOrderCount =
                    static_cast<std::size_t>(
                        parseU64(value));
            } else if (key == "order") {
                record.fullInsertionOrder =
                    parseIds(value);
            } else if (key == "expected_record_hex") {
                record.expectedPartialRecord =
                    hexToBytes(value);
            } else if (key == "decision_count") {
                expectedDecisionCount =
                    static_cast<std::size_t>(
                        parseU64(value));
            } else if (key == "decision") {
                std::array<std::string, 5> fields;
                std::size_t begin = 0U;
                for (std::size_t field = 0U;
                     field < 5U; ++field) {
                    const std::size_t comma =
                        value.find(',', begin);
                    if (field < 4U &&
                        comma == std::string::npos) {
                        throw std::invalid_argument(
                            "P1F replay decision field is incomplete");
                    }
                    fields[field] =
                        comma == std::string::npos
                            ? value.substr(begin)
                            : value.substr(
                                  begin,
                                  comma - begin);
                    begin =
                        comma == std::string::npos
                            ? value.size()
                            : comma + 1U;
                }
                const auto kind = parseU64(fields[0]);
                if (kind >
                    static_cast<std::uint64_t>(
                        DelaunayReplayDecisionKind::Commit)) {
                    throw std::invalid_argument(
                        "P1F replay decision kind is out of range");
                }
                const int geometric =
                    std::stoi(fields[2]);
                const int resolved =
                    std::stoi(fields[3]);
                if (geometric < -1 || geometric > 1 ||
                    resolved < -1 || resolved > 1) {
                    throw std::invalid_argument(
                        "P1F replay predicate sign is out of range");
                }
                record.decisions.push_back({
                    static_cast<DelaunayReplayDecisionKind>(
                        kind),
                    parseU64(fields[1]),
                    parseIds(fields[4]),
                    static_cast<PredicateSign>(geometric),
                    static_cast<PredicateSign>(resolved)});
            }
        }

        if (record.canonicalSites.size() !=
                expectedSiteCount ||
            record.fullInsertionOrder.size() !=
                expectedOrderCount ||
            record.decisions.size() !=
                expectedDecisionCount) {
            throw std::invalid_argument(
                "P1F replay declared cardinality does not match record");
        }
    } catch (const std::exception& error) {
        replay.status =
            DelaunayReplayStatus::ParseError;
        replay.detail = error.what();
        return replay;
    }

    if (record.replaySchema != D26ReplaySchemaId ||
        record.sitePolicy != D26SitePolicyId ||
        record.symbolicPolicy !=
            D26SymbolicPolicyId ||
        record.fingerprintSchema !=
            D26FingerprintSchemaId) {
        replay.status =
            DelaunayReplayStatus::PolicyMismatch;
        replay.detail =
            "P1F replay policy/schema identifier mismatch";
        return replay;
    }

    std::vector<InputSite> raw;
    raw.reserve(record.canonicalSites.size());
    for (const auto& site : record.canonicalSites) {
        raw.push_back({
            {
                std::bit_cast<double>(site.xBits),
                std::bit_cast<double>(site.yBits),
                std::bit_cast<double>(site.zBits)},
            site.id});
    }

    DelaunayConstructorOptions options;
    options.fullInsertionOrder =
        record.fullInsertionOrder;
    options.resourceLimits.maxTotalSlots =
        record.maxTotalSlots;
    options.captureReplayDecisions = true;
    options.compareBruteForceLocation = true;
    options.telemetryEnabled = true;

    DelaunayConstructorResult constructed =
        constructDelaunayReference(raw, options);

    const std::string actualRecord =
        constructed.finalFingerprint.has_value()
            ? constructed.finalFingerprint->canonicalRecord
            : constructed.partialFingerprint.has_value()
                  ? constructed.partialFingerprint->canonicalRecord
                  : std::string{};

    if (constructed.status != record.expectedStatus ||
        constructed.failurePointId !=
            record.failurePointId ||
        constructed.failureInsertionIndex !=
            record.failureInsertionIndex ||
        actualRecord != record.expectedPartialRecord ||
        constructed.replayDecisions !=
            record.decisions) {
        replay.status =
            DelaunayReplayStatus::ConstructorMismatch;
        replay.detail =
            "P1F standalone replay did not reproduce status/state/decision prefix";
        replay.constructor =
            std::move(constructed);
        return replay;
    }

    replay.status = DelaunayReplayStatus::Success;
    replay.constructor =
        std::move(constructed);
    return replay;
}

} // namespace femcae::meshing::m2
