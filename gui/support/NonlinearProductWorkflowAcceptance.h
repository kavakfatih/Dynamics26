#pragma once

// Dynamics26 V1.1.0-beta.3 — supported nonlinear product vertical slice.
//
// Bu acceptance verification preset'i veya demo ABI kullanmaz. Gercek
// Dynamics26MainWindow composition'i uzerinde persistent document nesnelerini
// author eder, kaydeder/açar ve AnalysisService'in immutable snapshot ->
// general nonlinear C ABI -> mevcut Fortran Newton solver zincirini calistirir.

#include "BoundarySelectionAuthoringAcceptance.h"

#include "../commands/DomainCommands.h"
#include "../core/AnalysisCapability.h"
#include "../core/DocumentCommandManager.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/ProjectNavigator.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QJsonObject>
#include <QLabel>
#include <QTemporaryDir>
#include <QUndoStack>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace d26 {

inline int runNonlinearProductWorkflowAcceptanceTest(QApplication &app,
                                                       Dynamics26MainWindow &window)
{
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const std::string &message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };
    const auto flushUi = [&app] {
        app.processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        app.processEvents();
    };

    const ServiceContext services = window.services();
    SelectionCoordinator *selectionCoordinator =
        boundary_selection_acceptance_detail::coordinator(window);
    SelectionManager *selection = selectionCoordinator != nullptr
        ? selectionCoordinator->selectionManager() : nullptr;
    DocumentCommandManager *commands = window.documentCommands();
    QUndoStack *undo = commands != nullptr ? commands->stack() : nullptr;

    check(services.project != nullptr && services.geometry != nullptr
              && services.mesh != nullptr && services.materials != nullptr
              && services.analysis != nullptr && services.namedSelections != nullptr
              && services.dependencies != nullptr && commands != nullptr && undo != nullptr,
          "B3.7 acceptance has the real project/geometry/mesh/material/analysis composition");
    if (services.project == nullptr || services.geometry == nullptr
        || services.mesh == nullptr || services.materials == nullptr
        || services.analysis == nullptr || services.namedSelections == nullptr
        || services.dependencies == nullptr || commands == nullptr || undo == nullptr) {
        return 1;
    }

    check(services.materials->assigned() != nullptr
              && services.materials->assigned()->model == MaterialModel::LinearElastic,
          "nonlinear product fixture starts from Linear Elastic authoring");
    if (services.materials->assigned() == nullptr
        || services.materials->assigned()->model != MaterialModel::LinearElastic) {
        return failures + 1;
    }

    QTemporaryDir temporary;
    check(temporary.isValid(),
          "B3.7 acceptance owns an isolated project persistence directory");
    if (!temporary.isValid()) {
        return failures + 1;
    }
    const QString baselinePath = temporary.filePath(QStringLiteral("b3_7_baseline.femcae.json"));
    const QString authoredPath = temporary.filePath(QStringLiteral("b3_7_authored.femcae.json"));
    const QString solvedPath = temporary.filePath(QStringLiteral("b3_7_solved.femcae.json"));
    const bool baselineHadMesh = services.mesh->hasMesh();
    check(window.saveProjectToPath(baselinePath),
          "B3.7 captures the pre-test document for deterministic restoration");

    // Dense reference Newton acceptance'i kucuk ve deterministik tutar. Bu
    // degisiklik document command'dir; CAD kaynagi ve fiziksel boyutlar korunur.
    const MeshService::Definition meshBefore = services.mesh->definition();
    MeshService::Definition meshForSolve = meshBefore;
    meshForSolve.nx = 2;
    meshForSolve.ny = 1;
    meshForSolve.nz = 1;
    if (meshForSolve != meshBefore) {
        commands->push(new commands::SetMeshDefinitionCommand(
            services, meshBefore, meshForSolve, QStringLiteral("Set Beta.3 acceptance mesh")));
    }
    window.selectObject(services.project->meshNode());
    flushUi();
    check(window.runCommand(QStringLiteral("mesh.generate")),
          "structured HEX8 mesh is generated through the application command");
    flushUi();
    check(services.mesh->hasMesh() && services.mesh->isUpToDate()
              && services.mesh->elementCount() == 2,
          "nonlinear fixture owns a current 2x1x1 structured HEX8 mesh");

    // NonlinearStatic is a real analysis object, not a verification-only model.
    check(window.runCommand(QStringLiteral("analysis.insertNonlinear")),
          "Nonlinear Static is inserted through the canonical shell command");
    flushUi();
    const ObjectId analysisId = window.currentAnalysis();
    const AnalysisRecord *record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->type == AnalysisType::NonlinearStatic,
          "inserted analysis preserves the existing NonlinearStatic enum identity");

    if (record != nullptr) {
        commands->push(new commands::SetLargeDeflectionCommand(
            services, analysisId, record->largeDeflection, true));
        const NonlinearSolverControls controlsBefore = record->nonlinearControls;
        NonlinearSolverControls invalidControls = controlsBefore;
        invalidControls.maximumIterations = 0;
        commands->push(new commands::SetNonlinearSolverControlsCommand(
            services, analysisId, controlsBefore, invalidControls,
            QStringLiteral("Set invalid nonlinear controls fixture")));
        const AnalysisCapabilityResolution invalidControlResolution =
            services.analysis->resolveCapabilities(analysisId);
        const CapabilityDecision *invalidAlgorithm =
            invalidControlResolution.decision(CapabilityAxis::NonlinearAlgorithm);
        check(invalidAlgorithm != nullptr
                  && invalidAlgorithm->state == CapabilityState::Invalid
                  && invalidAlgorithm->subject == record->settingsNode
                  && !services.analysis->preflight(analysisId).passed(),
              "invalid nonlinear controls fail authoritative Preflight at Analysis Settings");
        undo->undo();

        NonlinearSolverControls controlsForSolve = controlsBefore;
        controlsForSolve.method = NonlinearMethodIntent::FullNewton;
        controlsForSolve.maximumIterations = 30;
        controlsForSolve.adaptiveStepping = true;
        controlsForSolve.initialLoadIncrement = 0.25;
        controlsForSolve.minimumLoadIncrement = 1.0e-4;
        controlsForSolve.maximumLoadIncrement = 0.50;
        controlsForSolve.lineSearch = true;
        controlsForSolve.residualRelativeTolerance = 1.0e-8;
        controlsForSolve.displacementRelativeTolerance = 1.0e-8;
        commands->push(new commands::SetNonlinearSolverControlsCommand(
            services, analysisId, controlsBefore, controlsForSolve,
            QStringLiteral("Set Beta.3 nonlinear controls")));
    }
    flushUi();

    // Authoring eksikleri typed resolver -> authoritative Preflight yolunda
    // bloklanir. Basarisiz solve document/Undo state'ini degistiremez.
    const AnalysisCapabilityResolution incomplete =
        services.analysis->resolveCapabilities(analysisId);
    const CapabilityDecision *missingSupport =
        incomplete.decision(CapabilityAxis::BoundaryCondition);
    const CapabilityDecision *missingLoad = incomplete.decision(CapabilityAxis::LoadType);
    check(missingSupport != nullptr && missingSupport->state == CapabilityState::Invalid
              && missingSupport->subject == analysisId,
          "nonlinear Preflight rejects a missing Fixed Support with the exact analysis ObjectId");
    check(missingLoad != nullptr && missingLoad->state == CapabilityState::Invalid
              && missingLoad->subject == analysisId,
          "nonlinear Preflight rejects a missing Total Force with the exact analysis ObjectId");
    const QJsonObject incompleteDocument = services.analysis->analysisToJson(analysisId);
    const int undoBeforeRejectedSolve = undo->index();
    check(!services.analysis->solve(analysisId),
          "unsupported incomplete nonlinear model cannot enter a hidden linear solve");
    flushUi();
    check(services.analysis->analysisToJson(analysisId) == incompleteDocument
              && undo->index() == undoBeforeRejectedSolve,
          "failed nonlinear Preflight leaves document state and Undo history unchanged");

    ObjectId supportId = InvalidObjectId;
    ObjectId forceId = InvalidObjectId;
    ObjectId supportNamedSelectionId = InvalidObjectId;
    ObjectId forceNamedSelectionId = InvalidObjectId;
    bool usedCadFaceAuthoring = false;

    // OCCT topology gate'inde gercek CAD Face -> persistent Named Selection ->
    // consumer ObjectId yolu kullanilir. Parametric Box gate'i ayni product
    // consumer'i mevcut BoxFace scope authoring'iyle test eder.
    const auto bodies = services.geometry->bodies();
    const auto descriptor = bodies.size() == 1
        ? services.geometry->boxDescriptor(bodies.front()) : std::nullopt;
    if (descriptor.has_value() && selectionCoordinator != nullptr && selection != nullptr) {
        usedCadFaceAuthoring = true;
        const quint64 revision = services.geometry->summary().revision;
        selection->setPolicy(boundary_selection_acceptance_detail::facePolicy());
        (void)selection->apply(boundary_selection_acceptance_detail::faceItem(
                                   *services.geometry, descriptor->xMinFace, revision),
                               SelectionOperation::Replace);
        const BoundaryFromSelectionCreateResult supportCreated =
            selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
                BoundaryFromSelectionKind::FixedSupport);
        supportId = supportCreated.boundaryConditionId;
        supportNamedSelectionId = supportCreated.namedSelectionId;

        selection->setPolicy(boundary_selection_acceptance_detail::facePolicy());
        (void)selection->apply(boundary_selection_acceptance_detail::faceItem(
                                   *services.geometry, descriptor->xMaxFace, revision),
                               SelectionOperation::Replace);
        const BoundaryFromSelectionCreateResult forceCreated =
            selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
                BoundaryFromSelectionKind::TotalForce);
        forceId = forceCreated.boundaryConditionId;
        forceNamedSelectionId = forceCreated.namedSelectionId;
        check(supportCreated.success() && forceCreated.success(),
              "real CAD Face selections create Fixed Support and Total Force relationships");
    } else {
        window.selectObject(analysisId);
        check(window.runCommand(QStringLiteral("analysis.insertSupport")),
              "Parametric Box path inserts a Fixed Support through the shell command");
        record = services.analysis->analysis(analysisId);
        supportId = record != nullptr && !record->supports.isEmpty()
            ? record->supports.back() : InvalidObjectId;
        window.selectObject(analysisId);
        check(window.runCommand(QStringLiteral("analysis.insertForce")),
              "Parametric Box path inserts a Total Force through the shell command");
        record = services.analysis->analysis(analysisId);
        forceId = record != nullptr && !record->loads.isEmpty()
            ? record->loads.back() : InvalidObjectId;
    }
    flushUi();

    window.selectObject(analysisId);
    check(window.runCommand(QStringLiteral("analysis.insertDeformation")),
          "Total Deformation definition is authored through the application command");
    window.selectObject(analysisId);
    check(window.runCommand(QStringLiteral("analysis.insertStress")),
          "Equivalent Stress definition is authored through the application command");
    window.selectObject(analysisId);
    check(window.runCommand(QStringLiteral("analysis.insertReaction")),
          "Reaction Force definition is authored through the application command");
    flushUi();

    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->largeDeflection
              && record->nonlinearControls.method == NonlinearMethodIntent::FullNewton
              && record->nonlinearControls.adaptiveStepping
              && record->nonlinearControls.lineSearch
              && record->supports.size() == 1 && record->loads.size() == 1
              && record->results.size() == 3,
          "supported nonlinear document carries large deformation, controls, BC/load and three results");
    const AnalysisCapabilityResolution ready =
        services.analysis->resolveCapabilities(analysisId);
    check(ready.solveReady() && services.analysis->preflight(analysisId).passed(),
          "authoritative capability resolution and Preflight pass the supported nonlinear subset");

    const QJsonObject authoredAnalysis = services.analysis->analysisToJson(analysisId);
    const MeshService::Definition authoredMeshDefinition = services.mesh->definition();
    QVector<ObjectId> authoredResultIds;
    if (record != nullptr) {
        authoredResultIds = record->results;
    }
    check(window.saveProjectToPath(authoredPath),
          "nonlinear authoring state saves through the real project writer");
    window.newProjectWithoutPrompt();
    flushUi();
    check(window.openProjectFromPath(authoredPath),
          "nonlinear authoring state reopens through the real project reader");
    flushUi();

    record = services.analysis->analysis(analysisId);
    check(record != nullptr && services.analysis->analysisToJson(analysisId) == authoredAnalysis,
          "save/reopen preserves analysis, settings, controls, BC/load and result ObjectIds exactly");
    check(services.mesh->definition() == authoredMeshDefinition && !services.mesh->hasMesh(),
          "save/reopen preserves mesh definition while generated mesh remains derived state");
    if (usedCadFaceAuthoring) {
        const SupportDefinition *support = services.analysis->support(supportId);
        const LoadDefinition *force = services.analysis->load(forceId);
        check(support != nullptr && force != nullptr
                  && support->scopingMethod == BoundaryScopingMethod::NamedSelection
                  && force->scopingMethod == BoundaryScopingMethod::NamedSelection
                  && support->namedSelectionId == supportNamedSelectionId
                  && force->namedSelectionId == forceNamedSelectionId
                  && services.namedSelections->byId(supportNamedSelectionId) != nullptr
                  && services.namedSelections->byId(forceNamedSelectionId) != nullptr,
              "save/reopen preserves CAD Face Named Selection reference integrity");
        if (support != nullptr && force != nullptr) {
            check(services.analysis->resolveBoundaryScope(*support).valid
                      && services.analysis->resolveBoundaryScope(*force).valid,
                  "reopened CAD Face relationships resolve against current topology provenance");
        }
    }

    window.selectObject(services.project->meshNode());
    check(window.runCommand(QStringLiteral("mesh.generate")),
          "reopened model regenerates its derived structured HEX8 mesh");
    flushUi();
    window.selectObject(analysisId);
    check(services.analysis->preflight(analysisId).passed(),
          "reopened nonlinear document is Solve-ready after mesh regeneration");
    check(window.runCommand(QStringLiteral("analysis.preflight")),
          "supported nonlinear Preflight runs through the shell command");
    flushUi();
    check(window.runCommand(QStringLiteral("analysis.solve")),
          "Nonlinear Static Solve command enters the general product consumer");
    flushUi();

    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->solved
              && record->solveState == SolveState::Completed
              && !services.analysis->solutionIsOutOfDate(analysisId),
          "general nonlinear product solve reaches a fresh converged result state");
    ObjectId nonlinearDeformationId = InvalidObjectId;
    if (record != nullptr) {
        for (const ObjectId resultId : record->results) {
            if (services.project->typeOf(resultId) == ObjectType::TotalDeformation) {
                nonlinearDeformationId = resultId;
                break;
            }
        }
    }
    check(nonlinearDeformationId != InvalidObjectId
              && window.navigator()->selectedObject() == nonlinearDeformationId
              && window.currentAnalysis() == analysisId,
          "successful nonlinear Solve selects its own Total Deformation result context");

    const SolverConvergenceSnapshot *telemetry =
        services.analysis->solverTelemetry(analysisId);
    check(telemetry != nullptr
              && telemetry->summary.executionMode == SolverExecutionMode::NonlinearNewton
              && telemetry->summary.state == SolverConvergenceState::Converged
              && std::abs(telemetry->summary.completedLoadFactor - 1.0) <= 1.0e-10
              && telemetry->summary.acceptedSteps > 0
              && telemetry->summary.totalIterations > 0
              && telemetry->summary.minimumJacobian.has_value()
              && *telemetry->summary.minimumJacobian > 0.0
              && !telemetry->entries.isEmpty(),
          "typed telemetry reports real converged Newton summary and positive minimum J");

    bool completeHistory = telemetry != nullptr && !telemetry->entries.isEmpty();
    bool finalConvergedRow = false;
    if (telemetry != nullptr) {
        for (const SolverConvergenceEntry &entry : telemetry->entries) {
            completeHistory = completeHistory && entry.attempt > 0
                && entry.acceptedStepBefore.has_value() && entry.iteration >= 0
                && entry.loadIncrement.has_value() && entry.residualNorm.has_value()
                && entry.displacementIncrementNorm.has_value()
                && entry.minimumJacobian.has_value()
                && std::isfinite(entry.loadFactor)
                && std::isfinite(entry.relativeResidual)
                && std::isfinite(entry.relativeDisplacement)
                && std::isfinite(entry.lineSearchAlpha)
                && entry.lineSearchAlpha > 0.0 && entry.lineSearchAlpha <= 1.0
                && entry.pressureResidualNorm == std::nullopt
                && entry.relativePressureResidual == std::nullopt
                && entry.pressureIncrementNorm == std::nullopt
                && entry.activeContactCount == std::nullopt
                && entry.stickContactCount == std::nullopt
                && entry.slipContactCount == std::nullopt
                && entry.maximumPenetration == std::nullopt;
            finalConvergedRow = finalConvergedRow
                || (entry.converged && std::abs(entry.loadFactor - 1.0) <= 1.0e-10);
        }
        completeHistory = completeHistory
            && telemetry->summary.pressureMetrics == SolverMetricAvailability::Unavailable
            && telemetry->summary.contactMetrics == SolverMetricAvailability::Unavailable
            && telemetry->summary.finalPressureResidualNorm == std::nullopt
            && telemetry->summary.finalActiveContactCount == std::nullopt;
    }
    check(completeHistory && finalConvergedRow,
          "Newton history carries real iteration fields while mixed/contact metrics stay Unavailable");

    const femcae::meshing::ResultDatabase *database =
        services.analysis->resultDatabase(analysisId);
    const femcae::meshing::NodeVectorField *displacement =
        database != nullptr ? database->displacement() : nullptr;
    const femcae::meshing::ElementScalarField *stress =
        database != nullptr ? database->elementScalar("von_mises") : nullptr;
    bool finiteDisplacement = displacement != nullptr
        && displacement->values.size() == services.mesh->mesh().nodes.size();
    bool nonzeroDisplacement = false;
    if (displacement != nullptr) {
        for (const auto &[nodeId, value] : displacement->values) {
            (void)nodeId;
            finiteDisplacement = finiteDisplacement && std::isfinite(value.x)
                && std::isfinite(value.y) && std::isfinite(value.z);
            nonzeroDisplacement = nonzeroDisplacement
                || std::sqrt(value.x * value.x + value.y * value.y + value.z * value.z) > 0.0;
        }
    }
    bool finiteStress = stress != nullptr
        && stress->values.size() == services.mesh->mesh().elements.size();
    if (stress != nullptr) {
        for (const auto &[elementId, value] : stress->values) {
            (void)elementId;
            finiteStress = finiteStress && std::isfinite(value) && value >= 0.0;
        }
    }
    check(finiteDisplacement && nonzeroDisplacement && finiteStress,
          "final nonlinear displacement/deformed-shape and Cauchy von Mises fields are finite");

    const LoadDefinition *solvedForce = services.analysis->load(forceId);
    const SolveResults results = record != nullptr ? record->solveResults : SolveResults{};
    const double reactionTolerance = solvedForce != nullptr
        ? 1.0e-6 * std::max(1.0, solvedForce->magnitudeN()) : 0.0;
    check(solvedForce != nullptr && results.valid
              && std::abs(results.reactionXN + solvedForce->fxN) <= reactionTolerance
              && std::abs(results.reactionYN + solvedForce->fyN) <= reactionTolerance
              && std::abs(results.reactionZN + solvedForce->fzN) <= reactionTolerance,
          "recovered reaction resultant balances the applied Total Force");
    check(results.valid && std::isfinite(results.maxDisplacementMm)
              && results.maxDisplacementMm > 0.0
              && std::isfinite(results.minVonMisesMPa)
              && std::isfinite(results.maxVonMisesMPa)
              && results.minVonMisesMPa >= 0.0
              && results.maxVonMisesMPa >= results.minVonMisesMPa
              && results.probeNodeId >= 0 && std::isfinite(results.probeUxMm),
          "final result summary exposes finite min/max and probe data");

    ObjectId nonlinearStressId = InvalidObjectId;
    if (record != nullptr) {
        for (const ObjectId resultId : record->results) {
            if (services.project->typeOf(resultId) == ObjectType::EquivalentStress) {
                nonlinearStressId = resultId;
                break;
            }
        }
    }
    if (nonlinearStressId != InvalidObjectId) {
        window.selectObject(nonlinearStressId);
        flushUi();
    }
    const auto *measure = window.findChild<QLabel *>(QStringLiteral("resultInspector.measure"));
    check(nonlinearStressId != InvalidObjectId && measure != nullptr
              && measure->text().contains(QStringLiteral("Cauchy von Mises"))
              && measure->text().contains(QStringLiteral("8-GP")),
          "Equivalent Stress Details documents the nonlinear Cauchy/8-GP result definition");

    // Derived fields/history are deliberately absent from project JSON. The
    // persistent authoring model can be reopened and solved again.
    check(services.analysis->analysisToJson(analysisId) == authoredAnalysis,
          "nonlinear solve does not mutate persistent authoring JSON");
    check(window.saveProjectToPath(solvedPath),
          "solved nonlinear project saves without embedding derived field arrays");
    window.newProjectWithoutPrompt();
    flushUi();
    check(window.openProjectFromPath(solvedPath),
          "solved project reopens through the canonical persistence path");
    flushUi();
    record = services.analysis->analysis(analysisId);
    check(record != nullptr && !record->solved
              && services.analysis->analysisToJson(analysisId) == authoredAnalysis
              && record->results == authoredResultIds,
          "reopen restores solve-ready authoring identities but not derived nonlinear results");
    window.selectObject(services.project->meshNode());
    check(window.runCommand(QStringLiteral("mesh.generate")),
          "reopened solved project regenerates its derived mesh");
    flushUi();
    check(services.analysis->preflight(analysisId).passed(),
          "reopened authoring model returns to authoritative Solve-ready state");

    check(window.openProjectFromPath(baselinePath),
          "B3.7 restores the exact pre-test persistent document");
    flushUi();
    if (baselineHadMesh) {
        window.selectObject(services.project->meshNode());
        check(window.runCommand(QStringLiteral("mesh.generate")),
              "B3.7 restores the baseline derived mesh availability");
        flushUi();
    }

    std::cout << (failures == 0 ? "B3.7 nonlinear product workflow acceptance PASS"
                                : "B3.7 nonlinear product workflow acceptance FAIL")
              << " checks=" << checks << " failures=" << failures
              << " cad_face_authoring=" << (usedCadFaceAuthoring ? "yes" : "no") << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
