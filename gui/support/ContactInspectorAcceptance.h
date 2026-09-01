#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact Inspector application acceptance.
//
// Gerçek DetailsHost üzerinde Connections -> Create Contact Region ve seçili
// ContactRegion Inspector binding'lerini doğrular. Widget state ikinci bir model
// değildir; her mutation ContactCommands + DocumentCommandManager üzerinden
// authoritative ContactService state'ine gider.

#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../details/ConnectionsDetails.h"
#include "../details/ContactDetails.h"
#include "../services/ContactService.h"
#include "../services/MeshService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QUndoStack>

#include <iostream>

namespace d26 {
namespace contact_inspector_acceptance_detail {

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

} // namespace contact_inspector_acceptance_detail

inline int runContactInspectorAcceptanceTest(QApplication &app,
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
    DetailsHost *details = window.detailsHost();
    DocumentCommandManager *commands = window.documentCommands();
    check(services.project != nullptr && services.contacts != nullptr && services.mesh != nullptr
              && details != nullptr && details->connectionsPage() != nullptr
              && details->contactPage() != nullptr && commands != nullptr,
          "Contact Inspector acceptance has authoritative services, DetailsHost and document Undo stack");
    if (services.project == nullptr || services.contacts == nullptr || services.mesh == nullptr
        || details == nullptr || details->connectionsPage() == nullptr
        || details->contactPage() == nullptr || commands == nullptr) {
        return 1;
    }

    services.contacts->clear();
    commands->resetHistory();
    window.selectObject(services.project->connectionsNode());
    details->refresh();

    QPushButton *addContact = details->connectionsPage()->findChild<QPushButton *>(
        QStringLiteral("Dynamics26ConnectionsAddContact"));
    check(addContact != nullptr && addContact->isEnabled(),
          "Connections Inspector exposes enabled New Contact Region authoring action");
    if (addContact == nullptr) {
        return failures + 1;
    }

    const int beforeCreateCount = commands->stack()->count();
    addContact->click();
    check(commands->stack()->count() == beforeCreateCount + 1 && services.contacts->count() == 1,
          "Connections Inspector creates Contact through exactly one document command");
    const ObjectId contactId = services.contacts->order().isEmpty()
        ? InvalidObjectId : services.contacts->order().last();
    check(contactId != InvalidObjectId
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope
              && services.project->object(contactId) != nullptr,
          "new Contact Inspector object starts as persistent missing-Source authoring state");

    commands->stack()->undo();
    check(services.contacts->byId(contactId) == nullptr && services.project->object(contactId) == nullptr,
          "Undo Connections create removes draft Contact engineering state and tree identity");
    commands->stack()->redo();
    check(services.contacts->byId(contactId) != nullptr
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope,
          "Redo Connections create restores same draft Contact ObjectId and authoring state");

    window.selectObject(contactId);
    ContactDetails *contactPage = details->contactPage();
    check(details->currentObject() == contactId,
          "ContactRegion project selection maps DetailsHost to dedicated Contact Inspector");

    auto *name = contactPage->findChild<QLineEdit *>(QStringLiteral("Dynamics26ContactName"));
    auto *sourceSummary = contactPage->findChild<QLabel *>(QStringLiteral("Dynamics26ContactSourceSummary"));
    auto *targetSummary = contactPage->findChild<QLabel *>(QStringLiteral("Dynamics26ContactTargetSummary"));
    auto *validation = contactPage->findChild<QLabel *>(QStringLiteral("Dynamics26ContactValidation"));
    auto *solverSupport = contactPage->findChild<QLabel *>(QStringLiteral("Dynamics26ContactSolverSupport"));
    auto *clearSource = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactClearSource"));
    auto *clearTarget = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactClearTarget"));
    check(name != nullptr && sourceSummary != nullptr && targetSummary != nullptr
              && validation != nullptr && solverSupport != nullptr
              && clearSource != nullptr && clearTarget != nullptr,
          "Contact Inspector exposes stable Name/Source/Target/validation/solver-support bindings");
    if (name == nullptr || sourceSummary == nullptr || targetSummary == nullptr
        || validation == nullptr || solverSupport == nullptr
        || clearSource == nullptr || clearTarget == nullptr) {
        return failures + 1;
    }

    check(sourceSummary->text().contains(QStringLiteral("Tanımlanmadı"))
              && targetSummary->text().contains(QStringLiteral("Tanımlanmadı"))
              && validation->text().contains(QStringLiteral("Source"))
              && !clearSource->isEnabled() && !clearTarget->isEnabled(),
          "draft Contact Inspector reflects authoritative incomplete state without inventing scope");
    check(solverSupport->text().contains(QStringLiteral("henüz etkin değil")),
          "Contact Inspector does not overstate current model solver support");

    const int beforeRenameCount = commands->stack()->count();
    name->setText(QStringLiteral("Inspector Bonded Interface"));
    QMetaObject::invokeMethod(name, "editingFinished", Qt::DirectConnection);
    check(commands->stack()->count() == beforeRenameCount + 1
              && services.contacts->byId(contactId)->name == QStringLiteral("Inspector Bonded Interface")
              && services.project->object(contactId)->name == QStringLiteral("Inspector Bonded Interface"),
          "Contact Name widget creates one canonical rename transaction and keeps tree/service synchronized");
    commands->stack()->undo();
    details->refresh();
    check(services.contacts->byId(contactId)->name == QStringLiteral("Contact Region")
              && name->text() == QStringLiteral("Contact Region"),
          "Undo Contact Inspector rename restores authoritative service and widget state");
    commands->stack()->redo();
    details->refresh();

    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(services.mesh->generate(), "Contact Inspector fixture generates current FEM mesh");
    }
    check(services.mesh->mesh().boundaryFacets.size() >= 2,
          "Contact Inspector fixture exposes real boundary Facet identities");
    if (services.mesh->mesh().boundaryFacets.size() < 2) {
        return failures + 1;
    }

