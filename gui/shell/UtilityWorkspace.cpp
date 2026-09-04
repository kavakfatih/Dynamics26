#include "UtilityWorkspace.h"

#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QHeaderView>
#include <QLabel>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

namespace d26 {
namespace {

QTableWidget *makeTable(const QStringList &headers, QWidget *parent)
{
    auto *table = new QTableWidget(0, static_cast<int>(headers.size()), parent);
    table->setHorizontalHeaderLabels(headers);
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->verticalHeader()->setDefaultSectionSize(20);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    table->setFrameShape(QFrame::NoFrame);
    return table;
}

QPlainTextEdit *makeConsole(QWidget *parent)
{
    auto *console = new QPlainTextEdit(parent);
    console->setReadOnly(true);
    console->setFrameShape(QFrame::NoFrame);
    console->setMaximumBlockCount(5000);
    console->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    return console;
}

QString severityTag(const Severity severity)
{
    switch (severity) {
    case Severity::Info:    return QStringLiteral("[bilgi]   ");
    case Severity::Success: return QStringLiteral("[tamam]   ");
    case Severity::Warning: return QStringLiteral("[uyarı]   ");
    case Severity::Error:   return QStringLiteral("[HATA]    ");
    }
    return {};
}

QString convergenceStateText(const SolverConvergenceState state)
{
    switch (state) {
    case SolverConvergenceState::Unavailable: return QStringLiteral("Unavailable");
    case SolverConvergenceState::Running:     return QStringLiteral("Running");
    case SolverConvergenceState::Completed:   return QStringLiteral("Completed");
    case SolverConvergenceState::Converged:   return QStringLiteral("Converged");
    case SolverConvergenceState::Failed:      return QStringLiteral("Failed");
    }
    return QStringLiteral("Unavailable");
}

QString optionalNumber(const std::optional<double> &value)
{
    return value.has_value() ? QString::number(*value, 'g', 8) : QStringLiteral("—");
}

QString optionalInteger(const std::optional<int> &value)
{
    return value.has_value() ? QString::number(*value) : QStringLiteral("—");
}

QString adaptiveEventText(const SolverAdaptiveEvent event)
{
    switch (event) {
    case SolverAdaptiveEvent::Unavailable: return QStringLiteral("—");
    case SolverAdaptiveEvent::None:        return QStringLiteral("—");
    case SolverAdaptiveEvent::Growth:      return QStringLiteral("Growth");
    case SolverAdaptiveEvent::Cutback:     return QStringLiteral("Cutback");
    }
    return QStringLiteral("—");
}

QString criterionText(const SolverCriterionState residual,
                      const SolverCriterionState displacement)
{
    const auto symbol = [](const SolverCriterionState state) {
        switch (state) {
        case SolverCriterionState::Satisfied:   return QStringLiteral("✓");
        case SolverCriterionState::Unsatisfied: return QStringLiteral("✕");
        case SolverCriterionState::Unavailable: return QStringLiteral("—");
        }
        return QStringLiteral("—");
    };
    if (residual == SolverCriterionState::Unavailable
        && displacement == SolverCriterionState::Unavailable) {
        return QStringLiteral("—");
    }
    return QStringLiteral("R %1 • Δu %2").arg(symbol(residual), symbol(displacement));
}

QString availabilityText(const SolverMetricAvailability availability)
{
    return availability == SolverMetricAvailability::Available
        ? QStringLiteral("Available")
        : QStringLiteral("Unavailable");
}

ObjectId subjectFromItem(const QTableWidgetItem *item)
{
    if (item == nullptr) {
        return InvalidObjectId;
    }
    bool ok = false;
    const qulonglong value = item->data(Qt::UserRole).toString().toULongLong(&ok, 10);
    return ok ? static_cast<ObjectId>(value) : InvalidObjectId;
}

bool isLegacyPreflightCheckEcho(const QString &text)
{
    return text.startsWith(QStringLiteral("✓ "))
        || text.startsWith(QStringLiteral("! "))
        || text.startsWith(QStringLiteral("✕ "));
}

} // namespace

UtilityWorkspace::UtilityWorkspace(QWidget *parent) : QWidget(parent)
{
    setObjectName(QStringLiteral("Dynamics26UtilityWorkspace"));
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabs_ = new QTabWidget(this);
    tabs_->setDocumentMode(true);
    tabs_->setObjectName(QStringLiteral("Dynamics26UtilityTabs"));

    messages_ = makeConsole(tabs_);
    messages_->setObjectName(QStringLiteral("Dynamics26UtilityMessages"));
    preflight_ = makeTable({tr("Durum"), tr("Kontrol"), tr("Açıklama"), tr("Nesne / Göster")}, tabs_);
    preflight_->setObjectName(QStringLiteral("Dynamics26UtilityPreflight"));
    preflight_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    preflight_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    preflight_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);
    preflight_->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    solverOutput_ = makeConsole(tabs_);

