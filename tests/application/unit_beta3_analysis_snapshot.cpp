#include <femcae/application/AnalysisSnapshot.h>
#include <femcae/application/SolverInputBuilder.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>
#include <type_traits>
#include <utility>

using namespace femcae::application;

namespace {

int failures = 0;

void expect(const bool condition, const std::string_view label)
{
    if (condition) {
        std::cout << "PASS " << label << '\n';
    } else {
        std::cerr << "FAIL " << label << '\n';
        ++failures;
    }
}

AnalysisSnapshotDraft nonlinearCubeDraft()
{
    AnalysisSnapshotDraft draft;
    draft.analysisKind = SnapshotAnalysisKind::NonlinearStatic;
    draft.largeDeformation = true;
    draft.nodes = {
        {81, {0.0, 0.0, 0.0}}, {7, {1.0, 0.0, 0.0}},
        {42, {1.0, 1.0, 0.0}}, {5, {0.0, 1.0, 0.0}},
        {900, {0.0, 0.0, 1.0}}, {11, {1.0, 0.0, 1.0}},
        {3, {1.0, 1.0, 1.0}}, {77, {0.0, 1.0, 1.0}}
    };
    draft.material = {19, "Linear Elastic / StVK reference", SnapshotMaterialModel::LinearElastic,
                      210.0e9, 0.30};
    draft.elements = {{600, {81, 7, 42, 5, 900, 11, 3, 77}, 19,
                       SnapshotHex8Formulation::TotalLagrangianDisplacement}};
    draft.constraints = {
        {81, 1, 0.0}, {81, 2, 0.0}, {81, 3, 0.0},
        {5, 1, 0.0}, {5, 2, 0.0}, {5, 3, 0.0},
        {900, 1, 0.0}, {900, 2, 0.0}, {900, 3, 0.0},
        {77, 1, 0.0}, {77, 2, 0.0}, {77, 3, 0.0},
        {81, 1, 0.0} // overlap eden support scope canonicalization testi
    };
    draft.nodalLoads = {
        {7, 1, 100.0}, {7, 1, 25.0}, {42, 1, 125.0},
        {11, 1, 125.0}, {3, 1, 125.0}
    };
    draft.nonlinearControls.method = SnapshotNonlinearMethod::FullNewton;
    draft.linearSystem = {SnapshotLinearBackend::DenseReference,
                          SnapshotMatrixSymmetry::Symmetric,
                          SnapshotMatrixDefiniteness::SpdExpected};
    draft.requestedResults = {SnapshotResultField::TotalDeformation,
                              SnapshotResultField::EquivalentStress,
                              SnapshotResultField::ReactionForce};
    return draft;
}

} // namespace

int main()
{
    static_assert(std::is_same_v<
        decltype(std::declval<const AnalysisSnapshot &>().nodes()),
        const std::vector<SnapshotNode> &>);
    static_assert(!std::is_assignable_v<AnalysisSnapshot &, AnalysisSnapshot>);

    AnalysisSnapshotDraft draft = nonlinearCubeDraft();
    AnalysisSnapshotBuildResult built = AnalysisSnapshotBuilder::build(draft);
    expect(built.success(), "versioned nonlinear snapshot accepted");
    if (!built.success()) {
        std::cerr << built.detail << '\n';
        return 1;
    }

    // Builder draft'i değer semantiğiyle dondurur; sonraki document mutation
    // solver input'ını değiştiremez.
    draft.nodes.front().coordinatesSI.x = 999.0;
    draft.nodalLoads.front().valueSI = -999.0;
    expect(built.snapshot->nodes().front().coordinatesSI.x == 0.0,
           "snapshot detached from mutable document draft");
    expect(built.snapshot->nodalLoads().front().valueSI == 100.0,
           "snapshot owns equivalent nodal loads");

    SolverInputBuildResult flattened = SolverInputBuilder::buildHex8(*built.snapshot);
    expect(flattened.success(), "snapshot flattened for C ABI");
    if (flattened.success()) {
        const Hex8SolverInput &input = *flattened.input;
        expect(input.nodeIds.size() == 8 && input.coordinatesXYZ.size() == 24,
               "node ID and SI coordinate arrays preserve caller order");
        expect(input.elementIds.size() == 1 && input.connectivity8.size() == 8,
               "HEX8 connectivity is element-major");
        expect(input.constraintValues.size() == 12,
               "overlapping identical constraints canonicalized once");
        double totalX = 0.0;
        for (std::size_t i = 0; i < input.loadValues.size(); ++i) {
            if (input.loadComponents[i] == 1) {
                totalX += input.loadValues[i];
            }
        }
        expect(std::abs(totalX - 500.0) < 1.0e-12,
               "duplicate equivalent nodal loads aggregate without losing resultant");
        expect(input.nonlinearControls.method == SnapshotNonlinearMethod::FullNewton,
               "nonlinear controls survive immutable bridge");
        expect(input.requestedResults.size() == 3,
               "requested final result fields survive immutable bridge");
    }

    AnalysisSnapshotDraft dangling = nonlinearCubeDraft();
    dangling.elements.front().nodeIds.back() = 123456;
    expect(AnalysisSnapshotBuilder::build(std::move(dangling)).error
               == AnalysisSnapshotError::InvalidConnectivity,
           "dangling HEX8 connectivity rejected");

    AnalysisSnapshotDraft nonFinite = nonlinearCubeDraft();
    nonFinite.nodes.front().coordinatesSI.z = std::numeric_limits<double>::quiet_NaN();
    expect(AnalysisSnapshotBuilder::build(std::move(nonFinite)).error
               == AnalysisSnapshotError::NonFiniteCoordinate,
           "non-finite SI coordinate rejected");

    AnalysisSnapshotDraft smallKinematics = nonlinearCubeDraft();
    smallKinematics.largeDeformation = false;
    expect(AnalysisSnapshotBuilder::build(std::move(smallKinematics)).error
               == AnalysisSnapshotError::InvalidKinematics,
           "nonlinear snapshot without Large Deformation rejected");

    AnalysisSnapshotDraft badControls = nonlinearCubeDraft();
    badControls.nonlinearControls.initialLoadIncrement = 0.0;
    expect(AnalysisSnapshotBuilder::build(std::move(badControls)).error
               == AnalysisSnapshotError::InvalidNonlinearControls,
           "invalid nonlinear controls rejected before solver call");

    AnalysisSnapshotDraft conflicts = nonlinearCubeDraft();
    conflicts.constraints.push_back({81, 1, 1.0e-3});
    AnalysisSnapshotBuildResult conflictSnapshot = AnalysisSnapshotBuilder::build(std::move(conflicts));
    expect(conflictSnapshot.success(), "snapshot preserves authoring constraints before canonicalization");
    if (conflictSnapshot.success()) {
        expect(SolverInputBuilder::buildHex8(*conflictSnapshot.snapshot).error
                   == SolverInputBuildError::ConflictingConstraint,
               "conflicting prescribed values are never resolved by precedence");
    }

    std::cout << "Beta.3 analysis snapshot failures: " << failures << '\n';
    return failures == 0 ? 0 : 1;
}
