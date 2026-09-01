#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact shell mutation acceptance.
//
// Bu test ContactRegion'ın yalnız servis/komut seviyesinde değil, gerçek
// Dynamics26MainWindow komut yüzeyinden de ContactService authoritative state'ine
// gittiğini doğrular. Generic ProjectModel rename/suppress/delete yolunun Contact
// engineering tanımını tree state'ten koparmasına izin verilmez.

#include "../core/DocumentCommandManager.h"
#include "../services/ContactService.h"
#include "../services/MeshService.h"
#include "../shell/CommandRegistry.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QAction>
#include <QApplication>
#include <QMenu>
#include <QUndoStack>

#include <iostream>

namespace d26 {
namespace contact_shell_acceptance_detail {

inline ScopeReference meshFacetScope(const quint64 generation,
                                     const femcae::meshing::MeshEntityId facetId)
{
    ScopeEntityReference reference;
    reference.domain = SelectionDomain::Mesh;
    reference.kind = SelectionKind::Facet;
    reference.meshEntityId = facetId;

    ScopeReference scope;
    scope.sourceRevision = generation;
    scope.entities.push_back(reference);
    return scope;
}

inline bool menuContainsAction(const QMenu *menu, const QString &objectName)
{
    if (menu == nullptr) {
        return false;
    }
    for (const QAction *action : menu->actions()) {
        if (action != nullptr && action->objectName() == objectName) {
            return true;
        }
    }
    return false;
}

} // namespace contact_shell_acceptance_detail

inline int runContactShellAcceptanceTest(QApplication &app,
                                         Dynamics26MainWindow &window)
{
    Q_UNUSED(app);
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const char *message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };

    const ServiceContext services = window.services();
    check(services.project != nullptr && services.mesh != nullptr
              && services.contacts != nullptr && window.documentCommands() != nullptr
              && window.commandRegistry() != nullptr,
          "Contact shell acceptance has Project/Mesh/Contact/Undo/CommandRegistry composition");
    if (services.project == nullptr || services.mesh == nullptr
        || services.contacts == nullptr || window.documentCommands() == nullptr
        || window.commandRegistry() == nullptr) {
        return 1;
    }