    auto *convergencePage = new QWidget(tabs_);
    convergencePage->setObjectName(QStringLiteral("Dynamics26UtilityConvergencePage"));
    auto *convergenceLayout = new QVBoxLayout(convergencePage);
    convergenceLayout->setContentsMargins(8, 6, 8, 6);
    convergenceLayout->setSpacing(6);

    convergenceSummary_ = new QLabel(tr("Yakınsama verisi yok."), convergencePage);
    convergenceSummary_->setObjectName(QStringLiteral("Dynamics26UtilityConvergenceSummary"));
    convergenceSummary_->setWordWrap(true);
    convergenceSummary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    convergence_ = makeTable({tr("Attempt"), tr("Iter"), tr("Load Factor"), tr("Rel. |R|"), tr("Rel. Δu"),
                              QStringLiteral("α"), tr("Durum")}, convergencePage);
    convergence_->setObjectName(QStringLiteral("Dynamics26UtilityConvergence"));

    diagnosticsSummary_ = new QLabel(tr("Advanced diagnostics: Unavailable"), convergencePage);
    diagnosticsSummary_->setObjectName(QStringLiteral("Dynamics26UtilityConvergenceDiagnosticsSummary"));
    diagnosticsSummary_->setWordWrap(true);
    diagnosticsSummary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    diagnostics_ = makeTable({tr("Attempt"), tr("Iter"), QStringLiteral("Δλ"), QStringLiteral("|R|"),
                              QStringLiteral("|Δu|"), tr("min J"), tr("Adaptive"), tr("Criteria")},
                             convergencePage);
    diagnostics_->setObjectName(QStringLiteral("Dynamics26UtilityConvergenceDiagnostics"));

    coupledDiagnosticsSummary_ = new QLabel(tr("Coupled / Contact diagnostics: Unavailable"), convergencePage);
    coupledDiagnosticsSummary_->setObjectName(QStringLiteral("Dynamics26UtilityCoupledDiagnosticsSummary"));
    coupledDiagnosticsSummary_->setWordWrap(true);
    coupledDiagnosticsSummary_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    coupledDiagnostics_ = makeTable({tr("Attempt"), tr("Iter"), QStringLiteral("|Rp|"), tr("Rel. Rp"),
                                     QStringLiteral("|Δp|"), tr("Active"), tr("Stick"), tr("Slip"),
                                     tr("Penetration")}, convergencePage);
    coupledDiagnostics_->setObjectName(QStringLiteral("Dynamics26UtilityCoupledDiagnostics"));

    convergenceLayout->addWidget(convergenceSummary_);
    convergenceLayout->addWidget(convergence_, 2);
    convergenceLayout->addWidget(diagnosticsSummary_);
    convergenceLayout->addWidget(diagnostics_, 1);
    convergenceLayout->addWidget(coupledDiagnosticsSummary_);
    convergenceLayout->addWidget(coupledDiagnostics_, 1);

    results_ = makeTable({tr("Sonuç"), tr("Değer")}, tabs_);
    timings_ = makeTable({tr("İşlem"), tr("Süre")}, tabs_);

    tabs_->addTab(messages_, tr("Messages"));
    tabs_->addTab(preflight_, tr("Preflight"));
    tabs_->addTab(convergencePage, tr("Convergence"));
    tabs_->addTab(solverOutput_, tr("Solver Output"));
    tabs_->addTab(results_, tr("Results Table"));
    tabs_->addTab(timings_, tr("Timings"));
    layout->addWidget(tabs_);

