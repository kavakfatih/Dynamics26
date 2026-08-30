#include "GraphicsWorkspace.h"

#include "../core/CaeIcons.h"
#include "../core/UiTheme.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QToolBar>
#include <QVBoxLayout>

namespace d26 {

GraphicsWorkspace::GraphicsWorkspace(QWidget *parent) : QFrame(parent)
{
    setObjectName(QStringLiteral("Dynamics26GraphicsWorkspace"));
    setFrameShape(QFrame::NoFrame);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    toolbar_ = new QToolBar(this);
    toolbar_->setObjectName(QStringLiteral("Dynamics26GraphicsToolbar"));
    toolbar_->setMovable(false);
    toolbar_->setFloatable(false);
    toolbar_->setIconSize(QSize(16, 16));
    toolbar_->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolbar_->setContentsMargins(0, 0, 0, 0);

    auto *filterGroup = new QActionGroup(this);
    filterGroup->setExclusive(true);
    selectBody_ = toolbar_->addAction(tr("Body"));
    selectBody_->setCheckable(true);
    selectBody_->setChecked(true);
    selectBody_->setToolTip(tr("Gövde seçimi"));
    filterGroup->addAction(selectBody_);
    selectFace_ = toolbar_->addAction(tr("Face"));
    selectFace_->setCheckable(true);
    selectFace_->setToolTip(tr("Yüz seçimi"));
    filterGroup->addAction(selectFace_);
    toolbar_->addSeparator();
    fit_ = toolbar_->addAction(tr("Fit"));
    fit_->setToolTip(tr("Görünümü sığdır"));
    isometric_ = toolbar_->addAction(tr("Isometric"));
    isometric_->setToolTip(tr("İzometrik görünüm"));
    toolbar_->addSeparator();

    selectionLabel_ = new ui::SecondaryLabel(QString(), 0.68, 0.82, toolbar_);
    QFont compact = selectionLabel_->font();
    compact.setPointSizeF(qMax(9.0, compact.pointSizeF() - 1.5));
    selectionLabel_->setFont(compact);
    selectionLabel_->setContentsMargins(4, 0, 4, 0);
    toolbar_->addWidget(selectionLabel_);

    auto *spacer = new QWidget(toolbar_);
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar_->addWidget(spacer);

    contextLabel_ = new ui::SecondaryLabel(QString(), 0.52, 0.72, toolbar_);
    QFont contextFont = contextLabel_->font();
    contextFont.setPointSizeF(qMax(9.0, contextFont.pointSizeF() - 2.0));
    contextFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    contextLabel_->setFont(contextFont);
    contextLabel_->setContentsMargins(6, 0, 10, 0);
    toolbar_->addWidget(contextLabel_);

    layout->addWidget(toolbar_);

    layout->addWidget(new ui::Hairline(this));

    viewport_ = new ViewportWidget(this);
    layout->addWidget(viewport_, 1);

    refreshIcons();

    connect(fit_, &QAction::triggered, this, &GraphicsWorkspace::fitViewRequested);
    connect(isometric_, &QAction::triggered, this, &GraphicsWorkspace::isometricViewRequested);
    connect(selectBody_, &QAction::triggered, this, [this] {
        filter_ = SelectionFilter::Body;
        emit selectionFilterChanged(filter_);
    });
    connect(selectFace_, &QAction::triggered, this, [this] {
        filter_ = SelectionFilter::Face;
        emit selectionFilterChanged(filter_);
    });

    setFaceSelectionAvailable(false);
}

void GraphicsWorkspace::refreshIcons()
{
    const QColor tint = qApp->palette().color(QPalette::WindowText);
    selectBody_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectBody, tint));
    selectFace_->setIcon(CaeIcons::forCommand(CommandGlyph::SelectFace, tint));
    fit_->setIcon(CaeIcons::forCommand(CommandGlyph::FitView, tint));
    isometric_->setIcon(CaeIcons::forCommand(CommandGlyph::Isometric, tint));
}

void GraphicsWorkspace::setContextLabel(const QString &text)
{
    contextLabel_->setText(text.toUpper());
}

void GraphicsWorkspace::setSelectionLabel(const QString &text)
{
    selectionLabel_->setText(text);
}

void GraphicsWorkspace::setFaceSelectionAvailable(const bool available)
{
    selectFace_->setEnabled(available);
    selectFace_->setToolTip(available ? tr("Yüz seçimi")
                                      : tr("Yüz seçimi mesh üretildikten sonra kullanılabilir."));
    if (!available && filter_ == SelectionFilter::Face) {
        filter_ = SelectionFilter::Body;
        selectBody_->setChecked(true);
        emit selectionFilterChanged(filter_);
    }
}

} // namespace d26
