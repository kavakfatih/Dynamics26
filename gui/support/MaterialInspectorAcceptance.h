#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.1 — Material Inspector application acceptance.
//
// Bu kabul testi fiziksel mouse/trackpad UX testi değildir. Gerçek application
// composition üzerinde MaterialDetails widget sinyallerini çalıştırır ve
// Inspector'ın engineering state sahibi olmadığını doğrular:
//
//   Inspector widget
//       -> canonical DomainCommand
//       -> MaterialService
//       -> dependency / preflight
//       -> document Undo / Redo
//       -> Inspector refresh
//
// Test özellikle ad değişikliğinin solver girdisinden ayrılmasını, property
// edit'inin solution dependency'sini bayatlatmasını ve boş/no-op UI editlerinin
// document history üretmemesini güvence altına alır.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDoubleSpinBox>
#include <QEvent>
#include <QEventLoop>
#include <QLabel>
#include <QLineEdit>
#include <QMetaObject>
#include <QPushButton>
#include <QTimer>
#include <QUndoStack>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>

namespace d26 {

inline int runMaterialInspectorAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
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
    const auto nearlyEqual = [](const double a, const double b) {
        const double scale = std::max({1.0, std::abs(a), std::abs(b)});
        return std::abs(a - b) <= 1.0e-10 * scale;
    };

    const ServiceContext services = window.services();
    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    DetailsHost *detailsHost = window.detailsHost();
    check(services.project != nullptr && services.materials != nullptr
              && services.mesh != nullptr && services.analysis != nullptr
              && undoStack != nullptr && detailsHost != nullptr,
          "Material Inspector acceptance has authoritative services, DetailsHost and document Undo stack");
    if (services.project == nullptr || services.materials == nullptr || services.mesh == nullptr
        || services.analysis == nullptr || undoStack == nullptr || detailsHost == nullptr) {
        return 1;
    }

    const ObjectId materialId = services.materials->assignedMaterialId();
    const QVector<ObjectId> analyses = services.project->analyses();
    check(materialId != InvalidObjectId && services.materials->byId(materialId) != nullptr,
          "assigned Material project object resolves through MaterialService");
    check(!analyses.isEmpty(), "Material Inspector acceptance has an Analysis consumer");
    if (materialId == InvalidObjectId || services.materials->byId(materialId) == nullptr || analyses.isEmpty()) {
        return 1;
    }
    const ObjectId analysisId = analyses.first();