    // B2.6 closeout: Solver/Utility Workspace accessibility sahibi bu widget'tır.
    // main.cpp üzerinden sonradan UI patch'i yapılmaz. Convergence tabı ve üç
    // diagnostics tablosu explicit keyboard focus alır; özet/tablo yüzeyleri
    // assistive technology için kararlı isim ve açıklama taşır.
    tabs_->setFocusPolicy(Qt::StrongFocus);
    tabs_->setAccessibleName(tr("Utility workspace tabs"));
    tabs_->setAccessibleDescription(
        tr("Messages, Preflight, Convergence, Solver Output, Results Table ve Timings sekmeleri arasında klavyeyle gezinmeyi sağlar."));
    convergencePage->setAccessibleName(tr("Convergence workspace"));

    convergenceSummary_->setAccessibleName(tr("Convergence summary"));
    convergenceSummary_->setAccessibleDescription(
        tr("Geçerli solver oturumunun yakınsama durumunu ve özet metriklerini gösterir."));
    convergence_->setFocusPolicy(Qt::StrongFocus);
    convergence_->setAccessibleName(tr("Convergence iteration history"));
    convergence_->setAccessibleDescription(
        tr("Solver yakınsama iterasyonlarını ve temel yakınsama metriklerini satır bazında gösterir."));

    diagnosticsSummary_->setAccessibleName(tr("Advanced convergence diagnostics summary"));
    diagnosticsSummary_->setAccessibleDescription(
        tr("Mevcut olduğunda ileri nonlinear yakınsama metriklerinin özetini gösterir."));
    diagnostics_->setFocusPolicy(Qt::StrongFocus);
    diagnostics_->setAccessibleName(tr("Advanced convergence diagnostics table"));
    diagnostics_->setAccessibleDescription(
        tr("Mevcut olduğunda load increment, residual, displacement increment, minimum J ve criterion metriklerini gösterir."));

    coupledDiagnosticsSummary_->setAccessibleName(tr("Coupled and contact diagnostics summary"));
    coupledDiagnosticsSummary_->setAccessibleDescription(
        tr("Mevcut verification telemetry için mixed u-p ve Contact diagnostics özetini gösterir."));
    coupledDiagnostics_->setFocusPolicy(Qt::StrongFocus);
    coupledDiagnostics_->setAccessibleName(tr("Coupled and contact diagnostics table"));
    coupledDiagnostics_->setAccessibleDescription(
        tr("Mevcut verification telemetry için pressure residual ve Contact durum metriklerini gösterir."));

    const auto activateSubject = [this](const int row, const int column) {
        if (column != 3 || row < 0 || row >= preflight_->rowCount()) {
            return;
        }
        const ObjectId subject = subjectFromItem(preflight_->item(row, 3));
        if (subject != InvalidObjectId) {
            emit preflightSubjectActivated(subject);
        }
    };
    connect(preflight_, &QTableWidget::cellClicked, this, activateSubject);
    connect(preflight_, &QTableWidget::cellActivated, this, activateSubject);
}

void UtilityWorkspace::appendMessage(const QString &text, const Severity severity)
{
    if (text == QStringLiteral("── PRE-FLIGHT ──")) {
        suppressingPreflightEcho_ = true;
        return;
    }
    if (suppressingPreflightEcho_) {
        if (isLegacyPreflightCheckEcho(text)) {
            return;
        }
        suppressingPreflightEcho_ = false;
    }

    messages_->appendPlainText(QStringLiteral("%1 %2%3")
                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                                        severityTag(severity), text));
}

void UtilityWorkspace::setPreflightRows(const QVector<PreflightUtilityRow> &rows)
{
    preflight_->setRowCount(static_cast<int>(rows.size()));
    for (int row = 0; row < rows.size(); ++row) {
        const PreflightUtilityRow &value = rows.at(row);
        preflight_->setItem(row, 0, new QTableWidgetItem(value.status));
        preflight_->setItem(row, 1, new QTableWidgetItem(value.label));
        preflight_->setItem(row, 2, new QTableWidgetItem(value.detail));

        auto *subject = new QTableWidgetItem;
        if (value.subject != InvalidObjectId) {
            subject->setText(value.subjectLabel.isEmpty()
                                 ? tr("İlgili nesneyi göster ↗")
                                 : tr("%1 ↗").arg(value.subjectLabel));
            subject->setData(Qt::UserRole, QString::number(static_cast<qulonglong>(value.subject)));
            subject->setToolTip(tr("Model ağacında ilgili nesneyi göster"));
        } else {
            subject->setText(QStringLiteral("—"));
        }
        preflight_->setItem(row, 3, subject);
    }
    preflight_->resizeRowsToContents();
}

