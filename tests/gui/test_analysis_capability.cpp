#include "core/AnalysisCapability.h"

#include <iostream>

namespace {

int failures = 0;

void check(const bool condition, const char *message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

d26::AnalysisCapabilityInput supportedNonlinearInput()
{
    d26::AnalysisCapabilityInput input;
    input.analysisPresent = true;
    input.analysisType = d26::AnalysisType::NonlinearStatic;
    input.geometry = d26::GeometryCapability::ParametricBox;
    input.meshPresent = true;
    input.allElementsHex8 = true;
    input.materialAssigned = true;
    input.materialModel = d26::MaterialModel::LinearElastic;
    input.largeDeformation = true;
    input.formulation = d26::ResolvedFormulation::DisplacementBased;
    input.activeFixedSupportCount = 1;
    input.activeTotalForceCount = 1;
    input.totalForceConsumerAvailable = true;
    input.linearBackend = d26::LinearBackendCapability::DenseReference;
    input.dofCount = 240;
    input.maximumDenseDofCount = 6000;
    input.nonlinearAlgorithm = d26::NonlinearAlgorithmCapability::FullNewton;
    input.nonlinearControlsValid = true;
    input.nonlinearProductConsumerAvailable = true;
    input.nonlinearFinalResultsAvailable = true;
    input.requestedResults = {
        {d26::ResultDefinitionKind::TotalDeformation, 101},
        {d26::ResultDefinitionKind::EquivalentStress, 102},
        {d26::ResultDefinitionKind::ReactionForce, 103},
    };
    return input;
}

} // namespace

int main()
{
    using namespace d26;

    AnalysisCapabilityInput input = supportedNonlinearInput();
    AnalysisCapabilityResolution resolution = AnalysisCapabilityResolver::resolve(input);
    check(resolution.solveReady(), "supported nonlinear HEX8 subset resolves Ready");
    check(resolution.matrix.symmetry == MatrixSymmetry::Symmetric
              && resolution.matrix.definiteness == MatrixDefiniteness::SpdExpected,
          "displacement-only baseline exposes symmetric/SPD-expected metadata");

    input = supportedNonlinearInput();
    input.materialAssigned = false;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::MaterialModel)->state == CapabilityState::Invalid,
          "missing nonlinear material assignment is explicitly Invalid");

    input = supportedNonlinearInput();
    input.activeFixedSupportCount = 0;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::BoundaryCondition)->state == CapabilityState::Invalid,
          "missing nonlinear Fixed Support is explicitly Invalid");

    input = supportedNonlinearInput();
    input.activeTotalForceCount = 0;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::LoadType)->state == CapabilityState::Invalid,
          "missing nonlinear Total Force is explicitly Invalid");

    input = supportedNonlinearInput();
    input.largeDeformation = false;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::Kinematics)->state == CapabilityState::Invalid,
          "nonlinear product solve requires Large Deformation explicitly");

    input.materialModel = MaterialModel::NeoHookean;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::MaterialModel) != nullptr
              && resolution.decision(CapabilityAxis::MaterialModel)->state == CapabilityState::Unavailable,
          "hyperelastic product combination is explicitly Unavailable");

    input = supportedNonlinearInput();
    input.formulation = ResolvedFormulation::MixedUP;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::IncompressibilityFormulation)->state
                  == CapabilityState::Unavailable
              && resolution.matrix.definiteness == MatrixDefiniteness::Indefinite,
          "mixed u-p combination is blocked and exposes indefinite matrix metadata");

    input = supportedNonlinearInput();
    input.activeContactCount = 1;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::Contact)->state == CapabilityState::Unavailable
              && resolution.matrix.symmetry == MatrixSymmetry::Unsymmetric,
          "active Contact is blocked without inventing product support");

    input = supportedNonlinearInput();
    input.geometry = GeometryCapability::UnsupportedCad;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::GeometrySource)->state == CapabilityState::SetupOnly,
          "non-box CAD is Setup Only without arbitrary volume mesher");

    input = supportedNonlinearInput();
    input.meshStale = true;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::MeshTopology)->state == CapabilityState::Stale,
          "stale mesh is a typed blocking state");

    input = supportedNonlinearInput();
    input.nonlinearControlsValid = false;
    input.nonlinearControlsError = QStringLiteral("invalid controls fixture");
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::NonlinearAlgorithm)->state == CapabilityState::Invalid,
          "invalid nonlinear controls are rejected by capability contract");

    input = supportedNonlinearInput();
    input.nonlinearProductConsumerAvailable = false;
    input.nonlinearFinalResultsAvailable = false;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(!resolution.solveReady()
              && resolution.decision(CapabilityAxis::AnalysisType)->state == CapabilityState::SetupOnly,
          "authoring-only nonlinear route cannot masquerade as solve-ready");

    input = supportedNonlinearInput();
    input.linearBackend = LinearBackendCapability::SparseCg;
    input.formulation = ResolvedFormulation::MixedUP;
    resolution = AnalysisCapabilityResolver::resolve(input);
    check(resolution.decision(CapabilityAxis::LinearBackend)->state == CapabilityState::Unavailable,
          "CG is unavailable for indefinite mixed formulation metadata");

    std::cout << (failures == 0 ? "Analysis capability resolver PASS" : "Analysis capability resolver FAIL")
              << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}
