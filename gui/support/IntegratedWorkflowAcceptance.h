#pragma once

// Dynamics26 V1.1 — integrated modeling workflow acceptance.
//
// Bu test ayrı bir demo/model state kurmaz. Gerçek Dynamics26MainWindow
// composition'ı ve canonical command/DomainCommand yollarını kullanarak şu
// mühendislik zincirini doğrular:
//
// Geometry -> Material Inspector -> Mesh -> Analysis -> BC/Load -> Preflight
// -> Solve -> Results -> input mutation -> Solution Out of Date -> Undo recovery.
//
// Fiziksel pointer/trackpad UX kabulünün yerine geçmez.

#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../shell/CommandRegistry.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/ProjectNavigator.h"

#include <QAction>
#include <QApplication>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QLabel>
#include <QMetaObject>
#include <QUndoStack>
#include <QWidget>

#include <cmath>
#include <iostream>
#include <string>

namespace d26 {

inline int runIntegratedWorkflowAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
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
    const auto commitSpinEdit = [&flushUi](QDoubleSpinBox *field) {
        if (field == nullptr) {
            return false;
        }
        const bool invoked = QMetaObject::invokeMethod(field, "editingFinished", Qt::DirectConnection);
        flushUi();
        return invoked;
    };

    const ServiceContext services = window.services();
    check(services.project != nullptr && services.mesh != nullptr && services.materials != nullptr
              && services.analysis != nullptr && services.dependencies != nullptr,
          "integrated workflow has Project/Mesh/Material/Analysis/Dependency collaborators");
    if (services.project == nullptr || services.mesh == nullptr || services.materials == nullptr
        || services.analysis == nullptr || services.dependencies == nullptr) {
        return 1;
    }

    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    check(undoStack != nullptr,
          "integrated workflow has document Undo stack");
    if (undoStack == nullptr) {
        return 1;
    }

    // ------------------------------------------------------------------
    // Geometry / Materials / model graph prerequisites
    // ------------------------------------------------------------------
    const ObjectId bodyId = window.firstObjectOfType(ObjectType::Body);
    check(bodyId != InvalidObjectId && services.project->object(bodyId) != nullptr,
          "Geometry stage exposes a real Body project object");
    check(services.project->sectionsNode() != InvalidObjectId
              && services.project->connectionsNode() != InvalidObjectId,
          "Sections and Connections authoring nodes exist in the integrated model graph");

    const ObjectId assignedMaterialId = services.materials->assignedMaterialId();
    const MaterialDefinition *material = services.materials->assigned();
    check(assignedMaterialId != InvalidObjectId && material != nullptr,
          "Material stage has a persistent assigned Material ObjectId");
    check(material != nullptr && material->supportsLinearStaticSolve(),
          "assigned material is compatible with the current Static Structural solver path");