void UtilityWorkspace::appendSolverOutput(const QString &text)
{
    solverOutput_->appendPlainText(text);
}

void UtilityWorkspace::clearSolverOutput()
{
    solverOutput_->clear();
}

void UtilityWorkspace::setResultRows(const QVector<QPair<QString, QString>> &rows)
{
    results_->setRowCount(static_cast<int>(rows.size()));
    for (int row = 0; row < rows.size(); ++row) {
        results_->setItem(row, 0, new QTableWidgetItem(rows.at(row).first));
        results_->setItem(row, 1, new QTableWidgetItem(rows.at(row).second));
    }
    results_->resizeColumnToContents(0);
}

void UtilityWorkspace::setConvergenceData(const SolverConvergenceSnapshot &snapshot)
{
    const int rowCount = static_cast<int>(snapshot.entries.size());
    convergence_->setRowCount(rowCount);
    for (int row = 0; row < rowCount; ++row) {
        const SolverConvergenceEntry &entry = snapshot.entries.at(row);
        convergence_->setItem(row, 0, new QTableWidgetItem(QString::number(entry.attempt)));
        convergence_->setItem(row, 1, new QTableWidgetItem(QString::number(entry.iteration)));
        convergence_->setItem(row, 2, new QTableWidgetItem(QString::number(entry.loadFactor, 'g', 8)));
        convergence_->setItem(row, 3, new QTableWidgetItem(QString::number(entry.relativeResidual, 'g', 8)));
        convergence_->setItem(row, 4, new QTableWidgetItem(QString::number(entry.relativeDisplacement, 'g', 8)));
        convergence_->setItem(row, 5, new QTableWidgetItem(QString::number(entry.lineSearchAlpha, 'g', 8)));
        convergence_->setItem(row, 6, new QTableWidgetItem(entry.converged ? tr("Converged") : tr("Iterating")));
    }
    convergence_->resizeRowsToContents();

    QVector<const SolverConvergenceEntry *> advancedRows;
    QVector<const SolverConvergenceEntry *> coupledRows;
    advancedRows.reserve(rowCount);
    coupledRows.reserve(rowCount);
    for (const SolverConvergenceEntry &entry : snapshot.entries) {
        const bool hasAdvanced = entry.loadIncrement.has_value()
            || entry.residualNorm.has_value()
            || entry.displacementIncrementNorm.has_value()
            || entry.minimumJacobian.has_value()
            || entry.adaptiveEvent != SolverAdaptiveEvent::Unavailable
            || entry.residualCriterion != SolverCriterionState::Unavailable
            || entry.displacementCriterion != SolverCriterionState::Unavailable;
        if (hasAdvanced) {
            advancedRows.push_back(&entry);
        }
        const bool hasCoupled = entry.pressureResidualNorm.has_value()
            || entry.relativePressureResidual.has_value()
            || entry.pressureIncrementNorm.has_value()
            || entry.activeContactCount.has_value()
            || entry.stickContactCount.has_value()
            || entry.slipContactCount.has_value()
            || entry.maximumPenetration.has_value();
        if (hasCoupled) {
            coupledRows.push_back(&entry);
        }
    }

    diagnostics_->setRowCount(static_cast<int>(advancedRows.size()));
    for (int row = 0; row < advancedRows.size(); ++row) {
        const SolverConvergenceEntry &entry = *advancedRows.at(row);
        diagnostics_->setItem(row, 0, new QTableWidgetItem(QString::number(entry.attempt)));
        diagnostics_->setItem(row, 1, new QTableWidgetItem(QString::number(entry.iteration)));
        diagnostics_->setItem(row, 2, new QTableWidgetItem(optionalNumber(entry.loadIncrement)));
        diagnostics_->setItem(row, 3, new QTableWidgetItem(optionalNumber(entry.residualNorm)));
        diagnostics_->setItem(row, 4, new QTableWidgetItem(optionalNumber(entry.displacementIncrementNorm)));
        diagnostics_->setItem(row, 5, new QTableWidgetItem(optionalNumber(entry.minimumJacobian)));
        diagnostics_->setItem(row, 6, new QTableWidgetItem(adaptiveEventText(entry.adaptiveEvent)));
        diagnostics_->setItem(row, 7, new QTableWidgetItem(
            criterionText(entry.residualCriterion, entry.displacementCriterion)));
    }
    diagnostics_->resizeRowsToContents();

    coupledDiagnostics_->setRowCount(static_cast<int>(coupledRows.size()));
    for (int row = 0; row < coupledRows.size(); ++row) {
        const SolverConvergenceEntry &entry = *coupledRows.at(row);
        coupledDiagnostics_->setItem(row, 0, new QTableWidgetItem(QString::number(entry.attempt)));
        coupledDiagnostics_->setItem(row, 1, new QTableWidgetItem(QString::number(entry.iteration)));
        coupledDiagnostics_->setItem(row, 2, new QTableWidgetItem(optionalNumber(entry.pressureResidualNorm)));
        coupledDiagnostics_->setItem(row, 3, new QTableWidgetItem(optionalNumber(entry.relativePressureResidual)));
        coupledDiagnostics_->setItem(row, 4, new QTableWidgetItem(optionalNumber(entry.pressureIncrementNorm)));
        coupledDiagnostics_->setItem(row, 5, new QTableWidgetItem(optionalInteger(entry.activeContactCount)));
        coupledDiagnostics_->setItem(row, 6, new QTableWidgetItem(optionalInteger(entry.stickContactCount)));
        coupledDiagnostics_->setItem(row, 7, new QTableWidgetItem(optionalInteger(entry.slipContactCount)));
        coupledDiagnostics_->setItem(row, 8, new QTableWidgetItem(optionalNumber(entry.maximumPenetration)));
    }
    coupledDiagnostics_->resizeRowsToContents();

    if (snapshot.summary.state == SolverConvergenceState::Unavailable && snapshot.entries.isEmpty()) {
        convergenceSummary_->setText(tr("Yakınsama verisi yok."));
        diagnosticsSummary_->setText(tr("Advanced diagnostics: Unavailable"));
        coupledDiagnosticsSummary_->setText(tr("Coupled / Contact diagnostics: Unavailable"));
        return;
    }

    if (snapshot.summary.executionMode == SolverExecutionMode::DirectLinear) {
        convergenceSummary_->setText(
            tr("Durum: %1 | Direct solve | Newton history: not applicable")
                .arg(convergenceStateText(snapshot.summary.state)));
        diagnosticsSummary_->setText(tr("Advanced diagnostics: Direct solve için uygulanmaz."));
        coupledDiagnosticsSummary_->setText(tr("Coupled / Contact diagnostics: Direct solve için uygulanmaz."));
        diagnostics_->setRowCount(0);
        coupledDiagnostics_->setRowCount(0);
        return;
    }

    const bool nonlinearHistory = snapshot.summary.executionMode == SolverExecutionMode::NonlinearNewton
        || (snapshot.summary.executionMode == SolverExecutionMode::Unavailable && !snapshot.entries.isEmpty());
    if (!nonlinearHistory) {
        convergenceSummary_->setText(tr("Yakınsama verisi yok."));
        diagnosticsSummary_->setText(tr("Advanced diagnostics: Unavailable"));
        coupledDiagnosticsSummary_->setText(tr("Coupled / Contact diagnostics: Unavailable"));
        return;
    }

    convergenceSummary_->setText(
        tr("Durum: %1 | Reason: %2 | λ = %3 | Son denenen λ = %4 | Son Δλ = %5 | "
           "Kabul edilen adım = %6 | Attempt = %7 | Newton iterasyonu = %8 | "
           "Cutback = %9 | Final residual norm = %10")
            .arg(convergenceStateText(snapshot.summary.state))
            .arg(QString::fromLatin1(
                nonlinearTerminationReasonName(snapshot.summary.terminationReason)))
            .arg(snapshot.summary.completedLoadFactor, 0, 'g', 8)
            .arg(snapshot.summary.lastAttemptedLoadFactor, 0, 'g', 8)
            .arg(snapshot.summary.lastLoadIncrement, 0, 'g', 8)
            .arg(snapshot.summary.acceptedSteps)
            .arg(snapshot.summary.stepAttempts)
            .arg(snapshot.summary.totalIterations)
            .arg(snapshot.summary.cutbackCount)
            .arg(snapshot.summary.finalResidualNorm, 0, 'g', 8));

    if (advancedRows.isEmpty()) {
        diagnosticsSummary_->setText(
            tr("Advanced diagnostics: Bu telemetry kaynağında Unavailable. Mixed u-p: %1 | Contact: %2")
                .arg(availabilityText(snapshot.summary.pressureMetrics),
                     availabilityText(snapshot.summary.contactMetrics)));
    } else {
        const QString minimumJ = snapshot.summary.minimumJacobian.has_value()
            ? QString::number(*snapshot.summary.minimumJacobian, 'g', 8)
            : QStringLiteral("Unavailable");
        diagnosticsSummary_->setText(
            tr("Advanced diagnostics: min J = %1 | Mixed u-p: %2 | Contact: %3")
                .arg(minimumJ,
                     availabilityText(snapshot.summary.pressureMetrics),
                     availabilityText(snapshot.summary.contactMetrics)));
    }

    if (snapshot.summary.pressureMetrics == SolverMetricAvailability::Available) {
        coupledDiagnosticsSummary_->setText(
            tr("Mixed u-p verification: Available | Final |Rp| = %1 | Contact: %2")
                .arg(optionalNumber(snapshot.summary.finalPressureResidualNorm),
                     availabilityText(snapshot.summary.contactMetrics)));
    } else if (snapshot.summary.contactMetrics == SolverMetricAvailability::Available) {
        coupledDiagnosticsSummary_->setText(
            tr("Contact verification: Available | Active = %1 | Stick = %2 | Slip = %3 | Max penetration = %4 | Normal force = %5 | Mixed u-p: %6")
                .arg(optionalInteger(snapshot.summary.finalActiveContactCount),
                     optionalInteger(snapshot.summary.finalStickContactCount),
                     optionalInteger(snapshot.summary.finalSlipContactCount),
                     optionalNumber(snapshot.summary.maximumPenetration),
                     optionalNumber(snapshot.summary.totalContactNormalForce),
                     availabilityText(snapshot.summary.pressureMetrics)));
    } else {
        coupledDiagnosticsSummary_->setText(
            tr("Coupled / Contact diagnostics: Mixed u-p: Unavailable | Contact: Unavailable"));
        coupledDiagnostics_->setRowCount(0);
    }
}

