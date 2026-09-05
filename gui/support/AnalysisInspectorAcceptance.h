#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.4 + Beta.2 B2.3 — Analysis Inspector acceptance.
//
// Bu test fiziksel pointer UX testi değildir. Gerçek MainWindow composition
// üzerinde AnalysisDetails -> canonical command/service -> Undo/dependency ->
// Inspector refresh zincirini doğrular. Preflight için tek doğruluk kaynağı
// AnalysisService::preflight() olarak kalır; Inspector ikinci readiness state'i
// üretmez. B2.3 bölümü nonlinear controls authoring, persistence ve validation
// contract'ını doğrular; general nonlinear model consumer desteği iddia etmez.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../details/AnalysisDetails.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../shell/CommandRegistry.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QEventLoop>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QSpinBox>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QUndoStack>

#include <cmath>
#include <iostream>
#include <string>

namespace d26 {

inline int runAnalysisInspectorAcceptanceTest(QApplication &app,
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
    const auto settle = [&app](const int milliseconds) {
        QEventLoop loop;
        QTimer::singleShot(milliseconds, &loop, &QEventLoop::quit);
        loop.exec();
        app.processEvents(QEventLoop::AllEvents, 60);
    };
    const auto near = [](const double actual, const double expected) {
        const double scale = std::max(1.0, std::max(std::abs(actual), std::abs(expected)));
        return std::abs(actual - expected) <= 1.0e-10 * scale;
    };

    const ServiceContext services = window.services();
    QUndoStack *stack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    AnalysisDetails *details = window.findChild<AnalysisDetails *>();
    DetailsHost *detailsHost = window.detailsHost();
    check(services.project != nullptr && services.mesh != nullptr
              && services.materials != nullptr && services.analysis != nullptr
              && stack != nullptr && details != nullptr && detailsHost != nullptr,
          "Analysis Inspector acceptance has authoritative Project/Mesh/Material/Analysis/Undo composition");
    if (services.project == nullptr || services.mesh == nullptr
        || services.materials == nullptr || services.analysis == nullptr
        || stack == nullptr || details == nullptr || detailsHost == nullptr) {
        return 1;
    }

    window.newProjectWithoutPrompt();
    window.documentCommands()->resetHistory();
    const ObjectId analysisId = window.firstObjectOfType(ObjectType::Analysis);
    const AnalysisRecord *record = services.analysis->analysis(analysisId);
    check(analysisId != InvalidObjectId && record != nullptr,
          "Analysis Inspector fixture resolves the default real Analysis object");
    if (analysisId == InvalidObjectId || record == nullptr) {
        return 1;
    }

    window.selectObject(analysisId);
    flushUi();

    auto *name = details->findChild<QLineEdit *>(QStringLiteral("analysisInspector.name"));
    auto *procedure = details->findChild<QLabel *>(QStringLiteral("analysisInspector.procedure"));
    auto *largeDeflection = details->findChild<QComboBox *>(QStringLiteral("analysisInspector.largeDeflection"));
    auto *incompressibility = details->findChild<QComboBox *>(QStringLiteral("analysisInspector.incompressibility"));
    auto *nonlinearConsumer = details->findChild<QLabel *>(QStringLiteral("analysisInspector.nonlinearConsumer"));
    auto *nonlinearMethod = details->findChild<QComboBox *>(QStringLiteral("analysisInspector.nonlinearMethod"));
    auto *maximumIterations = details->findChild<QSpinBox *>(QStringLiteral("analysisInspector.maximumIterations"));
    auto *adaptiveStepping = details->findChild<QComboBox *>(QStringLiteral("analysisInspector.adaptiveStepping"));
    auto *lineSearch = details->findChild<QComboBox *>(QStringLiteral("analysisInspector.lineSearch"));
    auto *initialIncrement = details->findChild<QDoubleSpinBox *>(QStringLiteral("analysisInspector.initialIncrement"));
    auto *minimumIncrement = details->findChild<QDoubleSpinBox *>(QStringLiteral("analysisInspector.minimumIncrement"));
    auto *maximumIncrement = details->findChild<QDoubleSpinBox *>(QStringLiteral("analysisInspector.maximumIncrement"));
    auto *residualTolerance = details->findChild<QDoubleSpinBox *>(QStringLiteral("analysisInspector.residualTolerance"));
    auto *displacementTolerance = details->findChild<QDoubleSpinBox *>(QStringLiteral("analysisInspector.displacementTolerance"));
    auto *activeSupports = details->findChild<QLabel *>(QStringLiteral("analysisInspector.activeSupports"));
    auto *activeLoads = details->findChild<QLabel *>(QStringLiteral("analysisInspector.activeLoads"));
    auto *meshReadiness = details->findChild<QLabel *>(QStringLiteral("analysisInspector.meshReadiness"));
    auto *materialReadiness = details->findChild<QLabel *>(QStringLiteral("analysisInspector.materialReadiness"));
    auto *state = details->findChild<QLabel *>(QStringLiteral("analysisInspector.state"));
    auto *results = details->findChild<QLabel *>(QStringLiteral("analysisInspector.results"));
    auto *preflight = details->findChild<QPushButton *>(QStringLiteral("analysisInspector.preflight"));
    auto *solve = details->findChild<QPushButton *>(QStringLiteral("analysisInspector.solve"));

    check(name != nullptr && procedure != nullptr && largeDeflection != nullptr
              && incompressibility != nullptr && nonlinearConsumer != nullptr
              && nonlinearMethod != nullptr && maximumIterations != nullptr
              && adaptiveStepping != nullptr && lineSearch != nullptr
              && initialIncrement != nullptr && minimumIncrement != nullptr
              && maximumIncrement != nullptr && residualTolerance != nullptr
              && displacementTolerance != nullptr && activeSupports != nullptr
              && activeLoads != nullptr && meshReadiness != nullptr
              && materialReadiness != nullptr && state != nullptr && results != nullptr
              && preflight != nullptr && solve != nullptr,
          "Analysis Inspector exposes stable definition/solver-control/readiness/lifecycle/action bindings");
    if (name == nullptr || procedure == nullptr || largeDeflection == nullptr
        || incompressibility == nullptr || nonlinearConsumer == nullptr
        || nonlinearMethod == nullptr || maximumIterations == nullptr
        || adaptiveStepping == nullptr || lineSearch == nullptr
        || initialIncrement == nullptr || minimumIncrement == nullptr
        || maximumIncrement == nullptr || residualTolerance == nullptr
        || displacementTolerance == nullptr || activeSupports == nullptr
        || activeLoads == nullptr || meshReadiness == nullptr
        || materialReadiness == nullptr || state == nullptr || results == nullptr
        || preflight == nullptr || solve == nullptr) {
        return failures + 1;
    }

    const ProjectObject *analysisNode = services.project->object(analysisId);
    check(analysisNode != nullptr && name->text() == analysisNode->name
              && procedure->text() == displayName(record->type)
              && activeSupports->text().contains(QString::number(record->supports.size()))
              && activeLoads->text().contains(QString::number(record->loads.size()))
              && meshReadiness->text().contains(QStringLiteral("Not generated"), Qt::CaseInsensitive)
              && materialReadiness->text().contains(QStringLiteral("Ready"), Qt::CaseInsensitive),
          "fresh Analysis Inspector reads identity/procedure/default consumers/readiness from authoritative services");
    check(!nonlinearMethod->isEnabled() && !maximumIterations->isEnabled()
              && nonlinearConsumer->text().contains(QStringLiteral("Inactive"), Qt::CaseInsensitive)
              && nonlinearMethod->currentIndex() == 0 && maximumIterations->value() == 25
              && adaptiveStepping->currentIndex() == 1 && lineSearch->currentIndex() == 1,
          "linear Static Structural Inspector shows nonlinear defaults but keeps inactive authoring controls disabled");
    check(window.commandRegistry()->action(QStringLiteral("analysis.cancel")) == nullptr
              && state->toolTip().contains(QStringLiteral("synchronous"), Qt::CaseInsensitive)
              && state->toolTip().contains(QStringLiteral("unavailable"), Qt::CaseInsensitive),
          "RC.1 exposes synchronous solve truth and does not publish a fake Cancel command");

    // Inspector Run Preflight yalnız mevcut shell command registry yolunu ister.
    // Mesh henüz yokken blocking rapor beklenir; bu salt diagnostics işlemi Undo
    // geçmişini değiştirmemelidir.
    const int preflightUndoIndex = stack->index();
    const PreflightReport initialReport = services.analysis->preflight(analysisId);
    preflight->click();
    flushUi();
    auto *utilityTabs = window.findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs"));
    auto *preflightTable = window.findChild<QTableWidget *>(QStringLiteral("Dynamics26UtilityPreflight"));
    check(stack->index() == preflightUndoIndex
              && utilityTabs != nullptr && preflightTable != nullptr
              && utilityTabs->currentIndex() == static_cast<int>(UtilityWorkspace::Tab::Preflight)
              && preflightTable->rowCount() == initialReport.checks.size(),
          "Analysis Inspector Run Preflight uses canonical structured diagnostics without document mutation");

    // Default model gerçek support/load/material tanımlarına sahiptir. Mesh'i
    // canonical application command ile üretince authoritative Preflight Ready
    // olmalı ve Inspector bunu yalnız sunmalıdır.
    check(window.runCommand(QStringLiteral("mesh.generate")),
          "Analysis Inspector fixture generates FEM mesh through canonical application command");
    settle(250);
    window.selectObject(analysisId);
    flushUi();
    check(services.mesh->hasMesh() && services.mesh->isUpToDate()
              && services.analysis->preflight(analysisId).passed()
              && meshReadiness->text().contains(QStringLiteral("Ready"), Qt::CaseInsensitive)
              && solve->isEnabled(),
          "current generated mesh makes authoritative Preflight Ready and enables canonical Solve action");

    const int solveUndoIndex = stack->index();
    solve->click();
    settle(350);
    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->solved
              && services.analysis->hasResults(analysisId)
              && !services.analysis->solutionIsOutOfDate(analysisId)
              && stack->index() == solveUndoIndex,
          "Analysis Inspector Solve creates only derived solution state and no document Undo transaction");

