#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — ContactRegion document commands.
//
// Contact source/target surface scope kalıcı engineering state'tir ve bu nedenle
// create/delete/rename/scope replacement/suppression işlemleri QUndoStack
// üzerinden geçer. Transient viewport selection bu komutlara girmez; yalnız
// doğrulanmış ScopeReference document state'e dönüştüğünde undoable olur.
//
// ObjectId ve ProjectModel tree row undo/redo boyunca birebir korunur. Contact
// engineering verisinin sahibi ContactService'tir; ProjectModel yalnız tree
// identity/state taşır.

#include "DomainCommands.h"
#include "../core/ProjectModel.h"
#include "../services/ContactService.h"

#include <QObject>
#include <utility>

namespace d26::commands {

class CreateContactCommand final : public DomainCommand
{
public:
    CreateContactCommand(const ServiceContext &services, ContactDefinition definition,
                         const int row = -1,
                         const QString &text = QObject::tr("Create Contact Region"))
        : DomainCommand(services, text), definition_(std::move(definition)), row_(row)
    {
    }

    void redo() override
    {
        if (services_.contacts == nullptr) {
            return;
        }
        const ObjectId id = services_.contacts->createContact(definition_, row_, created_);
        if (created_ == InvalidObjectId && id != InvalidObjectId) {
            created_ = id;
            if (const ContactDefinition *stored = services_.contacts->byId(id)) {
                definition_ = *stored; // benzersizleştirilmiş ad korunur
            }
            row_ = services_.contacts->rowOf(id);
        }
    }

    void undo() override
    {
        if (services_.contacts != nullptr && created_ != InvalidObjectId) {
            services_.contacts->remove(created_);
        }
    }

    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    ContactDefinition definition_;
    int row_{-1};
    ObjectId created_{InvalidObjectId};
};

class DeleteContactCommand final : public DomainCommand
{
public:
    DeleteContactCommand(const ServiceContext &services, const ObjectId id)
        : DomainCommand(services,
                        QObject::tr("Delete %1").arg(
                            services.project != nullptr && services.project->object(id) != nullptr
                                ? services.project->object(id)->name
                                : QObject::tr("Contact Region"))),
          object_(id)
    {
        if (services.contacts != nullptr) {
            if (const ContactDefinition *stored = services.contacts->byId(id)) {
                definition_ = *stored;
            }
            row_ = services.contacts->rowOf(id);
        }
        if (services.project != nullptr) {
            wasSuppressed_ = services.project->isSuppressed(id);
        }
    }

    void redo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->remove(object_);
        }
    }

    void undo() override
    {
        if (services_.contacts == nullptr || object_ == InvalidObjectId) {
            return;
        }
        const ObjectId restored = services_.contacts->createContact(definition_, row_, object_);
        if (restored == object_ && wasSuppressed_) {
            services_.contacts->setSuppressed(object_, true);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    ContactDefinition definition_;
    int row_{-1};
    bool wasSuppressed_{false};
};

class RenameContactCommand final : public DomainCommand
{
public:
    RenameContactCommand(const ServiceContext &services, const ObjectId id,
                         const QString &requestedName)
        : DomainCommand(services, QObject::tr("Rename Contact Region")), object_(id),
          after_(requestedName.trimmed())
    {
        if (services.contacts != nullptr) {
            if (const ContactDefinition *stored = services.contacts->byId(id)) {
                before_ = stored->name;
            }
        }
    }

    void redo() override
    {
        if (services_.contacts == nullptr || after_.isEmpty()) {
            return;
        }
        services_.contacts->rename(object_, after_);
        if (!capturedFinalName_) {
            if (const ContactDefinition *stored = services_.contacts->byId(object_)) {
                after_ = stored->name;
            }
            capturedFinalName_ = true;
        }
    }

    void undo() override
    {
        if (services_.contacts != nullptr && !before_.isEmpty()) {
            services_.contacts->rename(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    QString before_;
    QString after_;
    bool capturedFinalName_{false};
};

class ReplaceContactSourceScopeCommand final : public DomainCommand
{
public:
    ReplaceContactSourceScopeCommand(const ServiceContext &services, const ObjectId id,
                                     ScopeReference after)
        : DomainCommand(services, QObject::tr("Change Contact Source Scope")), object_(id),
          after_(std::move(after))
    {
        if (services.contacts != nullptr) {
            if (const ContactDefinition *stored = services.contacts->byId(id)) {
                before_ = stored->sourceScope;
            }
        }
    }

    void redo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->replaceSourceScope(object_, after_);
        }
    }

    void undo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->replaceSourceScope(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    ScopeReference before_;
    ScopeReference after_;
};

class ReplaceContactTargetScopeCommand final : public DomainCommand
{
public:
    ReplaceContactTargetScopeCommand(const ServiceContext &services, const ObjectId id,
                                     ScopeReference after)
        : DomainCommand(services, QObject::tr("Change Contact Target Scope")), object_(id),
          after_(std::move(after))
    {
        if (services.contacts != nullptr) {
            if (const ContactDefinition *stored = services.contacts->byId(id)) {
                before_ = stored->targetScope;
            }
        }
    }

    void redo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->replaceTargetScope(object_, after_);
        }
    }

    void undo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->replaceTargetScope(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    ScopeReference before_;
    ScopeReference after_;
};

class SetContactSuppressedCommand final : public DomainCommand
{
public:
    SetContactSuppressedCommand(const ServiceContext &services, const ObjectId id,
                                const bool suppressed)
        : DomainCommand(services,
                        suppressed ? QObject::tr("Suppress Contact Region")
                                   : QObject::tr("Unsuppress Contact Region")),
          object_(id), after_(suppressed)
    {
        if (services.project != nullptr) {
            before_ = services.project->isSuppressed(id);
        }
    }

    void redo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->setSuppressed(object_, after_);
        }
    }

    void undo() override
    {
        if (services_.contacts != nullptr) {
            services_.contacts->setSuppressed(object_, before_);
        }
    }

private:
    ObjectId object_{InvalidObjectId};
    bool after_{false};
    bool before_{false};
};

} // namespace d26::commands