    // ------------------------------------------------------------------
    // Beta.1 B1.1 — real Material Inspector binding + Undo contract
    // ------------------------------------------------------------------
    if (material != nullptr) {
        window.selectObject(assignedMaterialId);
        flushUi();

        auto *materialInspector = window.findChild<QWidget *>(QStringLiteral("Dynamics26MaterialInspector"));
        auto *youngField = window.findChild<QDoubleSpinBox *>(QStringLiteral("Dynamics26MaterialYoungGPa"));
        auto *densityField = window.findChild<QDoubleSpinBox *>(QStringLiteral("Dynamics26MaterialDensity"));
        auto *identityLabel = window.findChild<QLabel *>(QStringLiteral("Dynamics26MaterialObjectId"));
        auto *revisionLabel = window.findChild<QLabel *>(QStringLiteral("Dynamics26MaterialRevision"));
        auto *compatibilityLabel =
            window.findChild<QLabel *>(QStringLiteral("Dynamics26MaterialSolveCompatibility"));

        check(materialInspector != nullptr && youngField != nullptr && densityField != nullptr,
              "Material Inspector exposes stable engineering edit controls");
        check(identityLabel != nullptr && identityLabel->text() == QString::number(assignedMaterialId),
              "Material Inspector displays the real persistent Material ObjectId");
        check(revisionLabel != nullptr && revisionLabel->text() == QString::number(services.materials->revision()),
              "Material Inspector displays the authoritative MaterialService revision");
        check(compatibilityLabel != nullptr && compatibilityLabel->text().contains(QStringLiteral("Ready")),
              "Material Inspector exposes current Static Structural compatibility");
        check(youngField != nullptr
                  && qFuzzyCompare(youngField->value() + 1.0, material->youngGPa + 1.0),
              "Material Inspector Young modulus field is bound from MaterialService state");
        check(densityField != nullptr
                  && qFuzzyCompare(densityField->value() + 1.0, material->densityKgM3 + 1.0),
              "Material Inspector density field is bound from MaterialService state");

        if (youngField != nullptr && densityField != nullptr) {
            const double originalYoung = material->youngGPa;
            const double originalDensity = material->densityKgM3;
            const int undoBeforeNoOp = undoStack->index();
            check(commitSpinEdit(youngField),
                  "Material Inspector edit commit signal is invokable in application acceptance");
            check(undoStack->index() == undoBeforeNoOp,
                  "committing an unchanged Material field does not create a fake Undo transaction");

            const double changedYoung = originalYoung + 1.0;
            youngField->setValue(changedYoung);
            flushUi();
            const MaterialDefinition *beforeCommit = services.materials->byId(assignedMaterialId);
            check(beforeCommit != nullptr
                      && qFuzzyCompare(beforeCommit->youngGPa + 1.0, originalYoung + 1.0),
                  "Material spinbox typing does not mutate engineering state before edit commit");

            const int undoBeforeYoung = undoStack->index();
            check(commitSpinEdit(youngField),
                  "Young modulus edit commits through the Inspector binding");
            const MaterialDefinition *afterYoung = services.materials->byId(assignedMaterialId);
            check(afterYoung != nullptr
                      && qFuzzyCompare(afterYoung->youngGPa + 1.0, changedYoung + 1.0),
                  "Young modulus edit reaches authoritative MaterialService state");
            check(undoStack->index() == undoBeforeYoung + 1,
                  "one Material field edit creates exactly one document Undo transaction");

            const double changedDensity = originalDensity + 10.0;
            densityField->setValue(changedDensity);
            flushUi();
            const int undoBeforeDensity = undoStack->index();
            check(commitSpinEdit(densityField),
                  "density edit commits through the Inspector binding");
            const MaterialDefinition *afterDensity = services.materials->byId(assignedMaterialId);
            check(afterDensity != nullptr
                      && qFuzzyCompare(afterDensity->densityKgM3 + 1.0, changedDensity + 1.0),
                  "density edit reaches authoritative MaterialService state");
            check(undoStack->index() == undoBeforeDensity + 1,
                  "different Material fields remain distinct Undo transactions");

            undoStack->undo();
            flushUi();
            const MaterialDefinition *densityRestored = services.materials->byId(assignedMaterialId);
            check(densityRestored != nullptr
                      && qFuzzyCompare(densityRestored->densityKgM3 + 1.0, originalDensity + 1.0),
                  "Undo restores the exact previous density value");

            undoStack->undo();
            flushUi();
            const MaterialDefinition *youngRestored = services.materials->byId(assignedMaterialId);
            check(youngRestored != nullptr
                      && qFuzzyCompare(youngRestored->youngGPa + 1.0, originalYoung + 1.0),
                  "Undo restores the exact previous Young modulus value");
            check(undoStack->index() == undoBeforeYoung,
                  "Material Inspector acceptance returns document history to its original index");
        }
    }

    const QVector<ObjectId> analyses = services.project->analyses();
    check(!analyses.isEmpty(),
          "Analysis stage exposes a persistent analysis object");
    if (analyses.isEmpty()) {
        return 1;
    }
    const ObjectId analysisId = analyses.first();