void UtilityWorkspace::appendTiming(const QString &operation, const double seconds)
{
    const int row = timings_->rowCount();
    timings_->insertRow(row);
    timings_->setItem(row, 0, new QTableWidgetItem(operation));
    timings_->setItem(row, 1, new QTableWidgetItem(QStringLiteral("%1 s").arg(seconds, 0, 'f', 3)));
    timings_->resizeColumnToContents(0);
}

void UtilityWorkspace::clearAll()
{
    suppressingPreflightEcho_ = false;
    messages_->clear();
    preflight_->setRowCount(0);
    solverOutput_->clear();
    convergence_->setRowCount(0);
    convergenceSummary_->setText(tr("Yakınsama verisi yok."));
    diagnostics_->setRowCount(0);
    diagnosticsSummary_->setText(tr("Advanced diagnostics: Unavailable"));
    coupledDiagnostics_->setRowCount(0);
    coupledDiagnosticsSummary_->setText(tr("Coupled / Contact diagnostics: Unavailable"));
    results_->setRowCount(0);
    timings_->setRowCount(0);
}

void UtilityWorkspace::showTab(const Tab tab)
{
    tabs_->setCurrentIndex(static_cast<int>(tab));
}

void UtilityWorkspace::noteUserDismissed()
{
    userDismissed_ = true;
}

} // namespace d26
