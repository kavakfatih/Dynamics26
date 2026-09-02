#include "DetailsPage.h"

#include "../core/UiTheme.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QSpinBox>
#include <QSizePolicy>
#include <QStyle>
#include <QToolButton>
#include <QLayoutItem>
#include <QVBoxLayout>

namespace d26 {
namespace {

// Details yüzeyinin ölçü sistemi. Yoğun ama okunur: 22 px satır, 8 px yatay
// iç boşluk, etiket kolonu sabit oranlı.
constexpr int kRowHeight = 24;
constexpr int kHorizontalPadding = 9;
constexpr int kLabelColumnWidth = 126;

} // namespace

// ---------------------------------------------------------------------------

DetailsRow::DetailsRow(const QString &label, QWidget *value, const bool shaded, QWidget *parent)
    : QWidget(parent), shaded_(shaded)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(kHorizontalPadding, 1, kHorizontalPadding, 1);
    layout->setSpacing(8);

    label_ = new ui::SecondaryLabel(label, 0.62, 0.80, this);
    label_->setObjectName(QStringLiteral("Dynamics26DetailsRowLabel"));
    label_->setFont(ui::compactFont(this));
    label_->setFixedWidth(kLabelColumnWidth);
    // Inspector paneli daraltıldığında uzun engineering etiketleri kesilmez.
    // Sabit label kolonu korunur; yalnız satır gerektiği kadar düşey büyür.
    label_->setWordWrap(true);
    label_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    label_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    layout->addWidget(label_);

    if (value != nullptr) {
        value->setParent(this);
        value->setFont(ui::compactFont(value));
        layout->addWidget(value, 1);
    } else {
        layout->addStretch(1);
    }
    setMinimumHeight(kRowHeight);
}

void DetailsRow::setLabel(const QString &label)
{
    label_->setText(label);
}

void DetailsRow::paintEvent(QPaintEvent *)
{
    if (!shaded_) {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), ui::rowShadeColor());
}

// ---------------------------------------------------------------------------

DetailsSection::DetailsSection(const QString &title, const bool collapsible, const bool collapsed, QWidget *parent)
    : QWidget(parent)
{
    auto *outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto *header = new QWidget(this);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(kHorizontalPadding, 8, kHorizontalPadding, 3);
    headerLayout->setSpacing(4);

    if (collapsible) {
        disclosure_ = new QToolButton(header);
        disclosure_->setObjectName(QStringLiteral("Dynamics26DetailsSectionDisclosure"));
        disclosure_->setAutoRaise(true);
        disclosure_->setCheckable(true);
        disclosure_->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
        disclosure_->setChecked(!collapsed);
        disclosure_->setFixedSize(14, 14);
        // Advanced/Test Data gibi Inspector bölümleri yalnız pointer ile değil,
        // Tab + Space/Enter ile de açılıp kapanabilmelidir. Native Qt focus ring
        // sistem paletinden gelir; özel QSS/focus rengi üretilmez.
        disclosure_->setFocusPolicy(Qt::StrongFocus);
        disclosure_->setAccessibleName(title);
        disclosure_->setAccessibleDescription(collapsed ? tr("Bölümü aç") : tr("Bölümü kapat"));
        disclosure_->setToolTip(collapsed ? tr("Bölümü aç") : tr("Bölümü kapat"));
        headerLayout->addWidget(disclosure_);
    }

    title_ = new ui::SecondaryLabel(title, 0.55, 0.74, header);
    title_->setFont(ui::sectionTitleFont(this));
    headerLayout->addWidget(title_);
    headerLayout->addStretch(1);
    outer->addWidget(header);

    outer->addWidget(new ui::Hairline(this));

    body_ = new QWidget(this);
    rows_ = new QVBoxLayout(body_);
    rows_->setContentsMargins(0, 2, 0, 2);
    rows_->setSpacing(0);
    outer->addWidget(body_);

    if (collapsible) {
        body_->setVisible(!collapsed);
        connect(disclosure_, &QToolButton::toggled, this, [this](const bool expanded) {
            setCollapsed(!expanded);
        });
    }
}

void DetailsSection::setCollapsed(const bool collapsed)
{
    body_->setVisible(!collapsed);
    if (disclosure_ != nullptr) {
        disclosure_->setArrowType(collapsed ? Qt::RightArrow : Qt::DownArrow);
        disclosure_->setAccessibleDescription(collapsed ? tr("Bölümü aç") : tr("Bölümü kapat"));
        disclosure_->setToolTip(collapsed ? tr("Bölümü aç") : tr("Bölümü kapat"));
    }
}

