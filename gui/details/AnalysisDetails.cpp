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

    // Alpha.4 summary yalnız aynı authoritative PreflightReport'u özetler.
    // Yeni bir validation state'i veya paralel kural seti oluşturulmaz. İlk
    // actionable konu önce Failed, sonra Warning sırasıyla seçilir; navigation
    // document mutation değildir ve canonical MainWindow::selectObject yoluna gider.
    const bool materialMissing = services_.materials == nullptr || services_.materials->assigned() == nullptr;
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
                    // Güncel Preflight kontratında subject taşımayan ilk model
                    // blocker malzeme atamasıdır. Otomatik kart/assignment kararı
                    // vermek yerine kullanıcı Materials authoring bağlamına gider.
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

            // Quick-fix yalnız subject engineering type kesin olarak Mesh ise
            // sunulur. Diagnostic label metnine bakılmaz; böylece çeviri/metin
            // değişikliği command routing'i bozmaz. Generate Mesh mevcut shell
            // komutuna gider: timing, selection, dependency refresh ve derived
            // mesh lifecycle tek canonical uygulama yolunda kalır.
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

    // Eksik malzeme ataması bir otomatik-create/assign kararı değildir. Solver
    // modeline hangi malzemenin atanacağı kullanıcı mühendislik kararıdır; bu
    // nedenle Alpha.4 yalnız Materials authoring bağlamına güvenli navigasyon sunar.
    // Mesnet/yük eksikleri ise yeni nesne authoring'i olduğu için mevcut undoable
    // Insert komutlarıyla düzeltilebilir.
    bool hasActiveSupport = false;
    for (const ObjectId id : record->supports) {
        if (services_.project != nullptr && services_.project->object(id) != nullptr
            && !services_.project->isSuppressed(id)) {
            hasActiveSupport = true;
            break;
        }
    }
    bool hasActiveLoad = false;
    for (const ObjectId id : record->loads) {
        if (services_.project != nullptr && services_.project->object(id) != nullptr
            && !services_.project->isSuppressed(id)) {
            hasActiveLoad = true;
            break;
        }
    }
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