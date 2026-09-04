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
#include <unordered_set>

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

    // Aktif bir tree nesnesinin varlığı tek başına engineering action değildir:
    // Fixed Support en az bir DOF kısıtlamalı, Total Force ise finite ve
    // sıfırdan farklı bir resultant taşımalıdır. Bu geçerlilik aynı typed
    // capability kararı üzerinden hem Preflight'a hem Solve'a ulaşır.
    const SupportDefinition *authoredSupportPointer = services.analysis->support(supportId);
    const LoadDefinition *authoredForcePointer = services.analysis->load(forceId);
    if (authoredSupportPointer != nullptr && authoredForcePointer != nullptr) {
        const SupportDefinition authoredSupport = *authoredSupportPointer;
        const LoadDefinition authoredForce = *authoredForcePointer;

        SupportDefinition emptySupport = authoredSupport;
        emptySupport.fixX = false;
        emptySupport.fixY = false;
        emptySupport.fixZ = false;
        commands->push(new commands::SetSupportCommand(
            services, supportId, authoredSupport, emptySupport));
        flushUi();
        const AnalysisCapabilityResolution emptySupportCapabilities =
            services.analysis->resolveCapabilities(analysisId);
        const CapabilityDecision *emptySupportDecision =
            emptySupportCapabilities.decision(CapabilityAxis::BoundaryCondition);
        const PreflightReport emptySupportPreflight = services.analysis->preflight(analysisId);
        const bool emptySupportNavigatesExactly = std::any_of(
            emptySupportPreflight.checks.cbegin(), emptySupportPreflight.checks.cend(),
            [supportId](const PreflightCheck &item) {
                return item.status == PreflightCheck::Status::Failed
                    && item.subject == supportId;
            });
        check(emptySupportDecision != nullptr
                  && emptySupportDecision->state == CapabilityState::Invalid
                  && emptySupportDecision->subject == supportId
                  && !emptySupportPreflight.passed() && emptySupportNavigatesExactly,
              "Fixed Support with no constrained DOF fails Preflight at its exact ObjectId");
        const QJsonObject emptySupportDocument = services.analysis->analysisToJson(analysisId);
        const int undoBeforeEmptySupportSolve = undo->index();
        check(!services.analysis->solve(analysisId)
                  && services.analysis->analysisToJson(analysisId) == emptySupportDocument
                  && undo->index() == undoBeforeEmptySupportSolve,
              "rejected empty Fixed Support cannot reach snapshot/solver or mutate document Undo state");
        undo->undo();
        flushUi();

        SupportDefinition oneDofSupport = authoredSupport;
        oneDofSupport.fixX = true;
        oneDofSupport.fixY = false;
        oneDofSupport.fixZ = false;
        commands->push(new commands::SetSupportCommand(
            services, supportId, authoredSupport, oneDofSupport));
        flushUi();
        const AnalysisCapabilityResolution oneDofCapabilities =
            services.analysis->resolveCapabilities(analysisId);
        const CapabilityDecision *oneDofDecision =
            oneDofCapabilities.decision(CapabilityAxis::BoundaryCondition);
        const PreflightReport oneDofPreflight = services.analysis->preflight(analysisId);
        const bool oneDofFailsStability = std::any_of(
            oneDofPreflight.checks.cbegin(), oneDofPreflight.checks.cend(),
            [supportId](const PreflightCheck &item) {
                return item.status == PreflightCheck::Status::Failed
                    && item.label == QStringLiteral("Structural Stability")
                    && item.detail.contains(QStringLiteral("Rigid-body restraint rank: 3 / 6"))
                    && item.detail.contains(QStringLiteral("Free rigid-body modes: 3"))
                    && item.subject == supportId;
            });
        check(oneDofDecision != nullptr && oneDofDecision->state == CapabilityState::Ready
                  && !oneDofPreflight.passed() && oneDofFailsStability,
              "directional support remains applicable but component rank blocks unstable solve");
        const QJsonObject unstableDocument = services.analysis->analysisToJson(analysisId);
        const int undoBeforeUnstableSolve = undo->index();
        check(!services.analysis->solve(analysisId)
                  && services.analysis->analysisToJson(analysisId) == unstableDocument
                  && undo->index() == undoBeforeUnstableSolve,
              "component stability failure blocks Solve without changing document or Undo state");
        undo->undo();
        flushUi();
        const PreflightReport fullSupportPreflight = services.analysis->preflight(analysisId);
        const bool fullSupportPassesStability = std::any_of(
            fullSupportPreflight.checks.cbegin(), fullSupportPreflight.checks.cend(),
            [](const PreflightCheck &item) {
                return item.status == PreflightCheck::Status::Passed
                    && item.label == QStringLiteral("Structural Stability")
                    && item.detail.contains(QStringLiteral("Rigid-body restraint rank: 6 / 6"));
            });
        check(fullSupportPreflight.passed() && fullSupportPassesStability,
              "all-DOF Fixed Support restores rank 6/6 structural stability");

        LoadDefinition zeroForce = authoredForce;
        zeroForce.fxN = 0.0;
        zeroForce.fyN = 0.0;
        zeroForce.fzN = 0.0;
        commands->push(new commands::SetForceCommand(
            services, forceId, authoredForce, zeroForce));
        flushUi();
        const AnalysisCapabilityResolution zeroForceCapabilities =
            services.analysis->resolveCapabilities(analysisId);
        const CapabilityDecision *zeroForceDecision =
            zeroForceCapabilities.decision(CapabilityAxis::LoadType);
        const PreflightReport zeroForcePreflight = services.analysis->preflight(analysisId);
        const bool zeroForceNavigatesExactly = std::any_of(
            zeroForcePreflight.checks.cbegin(), zeroForcePreflight.checks.cend(),
            [forceId](const PreflightCheck &item) {
                return item.status == PreflightCheck::Status::Failed
                    && item.subject == forceId;
            });
        check(zeroForceDecision != nullptr
                  && zeroForceDecision->state == CapabilityState::Invalid
                  && zeroForceDecision->subject == forceId
                  && !zeroForcePreflight.passed() && zeroForceNavigatesExactly,
              "zero Total Force fails Preflight at its exact ObjectId");
        const QJsonObject zeroForceDocument = services.analysis->analysisToJson(analysisId);
        const int undoBeforeZeroForceSolve = undo->index();
        check(!services.analysis->solve(analysisId)
                  && services.analysis->analysisToJson(analysisId) == zeroForceDocument
                  && undo->index() == undoBeforeZeroForceSolve,
              "rejected zero Total Force cannot reach snapshot/solver or mutate document Undo state");
        undo->undo();
        flushUi();

        LoadDefinition smallForce = authoredForce;
        smallForce.fxN = 1.0e-40;
        smallForce.fyN = 0.0;
        smallForce.fzN = 0.0;
        commands->push(new commands::SetForceCommand(
            services, forceId, authoredForce, smallForce));
        flushUi();
        const AnalysisCapabilityResolution smallForceCapabilities =
            services.analysis->resolveCapabilities(analysisId);
        const CapabilityDecision *smallForceDecision =
            smallForceCapabilities.decision(CapabilityAxis::LoadType);
        check(smallForceDecision != nullptr
                  && smallForceDecision->state == CapabilityState::Ready
                  && services.analysis->preflight(analysisId).passed(),
              "small nonzero Total Force remains Ready in capability/Preflight");
        undo->undo();
        flushUi();
        const SupportDefinition *restoredSupport = services.analysis->support(supportId);
        const LoadDefinition *restoredForce = services.analysis->load(forceId);
        check(restoredSupport != nullptr && restoredForce != nullptr
                  && restoredSupport->fixX == authoredSupport.fixX
                  && restoredSupport->fixY == authoredSupport.fixY
                  && restoredSupport->fixZ == authoredSupport.fixZ
                  && restoredForce->fxN == authoredForce.fxN
                  && restoredForce->fyN == authoredForce.fyN
                  && restoredForce->fzN == authoredForce.fzN
                  && services.analysis->preflight(analysisId).passed(),
              "Undo restores the authored support/load relationship and Solve-ready state");
    } else {
        check(false, "structural-action validity fixture resolves the authored support and load");
    }

    const AnalysisCapabilityResolution ready =
        services.analysis->resolveCapabilities(analysisId);
    check(ready.solveReady() && services.analysis->preflight(analysisId).passed(),
          "authoritative capability resolution and Preflight pass the supported nonlinear subset");

    // Gercek Newton failure/cutback yolu derived solve state'i degistirebilir,
    // fakat persistent document input'u veya Undo index'ini degistiremez. Tek
    // correction/attempt fixture'i her increment'i reddeder ve mevcut solver'in
    // checkpoint/revert + cutback altyapisini product bridge uzerinden calistirir.
    const LoadDefinition *nominalForcePointer = services.analysis->load(forceId);
    record = services.analysis->analysis(analysisId);
    if (nominalForcePointer != nullptr && record != nullptr) {
        const LoadDefinition nominalForce = *nominalForcePointer;
        LoadDefinition failureForce = nominalForce;
        failureForce.fxN = 1.0e7;
        failureForce.fyN = 0.0;
        failureForce.fzN = 0.0;
        const NonlinearSolverControls nominalControls = record->nonlinearControls;
        NonlinearSolverControls failureControls = nominalControls;
        failureControls.maximumIterations = 1;
        failureControls.adaptiveStepping = true;
        failureControls.initialLoadIncrement = 0.25;
        failureControls.minimumLoadIncrement = 0.01;
        failureControls.maximumLoadIncrement = 0.25;
        commands->push(new commands::SetForceCommand(
            services, forceId, nominalForce, failureForce));
        commands->push(new commands::SetNonlinearSolverControlsCommand(
            services, analysisId, nominalControls, failureControls,
            QStringLiteral("Set nonlinear cutback fixture")));
        flushUi();

        const QJsonObject failureDocument = services.analysis->analysisToJson(analysisId);
        const int undoBeforeFailure = undo->index();
        check(services.analysis->preflight(analysisId).passed()
                  && !services.analysis->solve(analysisId),
              "valid product model reaches deterministic Newton cutback/failure instead of fallback");
        flushUi();
        const SolverConvergenceSnapshot *failedTelemetry =
            services.analysis->solverTelemetry(analysisId);
        bool sawTypedCutback = false;
        bool sawTypedRetry = false;
        if (failedTelemetry != nullptr) {
            for (const SolverConvergenceEntry &entry : failedTelemetry->entries) {
                sawTypedCutback = sawTypedCutback
                    || (entry.adaptiveEvent == SolverAdaptiveEvent::Cutback
                        && (entry.adaptiveReason == SolverAdaptiveReason::NewtonNonconvergence
                            || entry.adaptiveReason == SolverAdaptiveReason::MinimumIncrementLimit));
                sawTypedRetry = sawTypedRetry
                    || (entry.adaptiveEvent == SolverAdaptiveEvent::Retry
                        && entry.adaptiveReason == SolverAdaptiveReason::NewtonNonconvergence);
            }
        }
        check(failedTelemetry != nullptr
                  && failedTelemetry->summary.executionMode == SolverExecutionMode::NonlinearNewton
                  && failedTelemetry->summary.state == SolverConvergenceState::Failed
                  && failedTelemetry->summary.cutbackCount > 0
                  && failedTelemetry->summary.terminationPhase
                      == NonlinearTerminationPhase::LoadStepping
                  && failedTelemetry->summary.terminationReason
                      == NonlinearTerminationReason::MinimumIncrementReached
                  && failedTelemetry->summary.lastAttemptedLoadFactor
                      > failedTelemetry->summary.completedLoadFactor
                  && failedTelemetry->summary.lastLoadIncrement > 0.0
                  && sawTypedCutback
                  && sawTypedRetry
                  && !failedTelemetry->entries.isEmpty(),
              "failed product solve exposes typed minimum-increment termination, "
              "cutback/retry provenance and retained iteration history");
        check(services.analysis->analysisToJson(analysisId) == failureDocument
                  && undo->index() == undoBeforeFailure,
              "Newton failure/cutback leaves persistent document and Undo state unchanged");

        undo->undo();
        undo->undo();
        flushUi();
        const LoadDefinition *restoredForce = services.analysis->load(forceId);
        record = services.analysis->analysis(analysisId);
        check(restoredForce != nullptr && record != nullptr
                  && std::abs(restoredForce->fxN - nominalForce.fxN) <= 1.0e-12
                  && std::abs(restoredForce->fyN - nominalForce.fyN) <= 1.0e-12
                  && std::abs(restoredForce->fzN - nominalForce.fzN) <= 1.0e-12
                  && record->nonlinearControls == nominalControls
                  && services.analysis->preflight(analysisId).passed(),
              "Undo restores the nominal nonlinear controls/load and Solve-ready state");
    } else {
        check(false, "cutback fixture resolves the authored Force and nonlinear Analysis");
    }

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
    const femcae::meshing::NodeVectorField *reaction =
        database != nullptr ? database->reaction() : nullptr;
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
    check(displacement != nullptr
              && displacement->metadata.quantity
                  == femcae::meshing::ResultPhysicalQuantity::Displacement
              && displacement->metadata.association
                  == femcae::meshing::ResultAssociation::Node
              && displacement->metadata.storageUnit
                  == femcae::meshing::ResultUnit::Meter
              && stress != nullptr
              && stress->metadata.measure
                  == femcae::meshing::ResultMeasure::CauchyVonMises
              && stress->metadata.sourceLocation
                  == femcae::meshing::ResultSourceLocation::IntegrationPoints
              && stress->metadata.recovery
                  == femcae::meshing::ResultRecoveryMethod::ArithmeticMean
              && stress->metadata.integrationPointCount == 8
              && reaction != nullptr
              && reaction->metadata.quantity
                  == femcae::meshing::ResultPhysicalQuantity::ReactionForce
              && reaction->metadata.sourceLocation
                  == femcae::meshing::ResultSourceLocation::ConstrainedDegreesOfFreedom,
          "derived fields carry explicit quantity, association, source, recovery and unit metadata");

    const LoadDefinition *solvedForce = services.analysis->load(forceId);
    const SolveResults results = record != nullptr ? record->solveResults : SolveResults{};
    const double reactionTolerance = solvedForce != nullptr
        ? 1.0e-6 * std::max(1.0, solvedForce->magnitudeN()) : 0.0;
    check(solvedForce != nullptr && results.valid
              && std::abs(results.reactionXN + solvedForce->fxN) <= reactionTolerance
              && std::abs(results.reactionYN + solvedForce->fyN) <= reactionTolerance
              && std::abs(results.reactionZN + solvedForce->fzN) <= reactionTolerance,
          "recovered reaction resultant balances the applied Total Force");
    std::vector<femcae::meshing::MeshEntityId> supportNodeIds;
    std::unordered_set<femcae::meshing::MeshEntityId> uniqueSupportNodeIds;
    if (const SupportDefinition *support = services.analysis->support(supportId)) {
        const BoundaryScopeResolution resolved = services.analysis->resolveBoundaryScope(*support);
        if (resolved.valid) {
            for (const auto &facet : services.mesh->mesh().boundaryFacets) {
                if (!resolved.geometryFaceIds.contains(facet.sourceGeometryId)) {
                    continue;
                }
                for (const auto nodeId : facet.nodeIds) {
                    if (uniqueSupportNodeIds.insert(nodeId).second) {
                        supportNodeIds.push_back(nodeId);
                    }
                }
            }
        }
    }
    const auto supportReaction = database != nullptr
        ? database->reactionResultant(services.mesh->mesh(), supportNodeIds)
        : std::nullopt;
    check(supportReaction.has_value()
              && std::abs(supportReaction->value.x - results.reactionXN) <= reactionTolerance
              && std::abs(supportReaction->value.y - results.reactionYN) <= reactionTolerance
              && std::abs(supportReaction->value.z - results.reactionZN) <= reactionTolerance,
          "nodal reaction field sums to the support-scoped equilibrium resultant");
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
    const auto *association = window.findChild<QLabel *>(
        QStringLiteral("resultInspector.association"));
    const auto *sourceLocation = window.findChild<QLabel *>(
        QStringLiteral("resultInspector.sourceLocation"));
    const auto *recovery = window.findChild<QLabel *>(
        QStringLiteral("resultInspector.recoveryMethod"));
    check(nonlinearStressId != InvalidObjectId && measure != nullptr
              && measure->text().contains(QStringLiteral("Cauchy von Mises"))
              && association != nullptr && association->text() == QStringLiteral("Element")
              && sourceLocation != nullptr
              && sourceLocation->text().contains(QStringLiteral("8 Integration Points"))
              && recovery != nullptr && recovery->text() == QStringLiteral("Arithmetic Mean"),
          "Equivalent Stress Details exposes Cauchy, element, 8-GP and recovery semantics separately");

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
