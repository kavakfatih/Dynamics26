#pragma once

// Komut kayıt defteri.
//
// Tüm kullanıcı komutları tek bir yerde tanımlanır ve kimlikle (string id)
// adreslenir. Menü, üst komut yüzeyi ve Details sayfalarındaki düğmeler AYNI
// QAction'ı paylaşır; böylece bir komutun etkin/pasif durumu tek yerden
// yönetilir ve "çalışmayan buton" durumu oluşmaz.

#include <QHash>
#include <QObject>
#include <QString>

#include "../core/CaeIcons.h"

class QAction;

namespace d26 {

class CommandRegistry final : public QObject
{
    Q_OBJECT
public:
    explicit CommandRegistry(QObject *parent = nullptr);

    QAction *add(const QString &id, const QString &text, CommandGlyph glyph,
                 const QString &shortcut = QString(), const QString &toolTip = QString());
    QAction *addPlain(const QString &id, const QString &text, const QString &shortcut = QString());
    [[nodiscard]] QAction *action(const QString &id) const;

    void setEnabled(const QString &id, bool enabled, const QString &disabledReason = QString());
    void trigger(const QString &id);
    // Aynı QAction/menu/shortcut kimliğini koruyup yalnız application composition
    // katmanına yayılan command signal ID'sini değiştirir. Bu mekanizma migration
    // sırasında eski command surface'i ikinci QAction oluşturmadan yeni canonical
    // handler'a taşımak içindir. Boş routedId varsayılan id davranışını geri yükler.
    void routeSignal(const QString &id, const QString &routedId);
    // Görünüm değişiminde ikonlar palet rengiyle yeniden üretilir.
    void refreshIcons();

signals:
    void commandTriggered(const QString &id);

private:
    QHash<QString, QAction *> actions_;
    QHash<QString, CommandGlyph> glyphs_;
    QHash<QString, QString> baseToolTips_;
    QHash<QString, QString> routedIds_;
};

} // namespace d26
