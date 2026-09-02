#include "AnalysisDetails.h"

#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../commands/DomainCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLayoutItem>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

#include "../core/UiTheme.h"

namespace d26 {

AnalysisDetails::AnalysisDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    name_ = new QLineEdit(this);
    name_->setObjectName(QStringLiteral("analysisInspector.name"));
    definition->addRow(tr("Name"), name_);
    analysisType_ = definition->addValueRow(tr("Analysis Type"));
    analysisType_->setObjectName(QStringLiteral("analysisInspector.procedure"));
    largeDeflection_ = makeCombo({tr("Off"), tr("On")});
    largeDeflection_->setObjectName(QStringLiteral("analysisInspector.largeDeflection"));
    definition->addRow(tr("Large Deflection"), largeDeflection_);

    auto *formulation = addSection(tr("Formulation"));
    incompressibility_ = makeCombo({tr("Automatic"), tr("Compressible"), tr("Nearly Incompressible")});
    incompressibility_->setObjectName(QStringLiteral("analysisInspector.incompressibility"));
    formulation->addRow(tr("Incompressibility"), incompressibility_);

    auto *solverSection = addSection(tr("Solver"));
    solver_ = solverSection->addValueRow(tr("Solver"), tr("Automatic"));
    solver_->setObjectName(QStringLiteral("analysisInspector.solver"));

    auto *readiness = addSection(tr("Model Readiness"));
    activeSupports_ = readiness->addValueRow(tr("Supports"));
    activeSupports_->setObjectName(QStringLiteral("analysisInspector.activeSupports"));
    activeLoads_ = readiness->addValueRow(tr("Loads"));
    activeLoads_->setObjectName(QStringLiteral("analysisInspector.activeLoads"));
    meshReadiness_ = readiness->addValueRow(tr("Mesh"));
    meshReadiness_->setObjectName(QStringLiteral("analysisInspector.meshReadiness"));
    materialReadiness_ = readiness->addValueRow(tr("Material"));
    materialReadiness_->setObjectName(QStringLiteral("analysisInspector.materialReadiness"));

    validation_ = addSection(tr("Validation"));
    validationBody_ = new QWidget(this);
    validationLayout_ = new QVBoxLayout(validationBody_);
    validationLayout_->setContentsMargins(0, 0, 0, 0);
    validationLayout_->setSpacing(2);
    validation_->addFullWidth(validationBody_);

    auto *statusSection = addSection(tr("Status"));
    status_ = statusSection->addValueRow(tr("State"));
    status_->setObjectName(QStringLiteral("analysisInspector.state"));
    resultAvailability_ = statusSection->addValueRow(tr("Results"));
    resultAvailability_->setObjectName(QStringLiteral("analysisInspector.results"));

    auto *actions = new QWidget(this);
    auto *actionsLayout = new QHBoxLayout(actions);
    actionsLayout->setContentsMargins(0, 0, 0, 0);
    actionsLayout->setSpacing(6);
    preflight_ = makeActionButton(tr("Run Preflight"));
    preflight_->setObjectName(QStringLiteral("analysisInspector.preflight"));
    solve_ = makeActionButton(tr("Solve"));
    solve_->setObjectName(QStringLiteral("analysisInspector.solve"));
    actionsLayout->addWidget(preflight_);
    actionsLayout->addWidget(solve_);
    statusSection->addFullWidth(actions);

    auto *advanced = addSection(tr("Advanced Solver Settings"), true, true);
    resolvedFormulation_ = advanced->addValueRow(tr("Resolved Formulation"));
    resolvedFormulation_->setObjectName(QStringLiteral("analysisInspector.resolvedFormulation"));
    elementTechnology_ = advanced->addValueRow(tr("Element Technology"));
    elementTechnology_->setObjectName(QStringLiteral("analysisInspector.elementTechnology"));
    linearSolver_ = advanced->addValueRow(tr("Linear Solver"));
    linearSolver_->setObjectName(QStringLiteral("analysisInspector.linearSolver"));
    dofLimit_ = advanced->addValueRow(tr("Practical DOF Limit"));
    dofLimit_->setObjectName(QStringLiteral("analysisInspector.dofLimit"));
    newtonMethod_ = advanced->addValueRow(tr("Newton Method"));
    newtonMethod_->setObjectName(QStringLiteral("analysisInspector.newtonMethod"));
    advanced->addNote(tr("Bu bölüm kullanıcı niyetinin hangi solver implementasyonuna çözüldüğünü gösterir. "
                         "Değerler otomatik türetilir; doğrudan düzenlenmez."));

