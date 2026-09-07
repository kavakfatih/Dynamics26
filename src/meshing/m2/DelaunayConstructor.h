#pragma once

#include "DelaunayPredicates.h"
#include "DelaunayTransaction.h"
#include "DelaunayWalk.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace femcae::meshing::m2 {

inline constexpr const char* D26SitePolicyId = "D26SITE1";
inline constexpr const char* D26SymbolicPolicyId = "D26LIFT1";
inline constexpr const char* D26FingerprintSchemaId = "D26DT1";
inline constexpr const char* D26ReplaySchemaId = "D26DT-REPLAY1";

enum class DelaunayConstructorStatus : std::uint8_t {
    Success = 0,
    LowerDimensional,
    InvalidInput,
    InvalidInsertionOrder,
    WalkFailure,
    LocationOracleMismatch,
    VertexStateInconsistency,
    SeedPlanFailure,
    PlanFailure,
    ResourceLimit,
    ResourceFailure,
    CommitFailure,
    PostValidationFailure
};

struct DelaunayCanonicalSiteBits {
    PointId id{InvalidPointId};
    std::uint64_t xBits{0};
    std::uint64_t yBits{0};
    std::uint64_t zBits{0};

    friend bool operator==(
        const DelaunayCanonicalSiteBits&,
        const DelaunayCanonicalSiteBits&) = default;
};

struct DelaunayFingerprint {
    std::string sitePolicy{D26SitePolicyId};
    std::string symbolicPolicy{D26SymbolicPolicyId};
    std::string schema{D26FingerprintSchemaId};
    std::vector<DelaunayCanonicalSiteBits> canonicalSites;
    std::vector<std::array<PointId, 4>> finiteTets;
    std::vector<std::array<PointId, 3>> hullFacets;
    std::string canonicalRecord;
    // Convenience only. canonicalRecord remains the correctness authority.
    std::string digestHex;

    friend bool operator==(
        const DelaunayFingerprint& lhs,
        const DelaunayFingerprint& rhs) {
        return lhs.canonicalRecord == rhs.canonicalRecord;
    }
};

struct DelaunayPredicatePathTelemetry {
    std::uint64_t calls{0};
    std::uint64_t fast{0};
    std::uint64_t exact{0};
    std::uint64_t zero{0};
    std::uint64_t invalid{0};
};

struct DelaunayConstructorTelemetry {
    std::uint64_t sitesInput{0};
    std::uint64_t sitesCanonical{0};

    std::uint64_t insertionCalls{0};
    std::uint64_t insertionSuccesses{0};
    std::uint64_t insertionFailures{0};

    std::uint64_t locateCalls{0};
    std::uint64_t walkSteps{0};
    std::uint64_t maxWalk{0};
    std::uint64_t walkStalls{0};

    // P1C location oracle and P1D conflict oracle are deliberately separate.
    std::uint64_t bruteForceLocationCalls{0};
    std::uint64_t bruteForceLocationFiniteCellsTested{0};
    std::uint64_t locationMismatches{0};
    std::uint64_t conflictOracleCellsTested{0};
    std::uint64_t conflictOracleMismatches{0};

    std::uint64_t cavityCells{0};
    std::uint64_t internalFacets{0};
    std::uint64_t boundaryFacets{0};
    std::uint64_t candidateCells{0};
    std::uint64_t maxCavity{0};

    std::uint64_t transactionPlanFailures{0};
    std::uint64_t transactionReserveFailures{0};
    std::uint64_t transactionCommitFailures{0};

    DelaunayPredicatePathTelemetry orient2d;
    DelaunayPredicatePathTelemetry orient3d;
    DelaunayPredicatePathTelemetry incircle;
    DelaunayPredicatePathTelemetry insphere;
    std::uint64_t symbolicInsphereTies{0};
    std::uint64_t symbolicIncircleTies{0};

    std::uint64_t topologyValidations{0};
    std::uint64_t finiteLive{0};
    std::uint64_t ghostLive{0};
    std::uint64_t peakLive{0};
    std::uint64_t slotsTotal{0};
    std::uint64_t slotsDead{0};
    std::uint64_t hullFacets{0};

    double finiteTetsPerSite{0.0};
    double unifiedCellsPerSite{0.0};
};

struct DelaunayValidatedStateEvidence {
    std::size_t completedInsertions{0};
    std::size_t finiteCells{0};
    std::size_t ghostCells{0};
    std::size_t liveCells{0};
    std::size_t totalSlots{0};
    std::size_t hullFacets{0};
    std::uint64_t topologyVersion{0};
};

