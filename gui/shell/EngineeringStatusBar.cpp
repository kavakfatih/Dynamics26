#include "EngineeringStatusBar.h"

#include "../core/UiTheme.h"

#include <QFont>
#include <QLabel>
#include <QLocale>
#include <QToolButton>

namespace d26 {
namespace {

QString grouped(const int value)
{
    // Büyük mesh sayıları binlik ayraçla okunur olmalıdır.
    return QLocale::system().toString(value);
}

} // namespace

EngineeringStatusBar::EngineeringStatusBar(QWidget *parent) : QStatusBar(parent)
{
    setObjectName(QStringLiteral("Dynamics26StatusBar"));
    setSizeGripEnabled(false);

    statistics_ = new QLabel(this);
    selection_ = new ui::SecondaryLabel(QString(), 0.66, 0.80, this);
    solverState_ = new QLabel(this);

    for (QLabel *label : {statistics_, selection_, solverState_}) {
        QFont font = label->font();
        font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
        label->setFont(font);
    }

    documentState_ = new ui::SecondaryLabel(QString(), 0.66, 0.82, this);
    staleWarning_ = new QLabel(this);
    for (QLabel *label : {documentState_, staleWarning_}) {
        QFont font = label->font();
        font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.0));
        label->setFont(font);
    }

    addWidget(statistics_);
    addWidget(selection_, 1);
    addPermanentWidget(staleWarning_);
    addPermanentWidget(documentState_);
    addPermanentWidget(solverState_);

    diagnostics_ = new QToolButton(this);
    diagnostics_->setText(tr("Tanılama"));
    diagnostics_->setCheckable(true);
    diagnostics_->setAutoRaise(true);
    diagnostics_->setFocusPolicy(Qt::NoFocus);
    QFont diagnosticsFont = diagnostics_->font();
    diagnosticsFont.setPointSizeF(qMax(9.0, diagnosticsFont.pointSizeF() - 1.0));
    diagnostics_->setFont(diagnosticsFont);
    addPermanentWidget(diagnostics_);
    connect(diagnostics_, &QToolButton::toggled, this, &EngineeringStatusBar::diagnosticsToggled);

    setModelStatistics(0, 0, 0, false);
    setSelection(QString());
    setSolverState(SolverState::Idle);
}

void EngineeringStatusBar::setModelStatistics(const int bodyCount, const int elementCount, const int dofCount,
                                              const bool hasMesh)
{
    QStringList parts;
    parts << tr("%n body", "", bodyCount);
    if (hasMesh) {
        parts << tr("%1 HEX8").arg(grouped(elementCount));
        parts << tr("%1 DOF").arg(grouped(dofCount));
    } else {
        parts << tr("mesh yok");
    }
    statistics_->setText(parts.join(QStringLiteral("  •  ")));
}

void EngineeringStatusBar::setSelection(const QString &text)
{
    selection_->setText(text);
}

void EngineeringStatusBar::setSolverState(const SolverState state, const QString &detail)
{
    QString text;
    switch (state) {
    case SolverState::Idle:     text = tr("Hazır"); break;
    case SolverState::NotReady: text = tr("Çözüme hazır değil"); break;
    case SolverState::Ready:    text = tr("Çözüme hazır"); break;
    case SolverState::Solving:  text = tr("Çözülüyor…"); break;
    case SolverState::Solved:   text = tr("Çözüldü"); break;
    case SolverState::Failed:   text = tr("Çözüm başarısız"); break;
    }
    solverState_->setText(detail.isEmpty() ? text : QStringLiteral("%1 · %2").arg(detail, text));
}

void EngineeringStatusBar::setDocumentState(const bool dirty, const QString &staleWarning)
{
    documentState_->setText(dirty ? tr("Düzenlendi") : QString());
    staleWarning_->setText(staleWarning);
    QPalette palette = staleWarning_->palette();
    const QColor color = ui::statusColor(ui::StatusTone::OutOfDate);
    palette.setColor(QPalette::WindowText, color);
    palette.setColor(QPalette::Text, color);
    staleWarning_->setPalette(palette);
}

void EngineeringStatusBar::setDiagnosticsChecked(const bool checked)
{
    QSignalBlocker blocker(diagnostics_);
    diagnostics_->setChecked(checked);
}

} // namespace d26
