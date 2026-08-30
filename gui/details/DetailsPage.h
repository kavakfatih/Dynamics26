#pragma once

// Details paneli yapı taşları.
//
// ANSYS Mechanical "Details" / COMSOL "Settings" yoğunluğunda, iki kolonlu,
// bölümlenmiş bir özellik yüzeyi kurar. Global QSS veya QPalette değiştirilmez;
// yalnız bu sınıfların kendi çizimleri sistem paletinden türetilir. Böylece
// macOS Light/Dark geçişi Qt'nin native davranışıyla çalışmaya devam eder.

#include <QFrame>
#include <QString>
#include <QWidget>

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QGridLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QToolButton;
class QVBoxLayout;

namespace d26 {

// Tek özellik satırı: sol etiket, sağ değer. Tek/çift satır arka planı
// palette().alternateBase() üzerinden çizilir; sabit renk kullanılmaz.
class DetailsRow final : public QWidget
{
    Q_OBJECT
public:
    DetailsRow(const QString &label, QWidget *value, bool shaded, QWidget *parent = nullptr);
    void setLabel(const QString &label);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QLabel *label_{nullptr};
    bool shaded_{false};
};

// Başlıklı bölüm (DEFINITION / SCOPE / STATISTICS ...). İsteğe bağlı olarak
// açılır-kapanır (Advanced bölümleri için).
class DetailsSection final : public QWidget
{
    Q_OBJECT
public:
    explicit DetailsSection(const QString &title, bool collapsible = false, bool collapsed = false,
                            QWidget *parent = nullptr);

    QLabel *addValueRow(const QString &label, const QString &value = QString());
    DetailsRow *addRow(const QString &label, QWidget *value);
    void addFullWidth(QWidget *widget);
    void addNote(const QString &text);
    void addSeparator();
    [[nodiscard]] bool isEmpty() const noexcept { return rowCount_ == 0; }

private:
    void setCollapsed(bool collapsed);

    QVBoxLayout *rows_{nullptr};
    QWidget *body_{nullptr};
    QToolButton *disclosure_{nullptr};
    QLabel *title_{nullptr};
    int rowCount_{0};
};

// Bir nesne türüne ait Details sayfasının ortak temeli. Sayfalar kurucu içinde
// widget'larını bir kez kurar; refresh() yalnız değerleri günceller. Böylece
// kullanıcı bir alanı düzenlerken odak kaybolmaz.
class DetailsPage : public QWidget
{
    Q_OBJECT
public:
    explicit DetailsPage(QWidget *parent = nullptr);

    // Sayfanın gösterdiği nesneyi belirler ve içeriği tazeler.
    virtual void setObject(quint64 objectId);
    virtual void refresh() = 0;
    [[nodiscard]] quint64 objectId() const noexcept { return objectId_; }

signals:
    // Sayfa bir mühendislik değişikliği uyguladı; kabuk viewport/durum çubuğunu
    // yeniden değerlendirmelidir.
    void modelEdited();
    // Sayfa bir komutun çalıştırılmasını istiyor (dosya diyalogları, çözüm vb.
    // sayfa içinde değil, komut katmanında yürütülür).
    void requestCommand(const QString &commandId);

protected:
    DetailsSection *addSection(const QString &title, bool collapsible = false, bool collapsed = false);
    void addStretch();
    // Salt-okunur sayfalar içeriklerini her tazelemede yeniden kurabilir.
    void clearSections();

    // Kompakt, hizalı denetim üretmek için ortak yardımcılar.
    static QDoubleSpinBox *makeDoubleField(double minimum, double maximum, int decimals, const QString &suffix);
    static QSpinBox *makeIntField(int minimum, int maximum);
    static QComboBox *makeCombo(const QStringList &items);
    static QLabel *makeValueLabel(const QString &text = QString());
    static QPushButton *makeActionButton(const QString &text);
    static QLabel *makeNoteLabel(const QString &text);

    quint64 objectId_{0};

private:
    QVBoxLayout *layout_{nullptr};
};

} // namespace d26
