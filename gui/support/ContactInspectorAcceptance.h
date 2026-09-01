#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact Inspector application acceptance.
//
// Gerçek DetailsHost üzerinde Connections -> Create Contact Region ve seçili
// ContactRegion Inspector binding'lerini doğrular. Source/Target edit oturumu
// gerçek SelectionCoordinator + SelectionManager üzerinden yürür: transient
// viewport seçimi Undo üretmez, yalnız Apply Selection tek persistent Contact
// scope transaction'ı oluşturur; Cancel ise document state'i değiştirmez.

#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/SelectionTypes.h"
#include "../details/ConnectionsDetails.h"
#include "../details/ContactDetails.h"
#include "../services/ContactService.h"
#include "../services/MeshService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/GraphicsWorkspace.h"
#include "../shell/SelectionCoordinator.h"

#include <QApplication>
#include <QLabel>
#include <QLineEdit>
#include <QObject>
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

inline SelectionItem meshFacetSelection(const quint64 generation,
                                        const femcae::meshing::MeshEntityId facetId)
{
    SelectionItem item;
    item.domain = SelectionDomain::Mesh;
    item.kind = SelectionKind::Facet;
    item.meshEntityId = facetId;
    item.sourceRevision = generation;
    return item;
}

inline SelectionCoordinator *coordinator(Dynamics26MainWindow &window)
{
    for (QObject *child : window.children()) {
        if (auto *candidate = dynamic_cast<SelectionCoordinator *>(child)) {
            return candidate;
        }
    }
    return nullptr;
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
    GraphicsWorkspace *graphics = window.graphics();
    SelectionCoordinator *selectionCoordinator =
        contact_inspector_acceptance_detail::coordinator(window);
    check(services.project != nullptr && services.contacts != nullptr && services.mesh != nullptr
              && details != nullptr && details->connectionsPage() != nullptr
              && details->contactPage() != nullptr && commands != nullptr
              && graphics != nullptr && selectionCoordinator != nullptr
              && selectionCoordinator->selectionManager() != nullptr,
          "Contact Inspector acceptance has authoritative services, Details, transient selection and document Undo stack");
    if (services.project == nullptr || services.contacts == nullptr || services.mesh == nullptr
        || details == nullptr || details->connectionsPage() == nullptr
        || details->contactPage() == nullptr || commands == nullptr
        || graphics == nullptr || selectionCoordinator == nullptr
        || selectionCoordinator->selectionManager() == nullptr) {
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
    auto *editSource = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactEditSource"));
    auto *applySource = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactApplySource"));
    auto *cancelSource = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactCancelSource"));
    auto *clearSource = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactClearSource"));
    auto *editTarget = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactEditTarget"));
    auto *applyTarget = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactApplyTarget"));
    auto *cancelTarget = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactCancelTarget"));
    auto *clearTarget = contactPage->findChild<QPushButton *>(QStringLiteral("Dynamics26ContactClearTarget"));
    check(name != nullptr && sourceSummary != nullptr && targetSummary != nullptr
              && validation != nullptr && solverSupport != nullptr
              && editSource != nullptr && applySource != nullptr && cancelSource != nullptr
              && clearSource != nullptr && editTarget != nullptr && applyTarget != nullptr
              && cancelTarget != nullptr && clearTarget != nullptr,
          "Contact Inspector exposes stable Name/Source/Target edit/validation/solver-support bindings");
    if (name == nullptr || sourceSummary == nullptr || targetSummary == nullptr
        || validation == nullptr || solverSupport == nullptr
        || editSource == nullptr || applySource == nullptr || cancelSource == nullptr
        || clearSource == nullptr || editTarget == nullptr || applyTarget == nullptr
        || cancelTarget == nullptr || clearTarget == nullptr) {
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

    const quint64 generation = services.mesh->generation();
    const auto sourceFacet = services.mesh->mesh().boundaryFacets.at(0).id;
    const auto targetFacet = services.mesh->mesh().boundaryFacets.at(1).id;

    // --- Source edit: transient selection -> single persistent Apply ----------
    const int beforeSourceEditCount = commands->stack()->count();
    editSource->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContact() == contactId
              && selectionCoordinator->editingContactSource()
              && details->currentObject() == contactId
              && graphics->selectionFilter() == SelectionFilter::Facet
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeSourceEditCount,
          "Edit Source opens Mesh/Facet transient session while Contact stays Navigator/Inspector context and Undo stays unchanged");

    const SelectionItem sourceSelection = contact_inspector_acceptance_detail::meshFacetSelection(
        generation, sourceFacet);
    check(selectionCoordinator->selectionManager()->apply(sourceSelection, SelectionOperation::Replace),
          "Source edit accepts real current FEM Facet transient selection");
    check(commands->stack()->count() == beforeSourceEditCount
              && details->currentObject() == contactId
              && applySource->isEnabled(),
          "transient Source selection creates no document Undo entry and keeps Contact context");
    applySource->click();
    const ContactDefinition *afterSource = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeSourceEditCount + 1
              && afterSource != nullptr
              && afterSource->sourceScope.sourceRevision == generation
              && afterSource->sourceScope.entities.size() == 1
              && afterSource->sourceScope.entities.front().meshEntityId == sourceFacet
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope,
          "Apply Source creates exactly one persistent Mesh/Facet scope transaction and advances to missing-Target state");

    // --- Target edit: domain locked to Source ---------------------------------
    const int beforeTargetEditCount = commands->stack()->count();
    editTarget->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContactTarget()
              && graphics->selectionFilter() == SelectionFilter::Facet
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeTargetEditCount,
          "Edit Target inherits Source Mesh/Facet domain without document mutation");
    const SelectionItem targetSelection = contact_inspector_acceptance_detail::meshFacetSelection(
        generation, targetFacet);
    check(selectionCoordinator->selectionManager()->apply(targetSelection, SelectionOperation::Replace),
          "Target edit accepts a second real current FEM Facet transient selection");
    check(commands->stack()->count() == beforeTargetEditCount && applyTarget->isEnabled(),
          "transient Target selection remains outside document Undo history");
    applyTarget->click();
    const ContactDefinition *completed = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeTargetEditCount + 1
              && completed != nullptr
              && completed->targetScope.sourceRevision == generation
              && completed->targetScope.entities.front().meshEntityId == targetFacet
              && services.contacts->validate(contactId).valid(),
          "Apply Target creates one persistent scope transaction and completes valid Contact definition");
    details->refresh();
    check(sourceSummary->text().contains(QStringLiteral("Mesh / Facet"))
              && targetSummary->text().contains(QStringLiteral("Mesh / Facet"))
              && clearSource->isEnabled() && clearTarget->isEnabled(),
          "Contact Inspector reads completed persistent Source/Target scope from ContactService");

    commands->stack()->undo();
    details->refresh();
    check(services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope
              && services.contacts->byId(contactId)->sourceScope.entities.front().meshEntityId == sourceFacet,
          "Undo Target Apply restores exact Source and missing-Target authoring state");
    commands->stack()->redo();
    details->refresh();
    check(services.contacts->validate(contactId).valid()
              && services.contacts->byId(contactId)->targetScope.entities.front().meshEntityId == targetFacet,
          "Redo Target Apply restores completed Contact exactly");

    // --- Cancel: changed transient selection must not persist ------------------
    const int beforeCancelCount = commands->stack()->count();
    editTarget->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContactTarget()
              && selectionCoordinator->selectionManager()->items().size() == 1
              && selectionCoordinator->selectionManager()->items().front().meshEntityId == targetFacet,
          "editing an existing current Target safely preloads its exact persistent Facet identity");
    check(selectionCoordinator->selectionManager()->apply(sourceSelection, SelectionOperation::Replace),
          "Cancel fixture changes only transient Target selection");
    check(commands->stack()->count() == beforeCancelCount,
          "changed transient Target selection still creates no document transaction");
    cancelTarget->click();
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeCancelCount
              && services.contacts->byId(contactId)->targetScope.entities.front().meshEntityId == targetFacet,
          "Cancel Target closes session with zero document mutation and preserves persisted Target scope");

    // --- Clear buttons remain canonical persistent operations ------------------
    const ScopeReference source = services.contacts->byId(contactId)->sourceScope;
    const ScopeReference target = services.contacts->byId(contactId)->targetScope;
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

    // --- Stale scope never preloads old MeshEntityId ---------------------------
    const quint64 oldGeneration = services.mesh->generation();
    const int beforeStaleEditCount = commands->stack()->count();
    check(services.mesh->generate() && services.mesh->generation() != oldGeneration,
          "mesh regeneration advances generation before stale Contact edit acceptance");
    check(services.contacts->validate(contactId).error == ContactValidationError::SourceScopeInvalid
              && services.project->object(contactId) != nullptr
              && services.project->object(contactId)->state == ObjectState::OutOfDate,
          "mesh regeneration marks stored Contact scope stale before repair edit");
    details->refresh();
    editSource->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContactSource()
              && selectionCoordinator->editPreloadError() == ScopeReferenceValidationError::StaleMeshGeneration
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeStaleEditCount,
          "stale Contact Source edit opens with zero old-ID preload and explicit StaleMeshGeneration diagnostic");
    cancelSource->click();
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeStaleEditCount
              && services.contacts->byId(contactId)->sourceScope.sourceRevision == oldGeneration,
          "Cancel stale Source repair preserves old persistent scope and creates no Undo entry");

    window.selectObject(services.project->projectRoot());
    services.contacts->remove(contactId);
    commands->resetHistory();

    std::cout << "Contact Inspector authoring acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