    // Başlangıç testleri önce çözüm üretmiş olabilir. Integrated workflow kendi
    // Solve adımını yeniden kanıtlamak için derived result state'i temizler.
    if (services.analysis->hasResults(analysisId)) {
        window.selectObject(analysisId);
        flushUi();
        check(window.runCommand(QStringLiteral("analysis.clearSolution")),
              "integrated workflow clears previous derived solution through canonical command");
        flushUi();
    }

    // ------------------------------------------------------------------
    // Mesh
    // ------------------------------------------------------------------
    if (services.mesh->hasMesh()) {
        window.selectObject(services.project->meshNode());
        flushUi();
        check(window.runCommand(QStringLiteral("mesh.clearGenerated")),
              "integrated workflow clears previous generated mesh through canonical command");
        flushUi();
    }
    check(!services.mesh->hasMesh(),
          "Mesh stage starts from an explicit ungenerated state");

    window.selectObject(services.project->meshNode());
    flushUi();
    check(window.runCommand(QStringLiteral("mesh.generate")),
          "Generate Mesh command executes from the real Mesh context");
    flushUi();
    check(services.mesh->hasMesh() && !services.mesh->isOutOfDate(),
          "Mesh stage produces a current FEM mesh");
    check(services.mesh->nodeCount() > 0 && services.mesh->elementCount() > 0,
          "generated FEM mesh has real Node and Element entities");

    // ------------------------------------------------------------------
    // Analysis / BC / Load / Preflight
    // ------------------------------------------------------------------
    const AnalysisRecord *record = services.analysis->analysis(analysisId);
    check(record != nullptr && !record->supports.isEmpty() && !record->loads.isEmpty(),
          "Analysis contains Fixed Support and Force consumers");
    if (record == nullptr || record->supports.isEmpty() || record->loads.isEmpty()) {
        return 1;
    }

    int activeSupports = 0;
    for (const ObjectId id : record->supports) {
        if (!services.project->isEffectivelySuppressed(id)) {
            ++activeSupports;
        }
    }
    int activeLoads = 0;
    for (const ObjectId id : record->loads) {
        if (!services.project->isEffectivelySuppressed(id)) {
            ++activeLoads;
        }
    }
    check(activeSupports > 0 && activeLoads > 0,
          "BC/Load stage has at least one active support and one active load");

    window.selectObject(analysisId);
    flushUi();
    check(services.analysis->preflight(analysisId).passed(),
          "authoritative Preflight passes the integrated model before Solve");
    check(window.runCommand(QStringLiteral("analysis.preflight")),
          "Preflight command executes through the real shell command registry");
    flushUi();
    check(services.analysis->preflight(analysisId).passed(),
          "Preflight remains Ready to Solve after structured diagnostics presentation");

    // SolveState::Idle / Completed gibi durumlar geçerli ObjectId ile signal
    // taşır. UI bu ObjectId'yi bool sanmamalı; Ready modelde Solve komutu açık
    // kalmalıdır. Bu assertion lifecycle signal imzasının yanlış bağlanmasına
    // karşı doğrudan regresyon kapısıdır.
    QAction *solveAction = window.commandRegistry() != nullptr
        ? window.commandRegistry()->action(QStringLiteral("analysis.solve")) : nullptr;
    check(solveAction != nullptr && solveAction->isEnabled(),
          "Idle/Ready analysis state keeps Solve command enabled after passing Preflight");

