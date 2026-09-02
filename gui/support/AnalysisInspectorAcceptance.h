#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.4 — Analysis Inspector application acceptance.
//
// Bu test fiziksel pointer UX testi değildir. Gerçek MainWindow composition
// üzerinde AnalysisDetails -> canonical command/service -> Undo/dependency ->
// Inspector refresh zincirini doğrular. Preflight için tek doğruluk kaynağı
// AnalysisService::preflight() olarak kalır; Inspector ikinci readiness state'i
// üretmez.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../details/AnalysisDetails.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QEventLoop>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QUndoStack>

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

    const ServiceContext services = window.services();
    QUndoStack *stack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    AnalysisDetails *details = window.findChild<AnalysisDetails *>();
    check(services.project != nullptr && services.mesh != nullptr
              && services.materials != nullptr && services.analysis != nullptr
              && stack != nullptr && details != nullptr,
          "Analysis Inspector acceptance has authoritative Project/Mesh/Material/Analysis/Undo composition");
    if (services.project == nullptr || services.mesh == nullptr
        || services.materials == nullptr || services.analysis == nullptr
        || stack == nullptr || details == nullptr) {
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
    auto *activeSupports = details->findChild<QLabel *>(QStringLiteral("analysisInspector.activeSupports"));
    auto *activeLoads = details->findChild<QLabel *>(QStringLiteral("analysisInspector.activeLoads"));
    auto *meshReadiness = details->findChild<QLabel *>(QStringLiteral("analysisInspector.meshReadiness"));
    auto *materialReadiness = details->findChild<QLabel *>(QStringLiteral("analysisInspector.materialReadiness"));
    auto *state = details->findChild<QLabel *>(QStringLiteral("analysisInspector.state"));
    auto *results = details->findChild<QLabel *>(QStringLiteral("analysisInspector.results"));
    auto *preflight = details->findChild<QPushButton *>(QStringLiteral("analysisInspector.preflight"));
    auto *solve = details->findChild<QPushButton *>(QStringLiteral("analysisInspector.solve"));

    check(name != nullptr && procedure != nullptr && largeDeflection != nullptr
              && incompressibility != nullptr && activeSupports != nullptr
              && activeLoads != nullptr && meshReadiness != nullptr
              && materialReadiness != nullptr && state != nullptr && results != nullptr
              && preflight != nullptr && solve != nullptr,
          "Analysis Inspector exposes stable definition/readiness/lifecycle/action bindings");
    if (name == nullptr || procedure == nullptr || largeDeflection == nullptr
        || incompressibility == nullptr || activeSupports == nullptr
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
    details->refresh();
    check(services.mesh->hasMesh() && services.mesh->isUpToDate()
              && services.analysis->preflight(analysisId).passed()
              && meshReadiness->text().contains(QStringLiteral("Ready"), Qt::CaseInsensitive)
              && solve->isEnabled(),
          "current generated mesh makes authoritative Preflight Ready and enables canonical Solve action");

    const int solveUndoIndex = stack->index();
    solve->click();
    settle(350);
    details->refresh();
    record = services.analysis->analysis(analysisId);
    check(record != nullptr && record->solved
              && services.analysis->hasResults(analysisId)
              && !services.analysis->solutionIsOutOfDate(analysisId)
              && stack->index() == solveUndoIndex,
          "Analysis Inspector Solve creates only derived solution state and no document Undo transaction");
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
    check(details->objectId() == analysisId,
          "Undo keeps AnalysisDetails bound to the same Analysis ObjectId");
    check(name->text() == originalName,
          "Undo automatically refreshes the Analysis name widget from authoritative ProjectModel state");

    // Diagnostic split: explicit refresh must be sufficient if delivery/timing is
    // the only remaining fault. This does not replace the automatic-refresh
    // acceptance above; a failure there still keeps B1.4 red.
    details->refresh();
    check(name->text() == originalName,
          "Explicit AnalysisDetails refresh restores authoritative Analysis name");
    flushUi();
    check(name->text() == originalName,
          "Authoritative Analysis name remains stable after event-loop drain");
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
    details->refresh();
    check(stack->index() == beforeSolverEdit + 1
              && services.analysis->analysis(analysisId)->largeDeflection != originalLargeDeflection
              && services.analysis->solutionIsOutOfDate(analysisId),
          "Large Deflection widget creates one solver-input transaction and invalidates solved signature");
    check(state->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive)
              && results->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive),
          "Analysis Inspector presents solution and result lifecycle as Out of Date after input mutation");

    stack->undo();
    flushUi();
    details->refresh();
    check(services.analysis->analysis(analysisId)->largeDeflection == originalLargeDeflection
              && !services.analysis->solutionIsOutOfDate(analysisId)
              && state->text().contains(QStringLiteral("Solved"), Qt::CaseInsensitive)
              && !state->text().contains(QStringLiteral("Out of Date"), Qt::CaseInsensitive),
          "Undo exact solver input restores current solution lifecycle and Inspector state");

    // Sonraki acceptance paketleri başlangıç varsayımlarını açıkça alsın.
    window.newProjectWithoutPrompt();
    window.documentCommands()->resetHistory();
    std::cout << "Analysis Inspector acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
