#pragma once

// Dynamics26 Alpha.3.6 — persistent Named Selection document commands.
//
// Transient viewport selection Undo/Redo'ya GİRMEZ. Ancak transient seçimden
// doğrulanmış ScopeReference üretildikten sonra oluşturulan/değiştirilen Named
// Selection bir document nesnesidir ve tüm mutasyonları QUndoStack üzerinden
// geçer. ObjectId ve tree row undo/redo boyunca birebir korunur.

#include "DomainCommands.h"
#include "../core/ProjectModel.h"
#include "../services/NamedSelectionService.h"

#include <QObject>
#include <utility>

namespace d26::commands {

class CreateNamedSelectionCommand final : public DomainCommand
{
public:
    CreateNamedSelectionCommand(const ServiceContext &services,
                                NamedSelectionDefinition definition,
                                const int row = -1,
                                const QString &text = QObject::tr("Create Named Selection"))
        : DomainCommand(services, text), definition_(std::move(definition)), row_(row)
    {
    }

    void redo() override
    {
        if (services_.namedSelections == nullptr) {
            return;
        }
        const ObjectId id = services_.namedSelections->createWithScope(definition_, row_, created_);
        if (created_ == InvalidObjectId && id != InvalidObjectId) {
            created_ = id;
            if (const NamedSelectionDefinition *stored = services_.namedSelections->byId(id)) {
                definition_ = *stored; // benzersizleştirilmiş ad korunur
            }
            row_ = services_.namedSelections->rowOf(id);
        }
    }

    void undo() override
    {
        if (services_.namedSelections != nullptr && created_ != InvalidObjectId) {
            services_.namedSelections->remove(created_);
        }
    }

    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    NamedSelectionDefinition definition_;
    int row_{-1};
    ObjectId created_{InvalidObjectId};
};

class DeleteNamedSelectionCommand final : public DomainCommand
{
public:
    DeleteNamedSelectionCommand(const ServiceContext &services, const ObjectId id)
        : DomainCommand(services,
                        QObject::tr("Delete %1").arg(
                            services.project != nullptr && services.project->object(id) != nullptr
                                ? services.project->object(id)->name
                                : QObject::tr("Named Selection"))),
          object_(id)
    {
        if (services.namedSelections != nullptr) {
            if (const NamedSelectionDefinition *stored = services.namedSelections->byId(id)) {
                definition_ = *stored;
            }
            row_ = services.namedSelections->rowOf(id);
        }
    }

    void redo() override
    {
        if (services_.namedSelections != nullptr) {
            services_.namedSelections->remove(object_);
        }
    }

    void undo() override
    {
        if (services_.namedSelections != nullptr && object_ != InvalidObjectId) {
            (void)services_.namedSelections->createWithScope(definition_, row_, object_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    NamedSelectionDefinition definition_;
    int row_{-1};
};

class RenameNamedSelectionCommand final : public DomainCommand
{
public:
    RenameNamedSelectionCommand(const ServiceContext &services, const ObjectId id,
                                const QString &requestedName)
        : DomainCommand(services, QObject::tr("Rename Named Selection")), object_(id),
          after_(requestedName.trimmed())
    {
        if (services.namedSelections != nullptr) {
            if (const NamedSelectionDefinition *stored = services.namedSelections->byId(id)) {
                before_ = stored->name;
            }
        }
    }

    void redo() override
    {
        if (services_.namedSelections == nullptr || after_.isEmpty()) {
            return;
        }
        services_.namedSelections->rename(object_, after_);
        if (!capturedFinalName_) {
            if (const NamedSelectionDefinition *stored = services_.namedSelections->byId(object_)) {
                after_ = stored->name;
            }
            capturedFinalName_ = true;
        }
    }

    void undo() override
    {
        if (services_.namedSelections != nullptr && !before_.isEmpty()) {
            services_.namedSelections->rename(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    QString before_;
    QString after_;
    bool capturedFinalName_{false};
};

class ReplaceNamedSelectionScopeCommand final : public DomainCommand
{
public:
    ReplaceNamedSelectionScopeCommand(const ServiceContext &services, const ObjectId id,
                                      ScopeReference after)
        : DomainCommand(services, QObject::tr("Change Named Selection Scope")), object_(id),
          after_(std::move(after))
    {
        if (services.namedSelections != nullptr) {
            if (const NamedSelectionDefinition *stored = services.namedSelections->byId(id)) {
                before_ = stored->scope;
            }
        }
    }

    void redo() override
    {
        if (services_.namedSelections != nullptr) {
            services_.namedSelections->replaceScope(object_, after_);
        }
    }

    void undo() override
    {
        if (services_.namedSelections != nullptr) {
            services_.namedSelections->replaceScope(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    ScopeReference before_;
    ScopeReference after_;
};

} // namespace d26::commands