    addStretch();

    connect(preflight_, &QPushButton::clicked, this,
            [this] { emit requestCommand(QStringLiteral("analysis.preflight")); });
    connect(solve_, &QPushButton::clicked, this,
            [this] { emit requestCommand(QStringLiteral("analysis.solve")); });

    connect(name_, &QLineEdit::editingFinished, this, [this] {
        if (updating_ || services_.project == nullptr || services_.analysis == nullptr
            || services_.commands == nullptr) {
            return;
        }
        const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
        const ProjectObject *object = services_.project->object(analysisId);
        if (object == nullptr) {
            return;
        }
        const QString requested = name_->text().trimmed();
        if (requested.isEmpty()) {
            updating_ = true;
            name_->setText(object->name);
            updating_ = false;
            return;
        }
        if (requested == object->name) {
            if (name_->text() != requested) {
                updating_ = true;
                name_->setText(requested);
                updating_ = false;
            }
            return;
        }
        services_.commands->push(new commands::RenameObjectCommand(services_, analysisId, requested));
        emit modelEdited();
    });

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

    if (services_.project != nullptr) {
        if (const ProjectObject *object = services_.project->object(analysisId)) {
            name_->setText(object->name);
        }
    }
    analysisType_->setText(displayName(record->type));
    largeDeflection_->setCurrentIndex(record->largeDeflection ? 1 : 0);
    incompressibility_->setCurrentIndex(static_cast<int>(record->incompressibility));

    int activeSupportCount = 0;
    for (const ObjectId id : record->supports) {
        if (services_.project != nullptr && services_.project->object(id) != nullptr
            && !services_.project->isSuppressed(id)) {
            ++activeSupportCount;
        }
    }
    int activeLoadCount = 0;
    for (const ObjectId id : record->loads) {
        if (services_.project != nullptr && services_.project->object(id) != nullptr
            && !services_.project->isSuppressed(id)) {
            ++activeLoadCount;
        }
    }
    activeSupports_->setText(tr("%1 active · %2 total")
                                 .arg(activeSupportCount)
                                 .arg(record->supports.size()));
    activeLoads_->setText(tr("%1 active · %2 total")
                              .arg(activeLoadCount)
                              .arg(record->loads.size()));

    if (services_.mesh == nullptr || !services_.mesh->hasMesh()) {
        meshReadiness_->setText(tr("Not generated"));
    } else if (services_.mesh->isOutOfDate()) {
        meshReadiness_->setText(tr("Out of Date · generation %1")
                                    .arg(services_.mesh->generation()));
    } else {
        meshReadiness_->setText(tr("Ready · generation %1")
                                    .arg(services_.mesh->generation()));
    }

    const MaterialDefinition *assignedMaterial = services_.materials != nullptr
        ? services_.materials->assigned() : nullptr;
    materialReadiness_->setText(assignedMaterial != nullptr
                                    ? tr("Ready · %1").arg(assignedMaterial->name)
                                    : tr("Missing assignment"));

    // --- preflight (§24/§25) ---
    const PreflightReport report = services_.analysis->preflight(analysisId);
    while (QLayoutItem *item = validationLayout_->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }

