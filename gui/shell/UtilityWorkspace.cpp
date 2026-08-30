#include "UtilityWorkspace.h"

#include <QDateTime>
#include <QFont>
#include <QFontDatabase>
#include <QHeaderView>
#include <QPlainTextEdit>
#include <QTabWidget>
#include <QTableWidget>
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
    solverOutput_ = makeConsole(tabs_);
    convergence_ = makeTable({tr("Attempt"), tr("Iter"), tr("Load Factor"), tr("Rel. |R|"), tr("Rel. Δu"),
                              QStringLiteral("α"), tr("Durum")},
                             tabs_);
    results_ = makeTable({tr("Sonuç"), tr("Değer")}, tabs_);
    timings_ = makeTable({tr("İşlem"), tr("Süre")}, tabs_);

    tabs_->addTab(messages_, tr("Messages"));
    tabs_->addTab(convergence_, tr("Convergence"));
    tabs_->addTab(solverOutput_, tr("Solver Output"));
    tabs_->addTab(results_, tr("Results Table"));
    tabs_->addTab(timings_, tr("Timings"));
    layout->addWidget(tabs_);
}

void UtilityWorkspace::appendMessage(const QString &text, const Severity severity)
{
    messages_->appendPlainText(QStringLiteral("%1 %2%3")
                                   .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")),
                                        severityTag(severity), text));
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

void UtilityWorkspace::setConvergenceRows(const QVector<QStringList> &rows)
{
    convergence_->setRowCount(static_cast<int>(rows.size()));
    for (int row = 0; row < rows.size(); ++row) {
        const QStringList &values = rows.at(row);
        for (int column = 0; column < values.size() && column < convergence_->columnCount(); ++column) {
            convergence_->setItem(row, column, new QTableWidgetItem(values.at(column)));
        }
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
    messages_->clear();
    solverOutput_->clear();
    convergence_->setRowCount(0);
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