    // B1.1 dependency acceptance requires a valid solved baseline. The fixture
    // uses existing canonical application commands; Inspector never fabricates
    // solution/preflight state locally.
    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(window.runCommand(QStringLiteral("mesh.generate")),
              "Material Inspector fixture generates mesh through canonical command");
        settle(250);
    }
    check(services.analysis->preflight(analysisId).passed(),
          "Material Inspector fixture is Ready to Solve before editing");
    if (!services.analysis->preflight(analysisId).passed()) {
        return 1;
    }
    check(window.runCommand(QStringLiteral("analysis.solve")),
          "Material Inspector fixture solves through canonical application command");
    settle(350);
    check(services.analysis->hasResults(analysisId) && !services.analysis->solutionIsOutOfDate(analysisId),
          "Material Inspector fixture starts from a current solved state");

    window.selectObject(materialId);
    flushUi();

    auto *nameField = detailsHost->findChild<QLineEdit *>(QStringLiteral("materialInspector.name"));
    auto *youngField = detailsHost->findChild<QDoubleSpinBox *>(QStringLiteral("materialInspector.youngGPa"));
    auto *modelField = detailsHost->findChild<QComboBox *>(QStringLiteral("materialInspector.model"));
    auto *assignmentLabel = detailsHost->findChild<QLabel *>(QStringLiteral("materialInspector.assignment"));
    auto *assignButton = detailsHost->findChild<QPushButton *>(QStringLiteral("materialInspector.assignToBody"));
    auto *supportStatus = detailsHost->findChild<QLabel *>(QStringLiteral("materialInspector.staticStructuralStatus"));
    check(nameField != nullptr && youngField != nullptr && modelField != nullptr
              && assignmentLabel != nullptr && assignButton != nullptr && supportStatus != nullptr,
          "Material Inspector exposes stable property, assignment and solver-support bindings");
    if (nameField == nullptr || youngField == nullptr || modelField == nullptr
        || assignmentLabel == nullptr || assignButton == nullptr || supportStatus == nullptr) {
        return 1;
    }

    const MaterialDefinition original = *services.materials->byId(materialId);
    check(nameField->text() == original.name
              && nearlyEqual(youngField->value(), original.youngGPa)
              && modelField->currentIndex() == static_cast<int>(original.model),
          "Inspector reads exact authoritative MaterialService state without mutation");
    check(assignmentLabel->text() == QStringLiteral("Body 1") && !assignButton->isEnabled(),
          "assigned Material shows its body context and disables redundant assignment action");
    check(supportStatus->text() == QObject::tr("Destekleniyor"),
          "linear assigned Material reports current Static Structural support state");

    // ------------------------------------------------------------------
    // Name mutation contract: canonical RenameObjectCommand, no solver invalidation.
    // ------------------------------------------------------------------
    const int noOpIndex = undoStack->index();
    nameField->setText(original.name);
    QMetaObject::invokeMethod(nameField, "editingFinished", Qt::DirectConnection);
    flushUi();
    check(undoStack->index() == noOpIndex,
          "unchanged Material name does not create an empty document transaction");

    nameField->setText(QStringLiteral("   "));
    QMetaObject::invokeMethod(nameField, "editingFinished", Qt::DirectConnection);
    flushUi();
    check(undoStack->index() == noOpIndex
              && nameField->text() == original.name
              && services.materials->byId(materialId)->name == original.name,
          "empty Material name is rejected locally without mutating engineering state");

    const QString renamed = original.name + QStringLiteral(" Inspector");
    const int renameIndex = undoStack->index();
    nameField->setText(renamed);
    QMetaObject::invokeMethod(nameField, "editingFinished", Qt::DirectConnection);
    flushUi();
    const ProjectObject *renamedNode = services.project->object(materialId);
    check(undoStack->index() == renameIndex + 1
              && services.materials->byId(materialId)->name == renamed
              && renamedNode != nullptr && renamedNode->name == renamed,
          "Material name widget creates one canonical rename transaction and keeps tree identity synchronized");
    check(!services.analysis->solutionIsOutOfDate(analysisId),
          "Material display-name edit does not invalidate solved engineering input");

    undoStack->undo();
    flushUi();
    check(services.materials->byId(materialId)->name == original.name
              && nameField->text() == original.name
              && !services.analysis->solutionIsOutOfDate(analysisId),
          "Undo restores Material name and Inspector state without invalidating solution");
    undoStack->redo();
    flushUi();
    check(services.materials->byId(materialId)->name == renamed && nameField->text() == renamed,
          "Redo reapplies Material rename through authoritative service state");
    undoStack->undo();
    flushUi();

    // ------------------------------------------------------------------
    // Solver property contract: one undoable transaction + dependency invalidation.
    // ------------------------------------------------------------------
    settle(750); // SetMaterialPropertiesCommand merge penceresinden bilinçli ayrım.
    const double changedYoung = original.youngGPa * 0.9;
    const int propertyIndex = undoStack->index();
    youngField->setValue(changedYoung);
    flushUi();
    check(undoStack->index() == propertyIndex + 1
              && nearlyEqual(services.materials->byId(materialId)->youngGPa, changedYoung),
          "Young's Modulus widget creates exactly one undoable material-property transaction");
    check(services.analysis->solutionIsOutOfDate(analysisId),
          "assigned Material property edit marks existing solution Out of Date");
    check(services.analysis->preflight(analysisId).passed(),
          "valid unit-aware Material property edit remains Ready to Solve in authoritative Preflight");

    undoStack->undo();
    flushUi();
    check(nearlyEqual(services.materials->byId(materialId)->youngGPa, original.youngGPa)
              && nearlyEqual(youngField->value(), original.youngGPa),
          "Undo restores exact Young's Modulus in service and Inspector");
    check(!services.analysis->solutionIsOutOfDate(analysisId),
          "Undo to exact solved material input restores solution validity");

    // ------------------------------------------------------------------
    // Engineering validity remains authoritative in AnalysisService::preflight().
    // ------------------------------------------------------------------
    const int originalModelIndex = static_cast<int>(original.model);
    const int hyperelasticIndex = static_cast<int>(MaterialModel::MooneyRivlin);
    if (originalModelIndex != hyperelasticIndex) {
        const int modelIndex = undoStack->index();
        modelField->setCurrentIndex(hyperelasticIndex);
        flushUi();
        check(undoStack->index() == modelIndex + 1
                  && services.materials->byId(materialId)->model == MaterialModel::MooneyRivlin,
              "Material Model widget updates authoritative MaterialService through one transaction");
        check(!services.analysis->preflight(analysisId).passed(),
              "unsupported Static Structural material model is rejected by authoritative Preflight");
        check(supportStatus->text() == QObject::tr("Bu malzeme modeliyle etkin değil"),
              "Inspector presents unsupported solver state from the current Material model");
        undoStack->undo();
        flushUi();
        check(services.materials->byId(materialId)->model == original.model
                  && modelField->currentIndex() == originalModelIndex
                  && services.analysis->preflight(analysisId).passed()
                  && supportStatus->text() == QObject::tr("Destekleniyor"),
              "Undo restores model, Inspector support state and Ready-to-Solve Preflight state");
    }

    // Acceptance, sonraki application tests'e başlangıç Material state'ini bırakır.
    const MaterialDefinition *finalMaterial = services.materials->byId(materialId);
    check(finalMaterial != nullptr && finalMaterial->toJson() == original.toJson(),
          "Material Inspector acceptance leaves persistent Material definition unchanged");

    std::cout << "Material Inspector acceptance: " << (checks - failures) << "/" << checks << " PASS\n";
    return failures == 0 ? 0 : 1;
}

} // namespace d26
