#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.5 — shared Inspector UX acceptance.
//
// Bu test engineering/domain state üretmez. Ortak Details primitive'lerinin
// dar Inspector genişliğinde taşmama ve keyboard accessibility sözleşmesini,
// ardından gerçek Analysis Inspector action focus contract'ını doğrular.

#include "../details/AnalysisDetails.h"
#include "../details/DetailsPage.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QPushButton>
#include <QSizePolicy>
#include <QToolButton>

#include <iostream>
#include <string>

namespace d26 {

inline int runInspectorUxAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
{
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const std::string &message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };

    DetailsSection section(QStringLiteral("Advanced"), true, true);
    DetailsRow *row = section.addRow(
        QStringLiteral("Resolved Element Technology and Formulation"),
        new QLabel(QStringLiteral("HEX8 / Mixed u-p")));
    section.resize(268, 180);
    section.ensurePolished();
    if (section.layout() != nullptr) {
        section.layout()->activate();
    }
    if (row != nullptr && row->layout() != nullptr) {
        row->layout()->activate();
    }
    app.processEvents();

    auto *rowLabel = row != nullptr
        ? row->findChild<QLabel *>(QStringLiteral("Dynamics26DetailsRowLabel")) : nullptr;
    check(rowLabel != nullptr,
          "shared DetailsRow exposes stable Inspector label binding");
    check(rowLabel != nullptr && rowLabel->wordWrap()
              && rowLabel->sizePolicy().horizontalPolicy() == QSizePolicy::Fixed
              && rowLabel->sizePolicy().verticalPolicy() == QSizePolicy::Preferred,
          "long Inspector labels wrap inside the fixed label column instead of clipping horizontally");
    check(rowLabel != nullptr && rowLabel->minimumWidth() == rowLabel->maximumWidth()
              && rowLabel->minimumWidth() > 0
              && rowLabel->hasHeightForWidth()
              && rowLabel->heightForWidth(rowLabel->minimumWidth()) > rowLabel->fontMetrics().height(),
          "wrapped Inspector label can grow vertically while preserving compact column alignment");

    auto *disclosure = section.findChild<QToolButton *>(QStringLiteral("Dynamics26DetailsSectionDisclosure"));
    check(disclosure != nullptr && disclosure->focusPolicy() == Qt::StrongFocus
              && !disclosure->accessibleName().isEmpty()
              && !disclosure->accessibleDescription().isEmpty(),
          "collapsible Inspector section exposes keyboard focus and accessibility metadata");
    const QString collapsedDescription = disclosure != nullptr ? disclosure->accessibleDescription() : QString();
    if (disclosure != nullptr) {
        disclosure->click();
        app.processEvents();
    }
    check(disclosure != nullptr && disclosure->isChecked()
              && disclosure->arrowType() == Qt::DownArrow
              && disclosure->accessibleDescription() != collapsedDescription,
          "collapsible Inspector disclosure updates expanded state and accessibility guidance together");

    window.newProjectWithoutPrompt();
    const ObjectId analysisId = window.firstObjectOfType(ObjectType::Analysis);
    window.selectObject(analysisId);
    app.processEvents();
    auto *analysis = window.findChild<AnalysisDetails *>();
    auto *preflight = analysis != nullptr
        ? analysis->findChild<QPushButton *>(QStringLiteral("analysisInspector.preflight")) : nullptr;
    auto *solve = analysis != nullptr
        ? analysis->findChild<QPushButton *>(QStringLiteral("analysisInspector.solve")) : nullptr;
    check(analysisId != InvalidObjectId && analysis != nullptr
              && analysis->objectId() == analysisId && preflight != nullptr && solve != nullptr,
          "Inspector UX acceptance resolves the real current Analysis action surface");
    check(preflight != nullptr && solve != nullptr
              && preflight->focusPolicy() == Qt::StrongFocus
              && solve->focusPolicy() == Qt::StrongFocus,
          "shared Inspector action buttons remain reachable in the keyboard focus chain");

    window.newProjectWithoutPrompt();
    window.documentCommands()->resetHistory();
    std::cout << "Inspector UX consistency acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
