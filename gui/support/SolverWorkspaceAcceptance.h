#pragma once

#include "../core/DocumentCommandManager.h"
#include "../core/SolverTelemetry.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <QApplication>
#include <QLabel>
#include <QTableWidget>
#include <QUndoStack>

#include <iostream>

namespace d26 {

inline int runSolverWorkspaceAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
{
    int checks = 0;
    int failures = 0;
    const auto check = [&](const bool condition, const char *message) {
        ++checks;
        if (condition) {
            std::cout << "PASS Solver workspace acceptance: " << message << '\n';
        } else {
            ++failures;
            std::cerr << "FAIL Solver workspace acceptance: " << message << '\n';
        }
    };

    UtilityWorkspace *utility = window.utility();
    check(utility != nullptr, "Utility Workspace exists");
    if (utility == nullptr) {
        return 1;
    }

    auto *summary = utility->findChild<QLabel *>(QStringLiteral("Dynamics26UtilityConvergenceSummary"));
    auto *table = utility->findChild<QTableWidget *>(QStringLiteral("Dynamics26UtilityConvergence"));
    check(summary != nullptr, "Convergence summary has stable objectName");
    check(table != nullptr, "Convergence table has stable objectName");
    if (summary == nullptr || table == nullptr) {
        return 1;
    }

    const int undoBefore = window.documentCommands()->stack()->count();

    SolverConvergenceSnapshot snapshot;
    snapshot.summary.state = SolverConvergenceState::Converged;
    snapshot.summary.completedLoadFactor = 1.0;
    snapshot.summary.finalResidualNorm = 2.5e-10;
    snapshot.summary.acceptedSteps = 4;
    snapshot.summary.totalIterations = 9;
    snapshot.summary.cutbackCount = 1;
    snapshot.entries = {
        SolverConvergenceEntry{1, 1, 0.25, 1.0, 0.12, 1.0, false},
        SolverConvergenceEntry{1, 2, 0.25, 2.5e-10, 8.0e-9, 0.5, true}
    };

    utility->setConvergenceData(snapshot);
    app.processEvents();

    check(window.documentCommands()->stack()->count() == undoBefore,
          "Typed telemetry update does not create document Undo state");
    check(table->rowCount() == 2, "Typed telemetry renders exact iteration row count");
    check(table->columnCount() == 7, "Convergence table keeps the B2.1 metric column contract");
    check(table->item(0, 0) != nullptr && table->item(0, 0)->text() == QStringLiteral("1"),
          "Attempt identity renders from typed telemetry");
    check(table->item(1, 1) != nullptr && table->item(1, 1)->text() == QStringLiteral("2"),
          "Iteration identity renders from typed telemetry");
    check(table->item(1, 2) != nullptr && table->item(1, 2)->text() == QStringLiteral("0.25"),
          "Load factor renders deterministically");
    check(table->item(1, 5) != nullptr && table->item(1, 5)->text() == QStringLiteral("0.5"),
          "Line-search alpha renders deterministically");
    check(table->item(1, 6) != nullptr && table->item(1, 6)->text() == QStringLiteral("Converged"),
          "Converged row state is explicit");
    check(summary->text().contains(QStringLiteral("Converged"))
              && summary->text().contains(QStringLiteral("Cutback = 1"))
              && summary->text().contains(QStringLiteral("Newton iterasyonu = 9")),
          "Session summary exposes state, iteration and cutback metrics");

    SolverConvergenceSnapshot emptySnapshot;
    utility->setConvergenceData(emptySnapshot);
    app.processEvents();

    check(table->rowCount() == 0, "Empty snapshot clears stale convergence rows");
    check(summary->text().contains(QStringLiteral("verisi yok")),
          "Empty snapshot exposes unavailable telemetry state");
    check(window.documentCommands()->stack()->count() == undoBefore,
          "Empty telemetry reset also leaves document Undo state unchanged");

    std::cout << "Solver workspace acceptance: " << (checks - failures) << '/' << checks
              << (failures == 0 ? " PASS\n" : " FAIL\n");
    return failures == 0 ? 0 : 1;
}

} // namespace d26