    services.contacts->clear();
    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(services.mesh->generate(),
              "Contact shell fixture generates a current FEM mesh");
    }
    check(services.mesh->mesh().boundaryFacets.size() >= 2,
          "Contact shell fixture exposes two real boundary Facet identities");
    if (services.mesh->mesh().boundaryFacets.size() < 2) {
        return failures + 1;
    }

    ContactDefinition definition;
    definition.name = QStringLiteral("Shell Contact");
    definition.sourceScope = contact_shell_acceptance_detail::meshFacetScope(
        services.mesh->generation(), services.mesh->mesh().boundaryFacets.at(0).id);
    definition.targetScope = contact_shell_acceptance_detail::meshFacetScope(
        services.mesh->generation(), services.mesh->mesh().boundaryFacets.at(1).id);

    const ObjectId contactId = services.contacts->createContact(definition);
    check(contactId != InvalidObjectId && services.contacts->validate(contactId).valid(),
          "Contact shell fixture creates valid Contact engineering state");
    if (contactId == InvalidObjectId) {
        return failures + 1;
    }

    window.documentCommands()->resetHistory();
    window.selectObject(contactId);

    check(supportsRename(ObjectType::ContactRegion)
              && supportsDelete(ObjectType::ContactRegion)
              && supportsSuppression(ObjectType::ContactRegion)
              && !supportsDuplicate(ObjectType::ContactRegion),
          "ContactRegion exposes Rename/Delete/Suppress but not unsupported Duplicate capability");

    QAction *renameAction = window.commandRegistry()->action(QStringLiteral("edit.rename"));
    QAction *deleteAction = window.commandRegistry()->action(QStringLiteral("edit.delete"));
    QAction *suppressAction = window.commandRegistry()->action(QStringLiteral("edit.suppress"));
    check(renameAction != nullptr && renameAction->isEnabled()
              && deleteAction != nullptr && deleteAction->isEnabled()
              && suppressAction != nullptr && suppressAction->isEnabled(),
          "selected ContactRegion enables shell Rename/Delete/Suppress command surface");

    QMenu *contextMenu = window.buildContextMenu(contactId, &window);
    check(contact_shell_acceptance_detail::menuContainsAction(contextMenu, QStringLiteral("edit.rename"))
              && contact_shell_acceptance_detail::menuContainsAction(contextMenu, QStringLiteral("edit.suppress"))
              && contact_shell_acceptance_detail::menuContainsAction(contextMenu, QStringLiteral("edit.delete"))
              && !contact_shell_acceptance_detail::menuContainsAction(contextMenu, QStringLiteral("edit.duplicate")),
          "ContactRegion context menu exposes only supported mutation actions");
    delete contextMenu;

    QUndoStack *stack = window.documentCommands()->stack();
    const int baselineCount = stack->count();
    window.renameObject(contactId, QStringLiteral("Shell Bonded Interface"));
    check(stack->count() == baselineCount + 1
              && services.contacts->byId(contactId) != nullptr
              && services.contacts->byId(contactId)->name == QStringLiteral("Shell Bonded Interface")
              && services.project->object(contactId) != nullptr
              && services.project->object(contactId)->name == QStringLiteral("Shell Bonded Interface"),
          "MainWindow rename routes through Contact domain command and keeps tree/service names synchronized");
    stack->undo();
    check(services.contacts->byId(contactId)->name == QStringLiteral("Shell Contact")
              && services.project->object(contactId)->name == QStringLiteral("Shell Contact"),
          "Undo shell Contact rename restores authoritative engineering name");
    stack->redo();
    check(services.contacts->byId(contactId)->name == QStringLiteral("Shell Bonded Interface"),
          "Redo shell Contact rename restores final engineering name");

    const int beforeSuppressCount = stack->count();
    window.setObjectSuppressed(contactId, true);
    check(stack->count() == beforeSuppressCount + 1
              && services.project->isSuppressed(contactId)
              && services.project->object(contactId)->state == ObjectState::Suppressed,
          "MainWindow suppress routes through ContactService document command");
    stack->undo();
    check(!services.project->isSuppressed(contactId)
              && services.contacts->validate(contactId).valid()
              && services.project->object(contactId)->state == ObjectState::Ready,
          "Undo shell Contact suppression restores current validation state");
    stack->redo();
    check(services.project->isSuppressed(contactId)
              && services.project->object(contactId)->state == ObjectState::Suppressed,
          "Redo shell Contact suppression restores canonical Suppressed state");

    const ContactDefinition beforeDelete = *services.contacts->byId(contactId);
    const int rowBeforeDelete = services.contacts->rowOf(contactId);
    const int beforeDeleteCount = stack->count();
    window.deleteObject(contactId);
    check(stack->count() == beforeDeleteCount + 1
              && services.contacts->byId(contactId) == nullptr
              && services.project->object(contactId) == nullptr,
          "MainWindow delete removes ContactService definition and tree identity in one command");
    stack->undo();
    const ContactDefinition *restored = services.contacts->byId(contactId);
    check(restored != nullptr && services.contacts->rowOf(contactId) == rowBeforeDelete
              && restored->name == beforeDelete.name
              && restored->sourceScope.sourceRevision == beforeDelete.sourceScope.sourceRevision
              && restored->targetScope.sourceRevision == beforeDelete.targetScope.sourceRevision
              && restored->sourceScope.entities.front().meshEntityId
                     == beforeDelete.sourceScope.entities.front().meshEntityId
              && restored->targetScope.entities.front().meshEntityId
                     == beforeDelete.targetScope.entities.front().meshEntityId,
          "Undo shell Contact delete restores exact ObjectId row name and source/target engineering scopes");
    check(services.project->isSuppressed(contactId)
              && services.project->object(contactId)->state == ObjectState::Suppressed,
          "Undo shell Contact delete also restores suppressed document state");
    stack->redo();
    check(services.contacts->byId(contactId) == nullptr
              && services.project->object(contactId) == nullptr,
          "Redo shell Contact delete removes the same ObjectId deterministically");
    stack->undo();
    check(services.contacts->byId(contactId) != nullptr
              && services.project->isSuppressed(contactId),
          "second Undo shell Contact delete remains deterministic");

    window.selectObject(services.project->projectRoot());
    services.contacts->remove(contactId);
    window.documentCommands()->resetHistory();

    std::cout << "Contact shell mutation acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
