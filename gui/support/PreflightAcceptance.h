#pragma once

// Dynamics26 V1.1.0-alpha.4 — Integrated Preflight application acceptance.
//
// Bu test fiziksel mouse/trackpad kabulünün yerine geçmez. Gerçek
// Dynamics26MainWindow composition'ı üzerinde Preflight Details kontrollerinin
// authoritative AnalysisService::preflight() raporuna bağlı kaldığını ve saf
// navigation işlemlerinin document Undo history'sini değiştirmediğini kanıtlar.
//
// Özellikle material assignment bilinçli olarak geçici şekilde kaldırılır:
// Dynamics26 kullanıcı adına malzeme seçmez/atamaz; Preflight yalnız Materials
// authoring bağlamına götürür. Fixture sonunda material state exact ObjectId ile
// geri yüklenir.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/ProjectNavigator.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QJsonObject>
#include <QLabel>
#include <QToolButton>
#include <QUndoStack>

#include <iostream>
#include <string>

namespace d26 {

inline int runPreflightAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
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
    check(services.project != nullptr && services.materials != nullptr && services.analysis != nullptr,
          "Alpha.4 Preflight acceptance has Project/Material/Analysis collaborators");
    if (services.project == nullptr || services.materials == nullptr || services.analysis == nullptr) {
        return 1;
    }

    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    check(undoStack != nullptr,
          "Alpha.4 Preflight acceptance has document Undo stack");
    if (undoStack == nullptr) {
        return 1;
    }

    const QVector<ObjectId> analyses = services.project->analyses();
    check(!analyses.isEmpty(),
          "Alpha.4 Preflight acceptance has an analysis object");
    if (analyses.isEmpty()) {
        return 1;
    }
    const ObjectId analysisId = analyses.first();
    const ObjectId materialsNode = services.project->materialsNode();
    const ObjectId originalAssignedMaterial = services.materials->assignedMaterialId();
    const QJsonObject materialSnapshot = services.materials->toJson();
    const int originalUndoIndex = undoStack->index();

    check(originalAssignedMaterial != InvalidObjectId && services.materials->assigned() != nullptr,
          "Alpha.4 material fixture starts with a real assigned material");

    // ------------------------------------------------------------------
    // Missing material -> authoritative report + summary navigation
    // ------------------------------------------------------------------
    // clear() test setup'tır; document command değildir. UI refresh'i gerçek
    // kullanıcı context değişimiyle zorlanır, ardından AnalysisDetails aynı
    // AnalysisService::preflight() raporunu render eder.
    services.materials->clear();
    check(services.materials->assigned() == nullptr,
          "material fixture removes assignment without inventing replacement material");

    window.selectObject(services.project->meshNode());
    flushUi();
    window.selectObject(analysisId);
    flushUi();

    const PreflightReport report = services.analysis->preflight(analysisId);
    int blockingCount = 0;
    int warningCount = 0;
    for (const auto &entry : report.checks) {
        if (entry.status == PreflightCheck::Status::Failed) {
            ++blockingCount;
        } else if (entry.status == PreflightCheck::Status::Warning) {
            ++warningCount;
        }
    }
    check(blockingCount > 0,
          "missing material produces a blocking Preflight issue");

    auto *summary = window.findChild<QLabel *>(QStringLiteral("Dynamics26PreflightSummary"));
    auto *firstIssue = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightNextIssue"));
    auto *goMaterials = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightGoMaterials"));
    check(summary != nullptr && firstIssue != nullptr && goMaterials != nullptr,
          "Analysis Details exposes summary, first-issue and Materials navigation controls");

    if (summary != nullptr) {
        const QString expectedSummary = QStringLiteral("%1 engel · %2 uyarı")
                                            .arg(blockingCount)
                                            .arg(warningCount);
        check(summary->text() == expectedSummary,
              "Preflight summary counts exactly match authoritative report checks");
    }

    if (firstIssue != nullptr) {
        const int undoBeforeFirstIssue = undoStack->index();
        firstIssue->click();
        flushUi();
        check(window.navigator()->selectedObject() == materialsNode,
              "First Issue navigation focuses Materials for the first blocking material issue");
        check(undoStack->index() == undoBeforeFirstIssue,
              "First Issue navigation does not mutate document Undo history");
    }

    // Materials quick navigation aynı engineering target'e gider; tekrar Analysis
    // Details'a dönülerek gerçek widget instance'ı yeniden çözdürülür.
    window.selectObject(analysisId);
    flushUi();
    goMaterials = window.findChild<QToolButton *>(QStringLiteral("Dynamics26PreflightGoMaterials"));
    if (goMaterials != nullptr) {
        const int undoBeforeMaterials = undoStack->index();
        goMaterials->click();
        flushUi();
        check(window.navigator()->selectedObject() == materialsNode,
              "Missing-material guidance focuses the Materials authoring context");
        check(undoStack->index() == undoBeforeMaterials,
              "Materials guidance navigation does not mutate document Undo history");
    } else {
        check(false, "Missing-material guidance control remains available after Details refresh");
    }

    // Fixture'i exact persistent state'e geri getir. MaterialService::fromJson
    // requested ObjectId'leri korur; böylece sonraki acceptance ve solver testleri
    // başlangıç engineering identity'sini kullanmaya devam eder.
    services.materials->fromJson(materialSnapshot);
    services.dependencies->evaluate();
    flushUi();
    check(services.materials->assignedMaterialId() == originalAssignedMaterial
              && services.materials->assigned() != nullptr,
          "material fixture restores original assigned material ObjectId exactly");
    check(undoStack->index() == originalUndoIndex,
          "Alpha.4 Preflight acceptance leaves document Undo history unchanged");

    window.selectObject(analysisId);
    flushUi();
    check(services.analysis->preflight(analysisId).passed(),
          "restored integrated model returns to Ready to Solve");

    std::cout << (failures == 0 ? "Alpha.4 Preflight acceptance PASS" : "Alpha.4 Preflight acceptance FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