    // ------------------------------------------------------------------
    // Solve / Results
    // ------------------------------------------------------------------
    window.selectObject(analysisId);
    flushUi();
    check(window.runCommand(QStringLiteral("analysis.solve")),
          "Solve command executes only after passing Preflight");
    flushUi();

    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->solved && services.analysis->hasResults(analysisId),
          "Solve produces persistent analysis result state");
    if (record == nullptr || !record->solved) {
        return 1;
    }
    check(!services.analysis->solutionIsOutOfDate(analysisId),
          "freshly solved integrated model is not Out of Date");
    check(record->solveResults.nodeCount > 0 && record->solveResults.elementCount > 0
              && record->solveResults.dofCount > 0,
          "solver result metadata reports real mesh and DOF counts");
    check(solveAction != nullptr && solveAction->isEnabled(),
          "Completed analysis state releases Solve command for a valid re-solve");

    const ObjectId deformationId = window.firstObjectOfType(ObjectType::TotalDeformation);
    check(deformationId != InvalidObjectId,
          "Results stage exposes Total Deformation result object");
    check(deformationId == InvalidObjectId || window.navigator()->selectedObject() == deformationId,
          "successful Solve moves canonical Navigator context to Total Deformation");

    // ------------------------------------------------------------------
    // Input mutation -> Out of Date -> Undo recovery
    // ------------------------------------------------------------------
    record = services.analysis->analysis(analysisId);
    const ObjectId loadId = record != nullptr && !record->loads.isEmpty()
        ? record->loads.first() : InvalidObjectId;
    const LoadDefinition *currentLoad = loadId != InvalidObjectId
        ? services.analysis->load(loadId) : nullptr;
    check(currentLoad != nullptr,
          "Out-of-Date fixture resolves a real Force definition");
    if (currentLoad != nullptr) {
        const LoadDefinition originalLoad = *currentLoad;
        LoadDefinition changedLoad = originalLoad;
        // Her zaman solver imzasını değiştiren fakat modelin fiziksel olarak
        // geçerliliğini bozmayan küçük, deterministik bir yük değişikliği.
        changedLoad.fxN = originalLoad.fxN + (std::abs(originalLoad.fxN) < 1.0 ? 125.0 : 25.0);

        const int undoBeforeMutation = undoStack->index();
        window.documentCommands()->push(
            new commands::SetForceCommand(services, loadId, originalLoad, changedLoad));
        flushUi();

        check(undoStack->index() == undoBeforeMutation + 1,
              "Force input mutation is exactly one document Undo transaction");
        check(services.analysis->solutionIsOutOfDate(analysisId),
              "changing a solver input marks the existing Solution Out of Date");
        check(services.analysis->hasResults(analysisId),
              "Out-of-Date transition preserves prior result data for comparison/recovery");
        check(services.analysis->preflight(analysisId).passed(),
              "changed Force remains a valid model even though prior Solution is stale");

        undoStack->undo();
        flushUi();
        const LoadDefinition *restoredLoad = services.analysis->load(loadId);
        check(undoStack->index() == undoBeforeMutation,
              "Undo returns to the pre-mutation document index");
        check(restoredLoad != nullptr
                  && qFuzzyCompare(restoredLoad->fxN + 1.0, originalLoad.fxN + 1.0),
              "Undo restores the exact Force engineering input");
        check(!services.analysis->solutionIsOutOfDate(analysisId),
              "Undo restoring the identical solver signature re-validates existing Solution");
        check(services.analysis->hasResults(analysisId),
              "re-validated Solution keeps the solved result database available");
    }

    // Son durum tekrar aynı authoritative Preflight + Details özetinden Ready
    // olmalıdır. Bu kontrol workflow'un yalnız solve etmekle kalmayıp recovery
    // sonrasında da kullanılabilir kaldığını doğrular.
    window.selectObject(analysisId);
    flushUi();
    check(services.analysis->preflight(analysisId).passed(),
          "integrated workflow ends in a valid Ready-to-Solve model state");
    auto *summary = window.findChild<QLabel *>(QStringLiteral("Dynamics26PreflightSummary"));
    check(summary != nullptr && summary->text() == QStringLiteral("Ready to Solve · engel yok"),
          "Analysis Details summary returns to Ready to Solve after Undo recovery");

    std::cout << (failures == 0 ? "Integrated workflow + Material Inspector acceptance PASS"
                                : "Integrated workflow + Material Inspector acceptance FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
