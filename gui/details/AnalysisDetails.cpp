#include "AnalysisDetails.h"

#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "../core/UiTheme.h"

namespace d26 {

AnalysisDetails::AnalysisDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    analysisType_ = definition->addValueRow(tr("Analysis Type"));
    largeDeflection_ = makeCombo({tr("Off"), tr("On")});
    definition->addRow(tr("Large Deflection"), largeDeflection_);

    auto *formulation = addSection(tr("Formulation"));
    incompressibility_ = makeCombo({tr("Automatic"), tr("Compressible"), tr("Nearly Incompressible")});
    formulation->addRow(tr("Incompressibility"), incompressibility_);

    auto *solverSection = addSection(tr("Solver"));
    solver_ = solverSection->addValueRow(tr("Solver"), tr("Automatic"));

    validation_ = addSection(tr("Validation"));
    validationBody_ = new QWidget(this);
    validationLayout_ = new QVBoxLayout(validationBody_);
    validationLayout_->setContentsMargins(0, 0, 0, 0);
    validationLayout_->setSpacing(2);
    validation_->addFullWidth(validationBody_);

    auto *statusSection = addSection(tr("Status"));
    status_ = statusSection->addValueRow(tr("State"));
    solve_ = makeActionButton(tr("Solve"));
    statusSection->addFullWidth(solve_);

    auto *advanced = addSection(tr("Advanced Solver Settings"), true, true);
    resolvedFormulation_ = advanced->addValueRow(tr("Resolved Formulation"));
    elementTechnology_ = advanced->addValueRow(tr("Element Technology"));
    linearSolver_ = advanced->addValueRow(tr("Linear Solver"));
    dofLimit_ = advanced->addValueRow(tr("Practical DOF Limit"));
    newtonMethod_ = advanced->addValueRow(tr("Newton Method"));
    advanced->addNote(tr("Bu bölüm kullanıcı niyetinin hangi solver implementasyonuna çözüldüğünü gösterir. "
                         "Değerler otomatik türetilir; doğrudan düzenlenmez."));

    addStretch();

    connect(solve_, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("analysis.solve")); });
    connect(incompressibility_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        if (updating_) {
            return;
        }
        const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
        const AnalysisRecord *record = services_.analysis->analysis(analysisId);
        if (record == nullptr) {
            return;
        }
        const auto after = static_cast<IncompressibilityIntent>(index);
        if (record->incompressibility == after) {
            return;
        }
        services_.commands->push(
            new commands::SetIncompressibilityCommand(services_, analysisId, record->incompressibility, after));
        emit modelEdited();
    });
    connect(largeDeflection_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        if (updating_) {
            return;
        }
        const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
        const AnalysisRecord *record = services_.analysis->analysis(analysisId);
        if (record == nullptr || record->largeDeflection == (index == 1)) {
            return;
        }
        services_.commands->push(
            new commands::SetLargeDeflectionCommand(services_, analysisId, record->largeDeflection, index == 1));
        emit modelEdited();
    });
}

void AnalysisDetails::refresh()
{
    const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
    const AnalysisRecord *record = services_.analysis->analysis(analysisId);
    if (record == nullptr) {
        return;
    }
    updating_ = true;
    analysisType_->setText(displayName(record->type));
    largeDeflection_->setCurrentIndex(record->largeDeflection ? 1 : 0);
    incompressibility_->setCurrentIndex(static_cast<int>(record->incompressibility));

    // --- preflight (§24/§25) ---
    const PreflightReport report = services_.analysis->preflight(analysisId);
    while (QLayoutItem *item = validationLayout_->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
    for (const auto &check : report.checks) {
        const QString mark = check.status == PreflightCheck::Status::Passed
            ? QStringLiteral("✓")
            : (check.status == PreflightCheck::Status::Warning ? QStringLiteral("!") : QStringLiteral("✕"));
        auto *line = new QLabel(QStringLiteral("%1  %2%3")
                                    .arg(mark, check.label,
                                         check.detail.isEmpty() ? QString()
                                                                : QStringLiteral(" — ") + check.detail),
                                validationBody_);
        line->setWordWrap(true);
        QFont font = line->font();
        font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.5));
        line->setFont(font);
        ui::StatusTone tone = ui::StatusTone::UpToDate;
        if (check.status == PreflightCheck::Status::Warning) {
            tone = ui::StatusTone::Warning;
        } else if (check.status == PreflightCheck::Status::Failed) {
            tone = ui::StatusTone::Error;
        }
        QPalette linePalette = line->palette();
        linePalette.setColor(QPalette::WindowText, ui::statusColor(tone));
        linePalette.setColor(QPalette::Text, ui::statusColor(tone));
        line->setPalette(linePalette);

        // Alpha.4 Integrated Modeling Workflow foundation:
        // PreflightCheck zaten authoritative subject ObjectId taşıyor. Validation
        // mantığını veya engineering state'i kopyalamadan, yalnız Failed/Warning
        // satırını canonical MainWindow::selectObject() navigation yoluna bağlarız.
        // Böylece "Mesh güncel değil" veya "scope stale" gibi bir diagnostic
        // doğrudan ilgili Navigator/Details nesnesine götürür; Undo geçmişi değişmez.
        const bool actionable = check.status != PreflightCheck::Status::Passed
            && check.subject != InvalidObjectId
            && services_.project != nullptr
            && services_.project->object(check.subject) != nullptr;
        if (actionable) {
            auto *row = new QWidget(validationBody_);
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(6);
            rowLayout->addWidget(line, 1);

            auto *show = new QToolButton(row);
            show->setText(tr("Göster"));
            show->setAutoRaise(true);
            show->setToolButtonStyle(Qt::ToolButtonTextOnly);
            show->setToolTip(tr("Model ağacında ilgili nesneyi göster"));
            show->setObjectName(QStringLiteral("Dynamics26PreflightSubject_%1")
                                    .arg(static_cast<qulonglong>(check.subject)));
            rowLayout->addWidget(show, 0, Qt::AlignTop);
            connect(show, &QToolButton::clicked, this, [this, subject = check.subject] {
                if (auto *mainWindow = qobject_cast<Dynamics26MainWindow *>(window())) {
                    mainWindow->selectObject(subject);
                }
            });
            validationLayout_->addWidget(row);
        } else {
            validationLayout_->addWidget(line);
        }
    }

    const bool canSolve = report.passed();
    if (record->solved && services_.analysis->solutionIsOutOfDate(analysisId)) {
        status_->setText(tr("Solved — Out of Date"));
    } else if (record->solved) {
        status_->setText(tr("Solved"));
    } else if (canSolve) {
        status_->setText(tr("Ready to Solve"));
    } else {
        status_->setText(tr("Not Ready"));
    }
    solve_->setEnabled(canSolve);
    solve_->setToolTip(canSolve ? QString() : report.firstFailure());

    const bool mixed = services_.analysis->resolvedFormulation(analysisId) == ResolvedFormulation::MixedUP;
    resolvedFormulation_->setText(mixed ? tr("Mixed displacement–pressure (u–p)") : tr("Displacement-based (u)"));
    elementTechnology_->setText(services_.analysis->resolvedElementTechnology(analysisId));
    linearSolver_->setText(services_.analysis->resolvedLinearSolver());
    dofLimit_->setText(QStringLiteral("%1 DOF").arg(AnalysisService::maximumDofThreshold()));
    newtonMethod_->setText(record->type == AnalysisType::StaticStructural && !record->largeDeflection
                               ? tr("— (lineer analiz)")
                               : tr("Full Newton-Raphson"));
    updating_ = false;
}

} // namespace d26