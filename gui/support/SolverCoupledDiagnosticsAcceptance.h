#pragma once

#include "../core/DocumentCommandManager.h"
#include "../services/AnalysisService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <QApplication>
#include <QLabel>
#include <QTableWidget>
#include <QUndoStack>

#include <iostream>

namespace d26 {

inline int runSolverCoupledDiagnosticsAcceptanceTest(QApplication &app,
                                                      Dynamics26MainWindow &window)
{
    int checks = 0;
    int failures = 0;
    const auto check = [&](const bool condition, const char *message) {
        ++checks;
        if (condition) {
            std::cout << "PASS Coupled diagnostics acceptance: " << message << '\n';
        } else {
            ++failures;
            std::cerr << "FAIL Coupled diagnostics acceptance: " << message << '\n';
        }
    };

    UtilityWorkspace *utility = window.utility();
    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    check(utility != nullptr, "Utility Workspace exists");
    check(undoStack != nullptr, "Document Undo stack exists");
    if (utility == nullptr || undoStack == nullptr) {
        return 1;
    }

    auto *table = utility->findChild<QTableWidget *>(
        QStringLiteral("Dynamics26UtilityCoupledDiagnostics"));
    auto *summary = utility->findChild<QLabel *>(
        QStringLiteral("Dynamics26UtilityCoupledDiagnosticsSummary"));
    check(table != nullptr && table->columnCount() == 9,
          "Coupled diagnostics table has stable nine-column contract");
    check(summary != nullptr, "Coupled diagnostics summary has stable objectName");
    if (table == nullptr || summary == nullptr) {
        return 1;
    }

    const int undoBeforeMixed = undoStack->count();
    const int indexBeforeMixed = undoStack->index();
    const bool dirtyBeforeMixed = window.documentCommands()->isDirty();
    check(window.runCommand(QStringLiteral("verify.mixedUp")),
          "Canonical verify.mixedUp executes routed diagnostics consumer");
    app.processEvents();
    check(undoStack->count() == undoBeforeMixed && undoStack->index() == indexBeforeMixed
              && window.documentCommands()->isDirty() == dirtyBeforeMixed,
          "Mixed verification telemetry creates no document mutation");
    check(table->rowCount() > 0,
          "Mixed verification publishes pressure history rows");
    check(summary->text().contains(QStringLiteral("Mixed u-p verification: Available"))
              && summary->text().contains(QStringLiteral("Contact: Unavailable")),
          "Mixed summary exposes pressure availability without fake Contact support");

    bool mixedColumns = table->rowCount() > 0;
    for (int row = 0; row < table->rowCount(); ++row) {
        bool p = false, rp = false, dp = false;
        table->item(row, 2)->text().toDouble(&p);
        table->item(row, 3)->text().toDouble(&rp);
        table->item(row, 4)->text().toDouble(&dp);
        mixedColumns = mixedColumns && p && rp && dp
            && table->item(row, 5)->text() == QStringLiteral("—")
            && table->item(row, 6)->text() == QStringLiteral("—")
            && table->item(row, 7)->text() == QStringLiteral("—")
            && table->item(row, 8)->text() == QStringLiteral("—");
    }
    check(mixedColumns, "Mixed rows contain pressure metrics and unavailable Contact cells");

    const int undoBeforeContact = undoStack->count();
    const int indexBeforeContact = undoStack->index();
    const bool dirtyBeforeContact = window.documentCommands()->isDirty();
    check(window.runCommand(QStringLiteral("verify.contact")),
          "Canonical verify.contact executes routed real verification consumer");
    app.processEvents();
    check(undoStack->count() == undoBeforeContact && undoStack->index() == indexBeforeContact
              && window.documentCommands()->isDirty() == dirtyBeforeContact,
          "Contact verification telemetry creates no document mutation");
    check(table->rowCount() > 0,
          "Contact verification publishes Contact history rows");
    check(summary->text().contains(QStringLiteral("Contact verification: Available"))
              && summary->text().contains(QStringLiteral("Mixed u-p: Unavailable")),
          "Contact summary is verification-scoped and keeps mixed metrics unavailable");

    bool contactColumns = table->rowCount() > 0;
    bool sawActive = false;
    for (int row = 0; row < table->rowCount(); ++row) {
        bool activeOk = false, stickOk = false, slipOk = false, penetrationOk = false;
        const int active = table->item(row, 5)->text().toInt(&activeOk);
        table->item(row, 6)->text().toInt(&stickOk);
        table->item(row, 7)->text().toInt(&slipOk);
        table->item(row, 8)->text().toDouble(&penetrationOk);
        contactColumns = contactColumns
            && table->item(row, 2)->text() == QStringLiteral("—")
            && table->item(row, 3)->text() == QStringLiteral("—")
            && table->item(row, 4)->text() == QStringLiteral("—")
            && activeOk && stickOk && slipOk && penetrationOk;
        sawActive = sawActive || (activeOk && active > 0);
    }
    check(contactColumns && sawActive,
          "Contact rows expose real active/stick/slip/penetration telemetry only");

    const ServiceContext services = window.services();
    const ObjectId analysisId = window.firstObjectOfType(ObjectType::Analysis);
    const SolverConvergenceSnapshot *modelTelemetry =
        services.analysis != nullptr && analysisId != InvalidObjectId
            ? services.analysis->solverTelemetry(analysisId) : nullptr;
    check(modelTelemetry == nullptr
              || (modelTelemetry->summary.pressureMetrics == SolverMetricAvailability::Unavailable
                  && modelTelemetry->summary.contactMetrics == SolverMetricAvailability::Unavailable),
          "Verification telemetry does not promote capability into general model solve");

    std::cout << "Coupled diagnostics acceptance: " << (checks - failures) << '/' << checks
              << (failures == 0 ? " PASS\n" : " FAIL\n");
    return failures == 0 ? 0 : 1;
}

} // namespace d26