    // Alpha.4 summary yalnız aynı authoritative PreflightReport'u özetler.
    // Yeni bir validation state'i veya paralel kural seti oluşturulmaz. İlk
    // actionable konu önce Failed, sonra Warning sırasıyla seçilir; navigation
    // document mutation değildir ve canonical MainWindow::selectObject yoluna gider.
    const bool materialMissing = assignedMaterial == nullptr;
    int blockingCount = 0;
    int warningCount = 0;
    ObjectId firstBlockingSubject = InvalidObjectId;
    ObjectId firstWarningSubject = InvalidObjectId;
    bool firstFailureEncountered = false;
    for (const auto &check : report.checks) {
        if (check.status == PreflightCheck::Status::Failed) {
            ++blockingCount;
            if (!firstFailureEncountered) {
                firstFailureEncountered = true;
                if (check.subject != InvalidObjectId && services_.project != nullptr
                    && services_.project->object(check.subject) != nullptr) {
                    firstBlockingSubject = check.subject;
                } else if (materialMissing && services_.project != nullptr) {
                    firstBlockingSubject = services_.project->materialsNode();
                }
            }
        } else if (check.status == PreflightCheck::Status::Warning) {
            ++warningCount;
            if (firstWarningSubject == InvalidObjectId && check.subject != InvalidObjectId
                && services_.project != nullptr && services_.project->object(check.subject) != nullptr) {
                firstWarningSubject = check.subject;
            }
        }
    }

    auto *summaryRow = new QWidget(validationBody_);
    auto *summaryLayout = new QHBoxLayout(summaryRow);
    summaryLayout->setContentsMargins(0, 0, 0, 3);
    summaryLayout->setSpacing(6);
    auto *summary = new QLabel(summaryRow);
    summary->setObjectName(QStringLiteral("Dynamics26PreflightSummary"));
    if (blockingCount == 0 && warningCount == 0) {
        summary->setText(tr("Ready to Solve · engel yok"));
    } else {
        summary->setText(tr("%1 engel · %2 uyarı").arg(blockingCount).arg(warningCount));
    }
    QFont summaryFont = summary->font();
    summaryFont.setBold(true);
    summary->setFont(summaryFont);
    const ui::StatusTone summaryTone = blockingCount > 0
        ? ui::StatusTone::Error
        : (warningCount > 0 ? ui::StatusTone::Warning : ui::StatusTone::UpToDate);
    QPalette summaryPalette = summary->palette();
    summaryPalette.setColor(QPalette::WindowText, ui::statusColor(summaryTone));
    summaryPalette.setColor(QPalette::Text, ui::statusColor(summaryTone));
    summary->setPalette(summaryPalette);
    summaryLayout->addWidget(summary, 1);

    const ObjectId firstActionableSubject = firstBlockingSubject != InvalidObjectId
        ? firstBlockingSubject : firstWarningSubject;
    if (firstActionableSubject != InvalidObjectId) {
        auto *nextIssue = new QToolButton(summaryRow);
        nextIssue->setText(tr("İlk Konuya Git"));
        nextIssue->setAutoRaise(true);
        nextIssue->setToolButtonStyle(Qt::ToolButtonTextOnly);
        nextIssue->setObjectName(QStringLiteral("Dynamics26PreflightNextIssue"));
        nextIssue->setToolTip(tr("İlk çözülmesi gereken model nesnesini göster"));
        summaryLayout->addWidget(nextIssue, 0, Qt::AlignTop);
        connect(nextIssue, &QToolButton::clicked, this, [this, firstActionableSubject] {
            if (auto *mainWindow = qobject_cast<Dynamics26MainWindow *>(window())) {
                mainWindow->selectObject(firstActionableSubject);
            }
        });
    }
    validationLayout_->addWidget(summaryRow);

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