    const ScopeReference source = contact_inspector_acceptance_detail::meshFacetScope(
        services.mesh->generation(), services.mesh->mesh().boundaryFacets.at(0).id);
    const ScopeReference target = contact_inspector_acceptance_detail::meshFacetScope(
        services.mesh->generation(), services.mesh->mesh().boundaryFacets.at(1).id);
    commands->push(new commands::ReplaceContactSourceScopeCommand(services, contactId, source));
    commands->push(new commands::ReplaceContactTargetScopeCommand(services, contactId, target));
    details->refresh();
    check(services.contacts->validate(contactId).valid()
              && sourceSummary->text().contains(QStringLiteral("Mesh / Facet"))
              && targetSummary->text().contains(QStringLiteral("Mesh / Facet"))
              && clearSource->isEnabled() && clearTarget->isEnabled(),
          "Contact Inspector reads completed persistent Source/Target scope from ContactService");

    const int beforeClearSourceCount = commands->stack()->count();
    clearSource->click();
    check(commands->stack()->count() == beforeClearSourceCount + 1
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope,
          "Clear Source widget creates exactly one persistent scope transaction");
    commands->stack()->undo();
    details->refresh();
    check(services.contacts->validate(contactId).valid()
              && services.contacts->byId(contactId)->sourceScope.entities.front().meshEntityId
                     == source.entities.front().meshEntityId,
          "Undo Clear Source restores exact previous Facet identity");

    const int beforeClearTargetCount = commands->stack()->count();
    clearTarget->click();
    check(commands->stack()->count() == beforeClearTargetCount + 1
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope,
          "Clear Target widget creates exactly one persistent scope transaction");
    commands->stack()->undo();
    details->refresh();
    check(services.contacts->validate(contactId).valid()
              && services.contacts->byId(contactId)->targetScope.entities.front().meshEntityId
                     == target.entities.front().meshEntityId,
          "Undo Clear Target restores exact previous Facet identity");

    window.selectObject(services.project->projectRoot());
    services.contacts->remove(contactId);
    commands->resetHistory();

    std::cout << "Contact Inspector authoring acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