    // Başarılı Solve, normal ürün akışında Total Deformation sonucuna gider.
    // Analysis Inspector property acceptance'ına devam etmek için kullanıcı gibi
    // Analysis nesnesini yeniden seçeriz; hidden page'i doğrudan mutate etmeyiz.
    window.selectObject(analysisId);
    flushUi();
    check(detailsHost->currentObject() == analysisId && details->objectId() == analysisId,
          "Analysis Inspector is current again before editable definition checks");
    check(state->text().contains(QStringLiteral("Solved"), Qt::CaseInsensitive)
              && !state->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive)
              && results->text().contains(QStringLiteral("Calculated"), Qt::CaseInsensitive)
              && results->text().contains(QString::number(record->results.size())),
          "Analysis Inspector reports current solved lifecycle and result-definition availability");

    // Display name engineering solver input değildir. Boş/no-op editler mutation
    // üretmez; gerçek rename tek canonical RenameObjectCommand transaction'ıdır.
    const QString originalName = services.project->object(analysisId)->name;
    const int noOpIndex = stack->index();
    name->setText(originalName);
    QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
    flushUi();
    name->setText(QStringLiteral("   "));
    QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
    flushUi();
    check(stack->index() == noOpIndex && name->text() == originalName
              && services.project->object(analysisId)->name == originalName,
          "Analysis name no-op/blank edits create no empty transaction and preserve identity");

    const QString renamed = originalName + QStringLiteral(" Inspector");
    const int renameIndex = stack->index();
    name->setText(renamed);
    QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
    flushUi();
    check(stack->index() == renameIndex + 1
              && services.project->object(analysisId)->name == renamed
              && name->text() == renamed,
          "Analysis name widget creates exactly one canonical rename transaction");
    check(!services.analysis->solutionIsOutOfDate(analysisId),
          "Analysis display-name edit does not invalidate solved engineering input");
    stack->undo();
    flushUi();
    check(services.project->object(analysisId)->name == originalName,
          "Undo restores exact authoritative Analysis name in ProjectModel");
    check(name->text() == originalName,
          "Undo refreshes the visible Analysis name widget from authoritative ProjectModel state");
    check(!services.analysis->solutionIsOutOfDate(analysisId),
          "Undo Analysis display-name edit preserves current solved engineering signature");

    // Large Deflection solver input'tur. Widget canonical command ile tek Undo
    // transaction üretmeli; solved signature değiştiğinde lifecycle Out-of-Date
    // görünmeli ve exact Undo ile tekrar current olmalıdır.
    record = services.analysis->analysis(analysisId);
    const bool originalLargeDeflection = record != nullptr && record->largeDeflection;
    const int beforeSolverEdit = stack->index();
    largeDeflection->setCurrentIndex(originalLargeDeflection ? 0 : 1);
    flushUi();
    check(stack->index() == beforeSolverEdit + 1
              && services.analysis->analysis(analysisId)->largeDeflection != originalLargeDeflection
              && services.analysis->solutionIsOutOfDate(analysisId),
          "Large Deflection widget creates one solver-input transaction and invalidates solved signature");
    check(state->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive)
              && results->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive),
          "Analysis Inspector presents solution and result lifecycle as Out of Date after input mutation");

    stack->undo();
    flushUi();
    check(services.analysis->analysis(analysisId)->largeDeflection == originalLargeDeflection
              && !services.analysis->solutionIsOutOfDate(analysisId)
              && state->text().contains(QStringLiteral("Solved"), Qt::CaseInsensitive)
              && !state->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive),
          "Undo exact solver input restores current solution lifecycle and Inspector state");

    // ------------------------------------------------------------------
    // B2.3 — Solver Controls Authoring
    // ------------------------------------------------------------------
    check(static_cast<int>(NonlinearMethodIntent::FullNewton) == 1
              && static_cast<int>(NonlinearMethodIntent::ModifiedNewton) == 2,
          "Nonlinear method intent IDs match authoritative Fortran solver constants 1/2");

    const ObjectId nonlinearId = services.analysis->createAnalysis(AnalysisType::NonlinearStatic);
    check(nonlinearId != InvalidObjectId,
          "B2.3 fixture creates a real Nonlinear Static analysis object");
    window.documentCommands()->resetHistory();
    window.selectObject(nonlinearId);
    flushUi();
    record = services.analysis->analysis(nonlinearId);
    check(record != nullptr && nonlinearMethod->isEnabled() && maximumIterations->isEnabled()
              && adaptiveStepping->isEnabled() && lineSearch->isEnabled()
              && initialIncrement->isEnabled() && minimumIncrement->isEnabled()
              && maximumIncrement->isEnabled() && residualTolerance->isEnabled()
              && displacementTolerance->isEnabled()
              && nonlinearConsumer->text().contains(QStringLiteral("Ready"), Qt::CaseInsensitive)
              && nonlinearConsumer->text().contains(QStringLiteral("synchronous"), Qt::CaseInsensitive)
              && nonlinearConsumer->toolTip().contains(QStringLiteral("cancellation"), Qt::CaseInsensitive),
          "Nonlinear Static Inspector exposes ready synchronous consumer and explicit cancellation limitation");
    if (record == nullptr) {
        return failures + 1;
    }

    check(record->nonlinearControls.method == NonlinearMethodIntent::FullNewton
              && record->nonlinearControls.maximumIterations == 25
              && record->nonlinearControls.adaptiveStepping
              && near(record->nonlinearControls.initialLoadIncrement, 0.25)
              && near(record->nonlinearControls.minimumLoadIncrement, 1.0e-4)
              && near(record->nonlinearControls.maximumLoadIncrement, 0.50)
              && record->nonlinearControls.lineSearch
              && near(record->nonlinearControls.residualRelativeTolerance, 1.0e-8)
              && near(record->nonlinearControls.displacementRelativeTolerance, 1.0e-8),
          "Nonlinear Static starts from core-aligned persistent control defaults");

    const int methodUndoIndex = stack->index();
    nonlinearMethod->setCurrentIndex(1);
    flushUi();
    record = services.analysis->analysis(nonlinearId);
    check(record != nullptr && stack->index() == methodUndoIndex + 1
              && record->nonlinearControls.method == NonlinearMethodIntent::ModifiedNewton,
          "Newton Method widget creates exactly one canonical document transaction");
    stack->undo();
    flushUi();
    record = services.analysis->analysis(nonlinearId);
    check(record != nullptr && record->nonlinearControls.method == NonlinearMethodIntent::FullNewton
              && nonlinearMethod->currentIndex() == 0,
          "Undo restores authoritative Newton method intent and Inspector binding");

    // Custom state bütün B2.3 alanlarını gerçek widget command yollarından geçirir.
    window.documentCommands()->resetHistory();
    nonlinearMethod->setCurrentIndex(1);
    maximumIterations->setValue(37);
    QMetaObject::invokeMethod(maximumIterations, "editingFinished", Qt::DirectConnection);
    adaptiveStepping->setCurrentIndex(0);
    lineSearch->setCurrentIndex(0);
    initialIncrement->setValue(0.20);
    QMetaObject::invokeMethod(initialIncrement, "editingFinished", Qt::DirectConnection);
    minimumIncrement->setValue(0.01);
    QMetaObject::invokeMethod(minimumIncrement, "editingFinished", Qt::DirectConnection);
    maximumIncrement->setValue(0.40);
    QMetaObject::invokeMethod(maximumIncrement, "editingFinished", Qt::DirectConnection);
    residualTolerance->setValue(1.0e-7);
    QMetaObject::invokeMethod(residualTolerance, "editingFinished", Qt::DirectConnection);
    displacementTolerance->setValue(1.0e-6);
    QMetaObject::invokeMethod(displacementTolerance, "editingFinished", Qt::DirectConnection);
    flushUi();

    record = services.analysis->analysis(nonlinearId);
    NonlinearSolverControls authored;
    authored.method = NonlinearMethodIntent::ModifiedNewton;
    authored.maximumIterations = 37;
    authored.adaptiveStepping = false;
    authored.initialLoadIncrement = 0.20;
    authored.minimumLoadIncrement = 0.01;
    authored.maximumLoadIncrement = 0.40;
    authored.lineSearch = false;
    authored.residualRelativeTolerance = 1.0e-7;
    authored.displacementRelativeTolerance = 1.0e-6;
    check(record != nullptr && record->nonlinearControls == authored && stack->count() == 9,
          "Basic and Advanced nonlinear widgets author the exact typed control snapshot through document commands");

    const QJsonObject persisted = services.analysis->analysisToJson(nonlinearId);
    const QJsonObject controlsJson = persisted.value(QStringLiteral("nonlinear_solver_controls")).toObject();
    check(!controlsJson.isEmpty()
              && controlsJson.value(QStringLiteral("method")).toInt() == 2
              && controlsJson.value(QStringLiteral("maximum_iterations")).toInt() == 37,
          "Analysis persistence stores typed nonlinear controls with core-aligned method identity");

    const int nonlinearRow = services.analysis->rowOfAnalysis(nonlinearId);
    check(services.analysis->removeAnalysis(nonlinearId),
          "B2.3 persistence fixture removes authored analysis before exact restore");
    const ObjectId restoredId = services.analysis->restoreAnalysis(persisted, nonlinearRow);
    record = services.analysis->analysis(restoredId);
    check(restoredId == nonlinearId && record != nullptr && record->nonlinearControls == authored,
          "Analysis persistence round-trip restores every nonlinear solver control exactly");

    QJsonObject legacyEntry = persisted;
    legacyEntry.remove(QStringLiteral("nonlinear_solver_controls"));
    check(services.analysis->removeAnalysis(restoredId),
          "B2.3 backward-compatibility fixture removes restored analysis");
    const ObjectId legacyRestoredId = services.analysis->restoreAnalysis(legacyEntry, nonlinearRow);
    record = services.analysis->analysis(legacyRestoredId);
    const NonlinearSolverControls defaults;
    check(legacyRestoredId == nonlinearId && record != nullptr && record->nonlinearControls == defaults,
          "Older analysis JSON without nonlinear_solver_controls migrates to safe core-aligned defaults");

    // Preflight validation, malformed persistent authoring state'i solver'a
    // ulaşmadan yakalamalı. Consumer zaten unsupported olduğundan bu kontrol
    // capability iddiası değildir; yalnız authoring contract validation'dır.
    NonlinearSolverControls invalid = defaults;
    invalid.initialLoadIncrement = 0.20;
    invalid.minimumLoadIncrement = 0.30;
    services.analysis->setNonlinearSolverControls(legacyRestoredId, invalid);
    const PreflightReport invalidReport = services.analysis->preflight(legacyRestoredId);
    bool foundControlFailure = false;
    const AnalysisRecord *invalidRecord = services.analysis->analysis(legacyRestoredId);
    for (const PreflightCheck &entry : invalidReport.checks) {
        if (entry.status == PreflightCheck::Status::Failed
            && entry.label == QStringLiteral("Nonlinear Solver Controls")
            && invalidRecord != nullptr && entry.subject == invalidRecord->settingsNode) {
            foundControlFailure = true;
            break;
        }
    }
    check(foundControlFailure,
          "Preflight blocks invalid nonlinear control ranges at the authoritative Analysis Settings subject");

    // Sonraki acceptance paketleri başlangıç varsayımlarını açıkça alsın.
    window.newProjectWithoutPrompt();
    window.documentCommands()->resetHistory();
    std::cout << "Analysis Inspector acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