            if (services_.project->typeOf(check.subject) == ObjectType::Mesh) {
                auto *fix = new QToolButton(row);
                fix->setText(tr("Mesh Üret"));
                fix->setAutoRaise(true);
                fix->setToolButtonStyle(Qt::ToolButtonTextOnly);
                fix->setToolTip(tr("Güncel FEM mesh üret"));
                fix->setObjectName(QStringLiteral("Dynamics26PreflightFixMesh"));
                rowLayout->addWidget(fix, 0, Qt::AlignTop);
                connect(fix, &QToolButton::clicked, this, [this] {
                    emit requestCommand(QStringLiteral("mesh.generate"));
                });
            }
            validationLayout_->addWidget(row);
        } else {
            validationLayout_->addWidget(line);
        }
    }

    const bool hasActiveSupport = activeSupportCount > 0;
    const bool hasActiveLoad = activeLoadCount > 0;
    if (materialMissing || !hasActiveSupport || !hasActiveLoad) {
        auto *quickFixRow = new QWidget(validationBody_);
        auto *quickFixLayout = new QHBoxLayout(quickFixRow);
        quickFixLayout->setContentsMargins(0, 3, 0, 0);
        quickFixLayout->setSpacing(6);
        quickFixLayout->addStretch(1);
        if (materialMissing && services_.project != nullptr) {
            auto *goMaterials = new QToolButton(quickFixRow);
            goMaterials->setText(tr("Malzemelere Git"));
            goMaterials->setAutoRaise(true);
            goMaterials->setToolButtonStyle(Qt::ToolButtonTextOnly);
            goMaterials->setToolTip(tr("Malzeme oluşturmak veya modele atamak için Materials bölümünü aç"));
            goMaterials->setObjectName(QStringLiteral("Dynamics26PreflightGoMaterials"));
            quickFixLayout->addWidget(goMaterials);
            connect(goMaterials, &QToolButton::clicked, this, [this] {
                if (services_.project == nullptr) {
                    return;
                }
                if (auto *mainWindow = qobject_cast<Dynamics26MainWindow *>(window())) {
                    mainWindow->selectObject(services_.project->materialsNode());
                }
            });
        }
        if (!hasActiveSupport) {
            auto *insertSupport = new QToolButton(quickFixRow);
            insertSupport->setText(tr("Mesnet Ekle"));
            insertSupport->setAutoRaise(true);
            insertSupport->setToolButtonStyle(Qt::ToolButtonTextOnly);
            insertSupport->setToolTip(tr("Static Structural analizine Fixed Support ekle"));
            insertSupport->setObjectName(QStringLiteral("Dynamics26PreflightFixSupport"));
            quickFixLayout->addWidget(insertSupport);
            connect(insertSupport, &QToolButton::clicked, this, [this] {
                emit requestCommand(QStringLiteral("analysis.insertSupport"));
            });
        }
        if (!hasActiveLoad) {
            auto *insertForce = new QToolButton(quickFixRow);
            insertForce->setText(tr("Yük Ekle"));
            insertForce->setAutoRaise(true);
            insertForce->setToolButtonStyle(Qt::ToolButtonTextOnly);
            insertForce->setToolTip(tr("Static Structural analizine Force ekle"));
            insertForce->setObjectName(QStringLiteral("Dynamics26PreflightFixForce"));
            quickFixLayout->addWidget(insertForce);
            connect(insertForce, &QToolButton::clicked, this, [this] {
                emit requestCommand(QStringLiteral("analysis.insertForce"));
            });
        }
        validationLayout_->addWidget(quickFixRow);
    }

    const bool canSolve = report.passed();
    const bool outOfDate = record->solved && services_.analysis->solutionIsOutOfDate(analysisId);
    if (record->solveState == SolveState::Solving) {
        status_->setText(tr("Solving"));
    } else if (record->solveState == SolveState::Failed) {
        status_->setText(tr("Solve Failed"));
    } else if (record->solveState == SolveState::Cancelled) {
        status_->setText(tr("Cancelled"));
    } else if (outOfDate) {
        status_->setText(tr("Solved — Out of Date"));
    } else if (record->solved) {
        status_->setText(tr("Solved"));
    } else if (canSolve) {
        status_->setText(tr("Ready to Solve"));
    } else {
        status_->setText(tr("Not Ready"));
    }
    solve_->setEnabled(canSolve && record->solveState != SolveState::Solving);
    solve_->setToolTip(canSolve ? QString() : report.firstFailure());

    const int resultDefinitionCount = record->results.size();
    if (services_.analysis->hasResults(analysisId)) {
        resultAvailability_->setText(outOfDate
                                         ? tr("Calculated — Out of Date · %1 definitions")
                                               .arg(resultDefinitionCount)
                                         : tr("Calculated · %1 definitions")
                                               .arg(resultDefinitionCount));
    } else {
        resultAvailability_->setText(tr("Defined · %1 · not solved")
                                         .arg(resultDefinitionCount));
    }

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
