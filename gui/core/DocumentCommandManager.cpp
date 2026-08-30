#include "DocumentCommandManager.h"

#include <QAction>
#include <QUndoCommand>
#include <QUndoStack>

namespace d26 {

DocumentCommandManager::DocumentCommandManager(QObject *parent) : QObject(parent)
{
    stack_ = new QUndoStack(this);
    // Yığın derinliği: mühendislik oturumu için yeterli, bellek için sınırlı.
    stack_->setUndoLimit(200);

    connect(stack_, &QUndoStack::indexChanged, this, [this](int) {
        emit documentMutated();
        const bool dirty = isDirty();
        if (dirty != lastDirty_) {
            lastDirty_ = dirty;
            emit dirtyChanged(dirty);
        }
    });
    connect(stack_, &QUndoStack::cleanChanged, this, [this](const bool clean) {
        const bool dirty = !clean;
        if (dirty != lastDirty_) {
            lastDirty_ = dirty;
            emit dirtyChanged(dirty);
        }
    });
}

void DocumentCommandManager::push(QUndoCommand *command)
{
    if (command == nullptr) {
        return;
    }
    stack_->push(command);
}

void DocumentCommandManager::beginMacro(const QString &text)
{
    stack_->beginMacro(text);
}

void DocumentCommandManager::endMacro()
{
    stack_->endMacro();
}

void DocumentCommandManager::markSaved()
{
    stack_->setClean();
    lastDirty_ = false;
    emit dirtyChanged(false);
}

void DocumentCommandManager::resetHistory()
{
    stack_->clear();
    stack_->setClean();
    lastDirty_ = false;
    emit dirtyChanged(false);
}

bool DocumentCommandManager::isDirty() const
{
    return !stack_->isClean();
}

QAction *DocumentCommandManager::createUndoAction(QObject *parent) const
{
    // QUndoStack dinamik metni üretir: "Undo Add Force", "Undo Rename Material" …
    return stack_->createUndoAction(parent, QObject::tr("Geri Al"));
}

QAction *DocumentCommandManager::createRedoAction(QObject *parent) const
{
    return stack_->createRedoAction(parent, QObject::tr("Yinele"));
}

} // namespace d26
