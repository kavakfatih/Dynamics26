#include "UiTheme.h"

#include <QApplication>
#include <QEvent>
#include <QPainter>

namespace d26::ui {

QPalette applicationPalette()
{
    // Tek doğruluk kaynağı: uygulamanın native (macOS System Appearance'tan
    // gelen) paleti. Widget'ların kendi paletleri türetme girdisi olarak
    // KULLANILMAZ — bkz. başlıktaki stale foreground notu.
    return QApplication::palette();
}

bool isDark(const QPalette &palette)
{
    return palette.color(QPalette::Window).lightnessF() < 0.5;
}

bool isDarkAppearance()
{
    return isDark(applicationPalette());
}

QColor secondaryText(const double lightAlpha, const double darkAlpha)
{
    const QPalette base = applicationPalette();
    QColor color = base.color(QPalette::WindowText);
    color.setAlphaF(static_cast<float>(isDark(base) ? darkAlpha : lightAlpha));
    return color;
}

QColor hairlineColor()
{
    const QPalette base = applicationPalette();
    QColor color = base.color(QPalette::WindowText);
    color.setAlphaF(isDark(base) ? 0.24f : 0.16f);
    return color;
}

QColor rowShadeColor()
{
    // Yoğun özellik tablosunda satır ayrımı için çok hafif bir gölge.
    const QPalette base = applicationPalette();
    QColor color = base.color(QPalette::WindowText);
    color.setAlphaF(isDark(base) ? 0.060f : 0.038f);
    return color;
}

QColor statusColor(const StatusTone tone)
{
    const bool dark = isDarkAppearance();
    switch (tone) {
    case StatusTone::Neutral:    return dark ? QColor(0x8a, 0x8a, 0x92) : QColor(0x9a, 0x9a, 0xa0);
    case StatusTone::Ready:      return dark ? QColor(0x5c, 0x9d, 0xf5) : QColor(0x2f, 0x6f, 0xd0);
    case StatusTone::UpToDate:   return dark ? QColor(0x4c, 0xc0, 0x6a) : QColor(0x2e, 0x8b, 0x49);
    case StatusTone::OutOfDate:  return dark ? QColor(0xf0, 0xa8, 0x35) : QColor(0xc2, 0x7c, 0x10);
    case StatusTone::Warning:    return dark ? QColor(0xf0, 0xa8, 0x35) : QColor(0xc2, 0x7c, 0x10);
    case StatusTone::Error:      return dark ? QColor(0xf2, 0x60, 0x59) : QColor(0xbe, 0x33, 0x2d);
    case StatusTone::Suppressed: return dark ? QColor(0x6e, 0x6e, 0x76) : QColor(0xb0, 0xb0, 0xb6);
    case StatusTone::Solving:    return dark ? QColor(0x5c, 0x9d, 0xf5) : QColor(0x2f, 0x6f, 0xd0);
    }
    return {};
}

SecondaryLabel::SecondaryLabel(const QString &text, const double lightAlpha, const double darkAlpha, QWidget *parent)
    : QLabel(text, parent), lightAlpha_(lightAlpha), darkAlpha_(darkAlpha)
{
    applyColor();
}

void SecondaryLabel::applyColor()
{
    if (applying_) {
        return; // setPalette() yeni bir PaletteChange üretir; özyineleme engellenir
    }
    applying_ = true;
    const QColor color = secondaryText(lightAlpha_, darkAlpha_);
    QPalette labelPalette = palette();
    // QLabel bağlama göre WindowText veya Text rolünü çözer; ikisi de yazılır.
    labelPalette.setColor(QPalette::WindowText, color);
    labelPalette.setColor(QPalette::Text, color);
    labelPalette.setColor(QPalette::ButtonText, color);
    QLabel::setPalette(labelPalette);
    applying_ = false;
}

void SecondaryLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);
    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange) {
        applyColor();
    }
}

Hairline::Hairline(QWidget *parent, const Qt::Orientation orientation) : QFrame(parent)
{
    if (orientation == Qt::Horizontal) {
        setFixedHeight(1);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else {
        setFixedWidth(1);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    setFrameShape(QFrame::NoFrame);
}

void Hairline::paintEvent(QPaintEvent *)
{
    // Her boyamada uygulama paletinden okunur: görünüm değişiminde
    // kendiliğinden doğrudur, saklanan renk yoktur.
    QPainter painter(this);
    painter.fillRect(rect(), hairlineColor());
}

QFont sectionTitleFont(const QWidget *reference)
{
    QFont font = reference->font();
    font.setPointSizeF(qMax(9.0, font.pointSizeF() - 1.5));
    font.setBold(true);
    font.setLetterSpacing(QFont::AbsoluteSpacing, 0.6);
    font.setCapitalization(QFont::AllUppercase);
    return font;
}

QFont compactFont(const QWidget *reference, const double delta)
{
    QFont font = reference->font();
    font.setPointSizeF(qMax(9.0, font.pointSizeF() + delta));
    return font;
}

} // namespace d26::ui