QLabel *DetailsSection::addValueRow(const QString &label, const QString &value)
{
    auto *display = new QLabel(value);
    display->setTextInteractionFlags(Qt::TextSelectableByMouse);
    // Uzun değerler paneli genişletmek yerine satır kaydırır.
    display->setWordWrap(true);
    display->setMinimumWidth(60);
    addRow(label, display);
    return display;
}

DetailsRow *DetailsSection::addRow(const QString &label, QWidget *value)
{
    auto *row = new DetailsRow(label, value, rowCount_ % 2 == 1, body_);
    rows_->addWidget(row);
    ++rowCount_;
    return row;
}

void DetailsSection::addFullWidth(QWidget *widget)
{
    auto *container = new QWidget(body_);
    auto *layout = new QHBoxLayout(container);
    layout->setContentsMargins(kHorizontalPadding, 4, kHorizontalPadding, 4);
    layout->setSpacing(6);
    widget->setParent(container);
    layout->addWidget(widget, 1);
    rows_->addWidget(container);
}

void DetailsSection::addNote(const QString &text)
{
    auto *note = new ui::SecondaryLabel(text, 0.66, 0.80, body_);
    note->setWordWrap(true);
    note->setFont(ui::compactFont(note, -1.5));
    note->setContentsMargins(kHorizontalPadding, 4, kHorizontalPadding, 6);
    rows_->addWidget(note);
}

void DetailsSection::addSeparator()
{
    rows_->addWidget(new ui::Hairline(body_));
}

// ---------------------------------------------------------------------------

DetailsPage::DetailsPage(QWidget *parent) : QWidget(parent)
{
    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(0, 0, 0, 8);
    layout_->setSpacing(0);
}

void DetailsPage::setObject(const quint64 objectId)
{
    objectId_ = objectId;
    refresh();
}

DetailsSection *DetailsPage::addSection(const QString &title, const bool collapsible, const bool collapsed)
{
    auto *section = new DetailsSection(title, collapsible, collapsed, this);
    layout_->addWidget(section);
    return section;
}

void DetailsPage::addStretch()
{
    layout_->addStretch(1);
}

void DetailsPage::clearSections()
{
    while (QLayoutItem *item = layout_->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        delete item;
    }
}

QDoubleSpinBox *DetailsPage::makeDoubleField(const double minimum, const double maximum, const int decimals,
                                             const QString &suffix)
{
    auto *field = new QDoubleSpinBox;
    field->setRange(minimum, maximum);
    field->setDecimals(decimals);
    field->setSuffix(suffix);
    field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    field->setMinimumHeight(22);
    field->setKeyboardTracking(false);
    // Geniş sayı aralıkları sizeHint'i şişirir ve paneli taşırırdı. Yatayda
    // Ignored kullanılarak alan mevcut genişliğe uyar, minimum korunur.
    field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    field->setMinimumWidth(76);
    return field;
}

QSpinBox *DetailsPage::makeIntField(const int minimum, const int maximum)
{
    auto *field = new QSpinBox;
    field->setRange(minimum, maximum);
    field->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    field->setMinimumHeight(22);
    field->setKeyboardTracking(false);
    field->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    field->setMinimumWidth(76);
    return field;
}

QComboBox *DetailsPage::makeCombo(const QStringList &items)
{
    auto *combo = new QComboBox;
    combo->addItems(items);
    combo->setMinimumHeight(22);
    combo->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    combo->setMinimumWidth(76);
    return combo;
}

QLabel *DetailsPage::makeValueLabel(const QString &text)
{
    auto *label = new QLabel(text);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    label->setWordWrap(true);
    label->setMinimumWidth(60);
    return label;
}

QPushButton *DetailsPage::makeActionButton(const QString &text)
{
    auto *button = new QPushButton(text);
    button->setMinimumHeight(24);
    button->setFocusPolicy(Qt::StrongFocus);
    return button;
}

QLabel *DetailsPage::makeNoteLabel(const QString &text)
{
    auto *note = new ui::SecondaryLabel(text, 0.66, 0.80);
    note->setWordWrap(true);
    QFont font = note->font();
    font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.5));
    note->setFont(font);
    return note;
}

} // namespace d26
