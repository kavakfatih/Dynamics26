#pragma once

// Görünüm yardımcıları.
//
// Bu dosya bir tema motoru DEĞİLDİR: QApplication paleti veya global QSS
// değiştirilmez. Yalnız Dynamics26'nın kendi çizdiği ikincil metin, ayraç,
// satır gölgesi ve mühendislik durum renklerini native paletten TÜRETİR.
//
// KRİTİK KURAL — stale foreground önlemi:
// Türetilen renkler HER ZAMAN `QApplication::palette()` üzerinden hesaplanır,
// asla widget'ın kendi (daha önce mutasyona uğratılmış) paletinden değil.
// Bir widget'a açıkça renk atandığında Qt o rolü "resolved" işaretler ve
// görünüm değişiminde native paletten güncellemez; kendi paletinden yeniden
// türetmek Light → Dark geçişinde koyu-üstüne-koyu metin üretirdi.

#include <QColor>
#include <QFont>
#include <QFrame>
#include <QLabel>
#include <QPalette>

namespace d26::ui {

// Uygulamanın native paleti — tüm türetmelerin tek kaynağı.
[[nodiscard]] QPalette applicationPalette();
[[nodiscard]] bool isDarkAppearance();
[[nodiscard]] bool isDark(const QPalette &palette);

// Koyu görünümde ikincil metin daha yüksek opaklık ister; aynı alfa iki modda
// aynı okunabilirliği vermez.
[[nodiscard]] QColor secondaryText(double lightAlpha = 0.62, double darkAlpha = 0.82);
[[nodiscard]] QColor hairlineColor();
[[nodiscard]] QColor rowShadeColor();

// Mühendislik durum renkleri. Bunlar tema rengi değil, MÜHENDİSLİK ANLAMIDIR;
// yine de iki görünümde yeterli kontrast için ton ayarlanır.
enum class StatusTone { Neutral, Ready, UpToDate, OutOfDate, Warning, Error, Suppressed, Solving };
[[nodiscard]] QColor statusColor(StatusTone tone);

// Palet değişimini izleyen ikincil metin etiketi.
class SecondaryLabel final : public QLabel
{
    Q_OBJECT
public:
    SecondaryLabel(const QString &text, double lightAlpha, double darkAlpha, QWidget *parent = nullptr);

protected:
    void changeEvent(QEvent *event) override;

private:
    void applyColor();
    double lightAlpha_;
    double darkAlpha_;
    bool applying_{false};
};

// Palet değişimini izleyen ince ayraç çizgisi.
class Hairline final : public QFrame
{
    Q_OBJECT
public:
    explicit Hairline(QWidget *parent = nullptr, Qt::Orientation orientation = Qt::Horizontal);

protected:
    void paintEvent(QPaintEvent *event) override;
};

// Bölüm başlığı (DEFINITION / SCOPE ...) ve panel başlığı yazı tipleri.
[[nodiscard]] QFont sectionTitleFont(const QWidget *reference);
[[nodiscard]] QFont compactFont(const QWidget *reference, double delta = -1.0);

} // namespace d26::ui
