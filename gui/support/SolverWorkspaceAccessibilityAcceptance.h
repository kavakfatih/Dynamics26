#pragma once

// Dynamics26 V1.1.0-beta.2 / B2.6 — Solver / Utility Workspace accessibility closeout.
//
// Bu acceptance engineering state üretmez. UtilityWorkspace'un sahibi olduğu
// Convergence yüzeyinin keyboard reachability ve accessibility metadata
// sözleşmesini doğrular. Telemetry/solver capability doğruluğu ayrı B2.1–B2.5
// acceptance testlerinde kalır.

#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <QApplication>
#include <QLabel>
#include <QTabWidget>
#include <QTableWidget>

#include <iostream>

namespace d26 {

inline int runSolverWorkspaceAccessibilityAcceptanceTest(QApplication &app,
                                                          Dynamics26MainWindow &window)
{
    int checks = 0;
    int failures = 0;
    const auto check = [&](const bool condition, const char *message) {
        ++checks;
        if (condition) {
            std::cout << "PASS Solver workspace accessibility: " << message << '\n';
        } else {
            ++failures;
            std::cerr << "FAIL Solver workspace accessibility: " << message << '\n';
        }
    };

    UtilityWorkspace *utility = window.utility();
    check(utility != nullptr, "Utility Workspace exists");
    if (utility == nullptr) {
        return 1;
    }

    auto *tabs = utility->findChild<QTabWidget *>(QStringLiteral("Dynamics26UtilityTabs"));
    auto *summary = utility->findChild<QLabel *>(QStringLiteral("Dynamics26UtilityConvergenceSummary"));
    auto *table = utility->findChild<QTableWidget *>(QStringLiteral("Dynamics26UtilityConvergence"));
    auto *diagnosticsSummary = utility->findChild<QLabel *>(
        QStringLiteral("Dynamics26UtilityConvergenceDiagnosticsSummary"));
    auto *diagnosticsTable = utility->findChild<QTableWidget *>(
        QStringLiteral("Dynamics26UtilityConvergenceDiagnostics"));
    auto *coupledSummary = utility->findChild<QLabel *>(
        QStringLiteral("Dynamics26UtilityCoupledDiagnosticsSummary"));
    auto *coupledTable = utility->findChild<QTableWidget *>(
        QStringLiteral("Dynamics26UtilityCoupledDiagnostics"));

    check(tabs != nullptr, "Utility tab container has stable objectName");
    check(summary != nullptr && table != nullptr,
          "Basic Convergence accessibility surfaces are addressable");
    check(diagnosticsSummary != nullptr && diagnosticsTable != nullptr,
          "Advanced diagnostics accessibility surfaces are addressable");
    check(coupledSummary != nullptr && coupledTable != nullptr,
          "Coupled/Contact diagnostics accessibility surfaces are addressable");
    if (tabs == nullptr || summary == nullptr || table == nullptr
        || diagnosticsSummary == nullptr || diagnosticsTable == nullptr
        || coupledSummary == nullptr || coupledTable == nullptr) {
        return 1;
    }

    check(tabs->focusPolicy() == Qt::StrongFocus
              && !tabs->accessibleName().isEmpty()
              && !tabs->accessibleDescription().isEmpty(),
          "Utility tabs expose keyboard focus and accessibility guidance");

    utility->showTab(UtilityWorkspace::Tab::Convergence);
    app.processEvents();
    check(tabs->currentIndex() == static_cast<int>(UtilityWorkspace::Tab::Convergence),
          "Convergence tab remains programmatically reachable through the canonical Utility API");

    check(table->focusPolicy() == Qt::StrongFocus
              && !table->accessibleName().isEmpty()
              && !table->accessibleDescription().isEmpty(),
          "Basic convergence table is keyboard reachable and named for assistive technology");
    check(diagnosticsTable->focusPolicy() == Qt::StrongFocus
              && !diagnosticsTable->accessibleName().isEmpty()
              && !diagnosticsTable->accessibleDescription().isEmpty(),
          "Advanced diagnostics table is keyboard reachable and has accessibility metadata");
    check(coupledTable->focusPolicy() == Qt::StrongFocus
              && !coupledTable->accessibleName().isEmpty()
              && !coupledTable->accessibleDescription().isEmpty(),
          "Coupled/Contact diagnostics table is keyboard reachable and has accessibility metadata");

    check(!summary->accessibleName().isEmpty()
              && !summary->accessibleDescription().isEmpty(),
          "Convergence summary has explicit accessibility metadata");
    check(!diagnosticsSummary->accessibleName().isEmpty()
              && !diagnosticsSummary->accessibleDescription().isEmpty(),
          "Advanced diagnostics summary has explicit accessibility metadata");
    check(!coupledSummary->accessibleName().isEmpty()
              && !coupledSummary->accessibleDescription().isEmpty(),
          "Coupled/Contact diagnostics summary has explicit accessibility metadata");

    std::cout << "Solver workspace accessibility acceptance: "
              << (checks - failures) << '/' << checks
              << (failures == 0 ? " PASS\n" : " FAIL\n");
    return failures == 0 ? 0 : 1;
}

} // namespace d26
