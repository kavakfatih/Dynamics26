#include "CommandRegistry.h"

#include <QAction>
#include <QApplication>
#include <QKeySequence>
#include <QPalette>

namespace d26 {

CommandRegistry::CommandRegistry(QObject *parent) : QObject(parent) {}

QAction *CommandRegistry::add(const QString &id, const QString &text, const CommandGlyph glyph,
                              const QString &shortcut, const QString &toolTip)
{
    auto *action = addPlain(id, text, shortcut);
    glyphs_.insert(id, glyph);
    action->setIcon(CaeIcons::forCommand(glyph, qApp->palette().color(QPalette::WindowText)));
    if (!toolTip.isEmpty()) {
        baseToolTips_.insert(id, toolTip);
        action->setToolTip(toolTip);
    }
    return action;
}

QAction *CommandRegistry::addPlain(const QString &id, const QString &text, const QString &shortcut)
{
    auto *action = new QAction(text, this);
    action->setObjectName(id);
    if (!shortcut.isEmpty()) {
        action->setShortcut(QKeySequence(shortcut));
    }
    if (!baseToolTips_.contains(id)) {
        baseToolTips_.insert(id, text);
    }
    actions_.insert(id, action);
    // Routed signal ID trigger anında okunur. Böylece menu/toolbar'ın elindeki
    // QAction pointer'ı değişmeden application composition handler'ı migrate
    // edilebilir; command surface identity ise daima orijinal id olarak kalır.
    connect(action, &QAction::triggered, this, [this, id] {
        emit commandTriggered(routedIds_.value(id, id));
    });
    return action;
}

QAction *CommandRegistry::action(const QString &id) const
{
    return actions_.value(id, nullptr);
}

void CommandRegistry::setEnabled(const QString &id, const bool enabled, const QString &disabledReason)
{
    QAction *target = action(id);
    if (target == nullptr) {
        return;
    }
    target->setEnabled(enabled);
    // Pasif komutun NEDEN pasif olduğu her zaman görünür olmalıdır.
    target->setToolTip(enabled || disabledReason.isEmpty() ? baseToolTips_.value(id, target->text())
                                                           : disabledReason);
}

void CommandRegistry::trigger(const QString &id)
{
    QAction *target = action(id);
    if (target != nullptr && target->isEnabled()) {
        target->trigger();
    }
}

void CommandRegistry::routeSignal(const QString &id, const QString &routedId)
{
    if (!actions_.contains(id)) {
        return;
    }
    if (routedId.isEmpty() || routedId == id) {
        routedIds_.remove(id);
    } else {
        routedIds_.insert(id, routedId);
    }
}

void CommandRegistry::refreshIcons()
{
    CaeIcons::invalidateCache();
    const QColor tint = qApp->palette().color(QPalette::WindowText);
    for (auto it = glyphs_.constBegin(); it != glyphs_.constEnd(); ++it) {
        if (QAction *target = action(it.key())) {
            target->setIcon(CaeIcons::forCommand(it.value(), tint));
        }
    }
}

} // namespace d26