enum class DelaunayReplayDecisionKind : std::uint8_t {
    Location = 0,
    WalkCrossing,
    ConflictSeed,
    FiniteConflict,
    GhostHalfspace,
    GhostCoplanarCircle,
    Commit
};

struct DelaunayReplayDecision {
    DelaunayReplayDecisionKind kind{DelaunayReplayDecisionKind::Location};
    std::uint64_t value{0};
    std::vector<PointId> ids;
    predicates::PredicateSign geometricSign{predicates::PredicateSign::Zero};
    predicates::PredicateSign resolvedSign{predicates::PredicateSign::Zero};

    friend bool operator==(
        const DelaunayReplayDecision&,
        const DelaunayReplayDecision&) = default;
};

struct DelaunayReplayRecord {
    std::string replaySchema{D26ReplaySchemaId};
    std::string sitePolicy{D26SitePolicyId};
    std::string symbolicPolicy{D26SymbolicPolicyId};
    std::string fingerprintSchema{D26FingerprintSchemaId};

    std::vector<DelaunayCanonicalSiteBits> canonicalSites;
    std::vector<PointId> fullInsertionOrder;
    std::size_t maxTotalSlots{0};

    DelaunayConstructorStatus expectedStatus{
        DelaunayConstructorStatus::InvalidInput};
    std::optional<PointId> failurePointId;
    std::optional<std::size_t> failureInsertionIndex;
    std::string expectedPartialRecord;
    std::vector<DelaunayReplayDecision> decisions;
};

struct DelaunayConstructorOptions {
    // Empty => ascending canonical PointId full order.
    // Non-empty => exact full canonical PointId permutation; bootstrap IDs are
    // removed only after full-domain validation.
    std::vector<PointId> fullInsertionOrder;
    DelaunayResourceLimits resourceLimits{};
    bool compareBruteForceLocation{true};
    bool captureReplayDecisions{false};
    bool telemetryEnabled{true};
};

struct DelaunayConstructorResult {
    DelaunayConstructorStatus status{DelaunayConstructorStatus::InvalidInput};
    std::string detail;
    AffineDimension affineDimension{AffineDimension::Empty};

    std::vector<CanonicalSite> canonicalSites;
    std::array<PointId, 4> bootstrapBasis{
        InvalidPointId, InvalidPointId, InvalidPointId, InvalidPointId};
    std::vector<PointId> requestedInsertionOrder;
    std::vector<PointId> effectiveInsertionOrder;
    std::size_t completedInsertions{0};
    std::optional<PointId> failurePointId;
    std::optional<std::size_t> failureInsertionIndex;

    DelaunayReferenceArena arena;
    std::optional<DelaunayFingerprint> finalFingerprint;
    std::optional<DelaunayFingerprint> partialFingerprint;
    DelaunayConstructorTelemetry telemetry;
    std::vector<DelaunayValidatedStateEvidence> validatedStates;
    std::vector<DelaunayReplayDecision> replayDecisions;
    std::optional<DelaunayReplayRecord> replayRecord;

    [[nodiscard]] bool ok() const noexcept {
        return status == DelaunayConstructorStatus::Success &&
               finalFingerprint.has_value();
    }
};

enum class DelaunayReplayStatus : std::uint8_t {
    Success = 0,
    ParseError,
    PolicyMismatch,
    ConstructorMismatch
};

struct DelaunayReplayResult {
    DelaunayReplayStatus status{DelaunayReplayStatus::ParseError};
    std::string detail;
    std::optional<DelaunayConstructorResult> constructor;

    [[nodiscard]] bool ok() const noexcept {
        return status == DelaunayReplayStatus::Success;
    }
};

// Raw InputSite boundary -> D26SITE1 -> deterministic P1B bootstrap ->
// P1E walk/seed -> P1D seed-driven flood + independent conflict oracle ->
// transaction -> exact post-state validation -> D26DT1.
[[nodiscard]] DelaunayConstructorResult constructDelaunayReference(
    std::span<const InputSite> input,
    const DelaunayConstructorOptions& options = {});

[[nodiscard]] DelaunayFingerprint buildDelaunayFingerprint(
    const DelaunayReferenceArena& arena,
    std::span<const CanonicalSite> canonicalSites);

[[nodiscard]] std::string serializeDelaunayReplay(
    const DelaunayReplayRecord& record);

[[nodiscard]] DelaunayReplayResult replaySerializedDelaunay(
    const std::string& serialized);

[[nodiscard]] const char* delaunayConstructorStatusName(
    DelaunayConstructorStatus status) noexcept;

} // namespace femcae::meshing::m2
