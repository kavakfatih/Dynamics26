#pragma once

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/SolverTelemetry.h"
#include "../services/AnalysisService.h"
#include "../services/ContactService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/UtilityWorkspace.h"

#include <femcae/femcae.h>

#include <QApplication>
#include <QJsonObject>
#include <QLabel>
#include <QTableWidget>
#include <QUndoStack>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <vector>

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
    const auto renderedValueMatches = [](const QTableWidgetItem *item, const double expected) {
        if (item == nullptr) {
            return false;
        }
        bool ok = false;
        const double actual = item->text().toDouble(&ok);
        const double tolerance = 1.0e-7 * std::max(std::abs(expected), 1.0e-12);
        return ok && std::abs(actual - expected) <= tolerance;
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

    QUndoStack *undoStack = window.documentCommands() != nullptr ? window.documentCommands()->stack() : nullptr;
    check(undoStack != nullptr, "Solver workspace acceptance has document Undo stack");
    if (undoStack == nullptr) {
        return 1;
    }

    const int undoBefore = undoStack->count();

    // B2.1 foundation: typed DTO doğrudan presentation'a verildiğinde tablo ve
    // summary deterministik render edilir; bu telemetry document state değildir.
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

    check(undoStack->count() == undoBefore,
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
              && summary->text().contains(QStringLiteral("Newton iterasyonu = 9"))
              && summary->text().contains(QStringLiteral("Final residual norm")),
          "Session summary exposes state, iteration, cutback and residual-norm semantics");

    SolverConvergenceSnapshot emptySnapshot;
    utility->setConvergenceData(emptySnapshot);
    app.processEvents();

    check(table->rowCount() == 0, "Empty snapshot clears stale convergence rows");
    check(summary->text().contains(QStringLiteral("verisi yok")),
          "Empty snapshot exposes unavailable telemetry state");
    check(undoStack->count() == undoBefore,
          "Empty telemetry reset also leaves document Undo state unchanged");

    // B2.2 production-path acceptance: referans veri aynı gerçek C ABI nonlinear
    // verification preset'inden alınır. Ardından shell'deki verify.nonlinear
    // komutu çalıştırılır ve Utility Workspace satırları bu authoritative history
    // ile bire bir karşılaştırılır. Fake telemetry bu bölümde kullanılmaz.
    const ServiceContext services = window.services();
    check(services.project != nullptr && services.geometry != nullptr && services.mesh != nullptr
              && services.materials != nullptr && services.analysis != nullptr,
          "Production telemetry acceptance has engineering service collaborators");
    if (services.project == nullptr || services.geometry == nullptr || services.mesh == nullptr
        || services.materials == nullptr || services.analysis == nullptr) {
        return 1;
    }

    const MaterialDefinition *material = services.materials->assigned();
    const double young = (material != nullptr ? material->youngGPa : 210.0) * 1.0e9;
    const double poisson = material != nullptr ? material->poisson : 0.30;
    constexpr int kCapacity = 512;
    std::vector<int> attempts(kCapacity);
    std::vector<int> iterations(kCapacity);
    std::vector<int> converged(kCapacity);
    std::vector<double> loadFactors(kCapacity);
    std::vector<double> relativeResiduals(kCapacity);
    std::vector<double> relativeDisplacements(kCapacity);
    std::vector<double> alphas(kCapacity);
    double displacement = 0.0;
    double completedLoadFactor = 0.0;
    double finalResidualNorm = 0.0;
    int acceptedSteps = 0;
    int totalIterations = 0;
    int cutbacks = 0;
    int historyCount = 0;
    const int referenceStatus = fem_demo_nonlinear_hex8(
        young, poisson, 1.0e-4, 1.0, 1000.0, 0.25, 0.01, 0.5, 1, 1, 25, 1,
        &displacement, &completedLoadFactor, &finalResidualNorm, &acceptedSteps,
        &totalIterations, &cutbacks, kCapacity, &historyCount,
        attempts.data(), iterations.data(), loadFactors.data(), relativeResiduals.data(),
        relativeDisplacements.data(), alphas.data(), converged.data());
    check(referenceStatus == 0, "Reference nonlinear C ABI verification solve succeeds");
    check(historyCount > 0, "Reference nonlinear C ABI exposes non-empty convergence history");
    if (referenceStatus != 0 || historyCount <= 0) {
        return 1;
    }

    const int undoCountBeforeCommand = undoStack->count();
    const int undoIndexBeforeCommand = undoStack->index();
    const bool dirtyBeforeCommand = window.documentCommands()->isDirty();
    const ObjectId nextObjectIdBeforeCommand = services.project->peekNextId();
    const QJsonObject geometryBeforeCommand = services.geometry->projectJson();
    const QJsonObject meshBeforeCommand = services.mesh->projectJson();
    const QJsonObject materialsBeforeCommand = services.materials->toJson();
    const QJsonObject analysesBeforeCommand = services.analysis->toJson();
    const QJsonObject namedSelectionsBeforeCommand = services.namedSelections != nullptr
        ? services.namedSelections->toJson() : QJsonObject{};
    const QJsonObject contactsBeforeCommand = services.contacts != nullptr
        ? services.contacts->toJson() : QJsonObject{};

    check(window.runCommand(QStringLiteral("verify.nonlinear")),
          "Production verify.nonlinear command executes through canonical command registry");
    app.processEvents();

    check(table->rowCount() == historyCount,
          "Production Convergence table row count matches real C ABI historyCount");

    bool identityMatches = table->rowCount() == historyCount;
    bool loadFactorMatches = identityMatches;
    bool relativeResidualMatches = identityMatches;
    bool relativeDisplacementMatches = identityMatches;
    bool alphaMatches = identityMatches;
    bool convergedStateMatches = identityMatches;
    for (int row = 0; row < historyCount && row < table->rowCount(); ++row) {
        const std::size_t i = static_cast<std::size_t>(row);
        identityMatches = identityMatches
            && table->item(row, 0) != nullptr
            && table->item(row, 1) != nullptr
            && table->item(row, 0)->text().toInt() == attempts[i]
            && table->item(row, 1)->text().toInt() == iterations[i];
        loadFactorMatches = loadFactorMatches && renderedValueMatches(table->item(row, 2), loadFactors[i]);
        relativeResidualMatches = relativeResidualMatches
            && renderedValueMatches(table->item(row, 3), relativeResiduals[i]);
        relativeDisplacementMatches = relativeDisplacementMatches
            && renderedValueMatches(table->item(row, 4), relativeDisplacements[i]);
        alphaMatches = alphaMatches && renderedValueMatches(table->item(row, 5), alphas[i]);
        convergedStateMatches = convergedStateMatches
            && table->item(row, 6) != nullptr
            && table->item(row, 6)->text()
                == (converged[i] != 0 ? QStringLiteral("Converged") : QStringLiteral("Iterating"));
    }
    check(identityMatches, "Attempt and iteration identities come from real nonlinear telemetry");
    check(loadFactorMatches, "Load Factor values match real nonlinear telemetry");
    check(relativeResidualMatches, "Rel. |R| values match real nonlinear telemetry");
    check(relativeDisplacementMatches, "Rel. delta-u values match real nonlinear telemetry");
    check(alphaMatches, "Line-search alpha values match real nonlinear telemetry");
    check(convergedStateMatches, "Converged state flags match real nonlinear telemetry");

    const QString summaryText = summary->text();
    check(summaryText.contains(QStringLiteral("Converged")),
          "Production summary reports converged verification state");
    check(summaryText.contains(QStringLiteral("λ = %1").arg(completedLoadFactor, 0, 'g', 8)),
          "Production summary contains completed load factor from C ABI");
    check(summaryText.contains(QStringLiteral("Kabul edilen adım = %1").arg(acceptedSteps)),
          "Production summary contains accepted steps from C ABI");
    check(summaryText.contains(QStringLiteral("Newton iterasyonu = %1").arg(totalIterations)),
          "Production summary contains total Newton iterations from C ABI");
    check(summaryText.contains(QStringLiteral("Cutback = %1").arg(cutbacks)),
          "Production summary contains direct C ABI cutback count");
    check(summaryText.contains(QStringLiteral("Final residual norm = %1").arg(finalResidualNorm, 0, 'g', 8)),
          "Production summary preserves absolute final residual norm semantics");

    check(undoStack->count() == undoCountBeforeCommand
              && undoStack->index() == undoIndexBeforeCommand
              && window.documentCommands()->isDirty() == dirtyBeforeCommand,
          "Production verification telemetry creates no document Undo or dirty-state mutation");
    check(services.project->peekNextId() == nextObjectIdBeforeCommand,
          "Production verification command allocates no engineering ObjectId");
    check(services.geometry->projectJson() == geometryBeforeCommand
              && services.mesh->projectJson() == meshBeforeCommand
              && services.materials->toJson() == materialsBeforeCommand
              && services.analysis->toJson() == analysesBeforeCommand
              && (services.namedSelections == nullptr
                      || services.namedSelections->toJson() == namedSelectionsBeforeCommand)
              && (services.contacts == nullptr || services.contacts->toJson() == contactsBeforeCommand),
          "Production verification command does not mutate document engineering input");

    std::cout << "Solver workspace acceptance: " << (checks - failures) << '/' << checks
              << (failures == 0 ? " PASS\n" : " FAIL\n");
    return failures == 0 ? 0 : 1;
}

} // namespace d26
