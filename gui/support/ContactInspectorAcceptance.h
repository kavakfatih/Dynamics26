#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact Inspector application acceptance.
//
// Gerçek DetailsHost üzerinde Connections -> Create Contact Region ve seçili
// ContactRegion Inspector binding'lerini doğrular. Source/Target edit oturumu
// gerçek SelectionCoordinator + SelectionManager üzerinden yürür: transient
// viewport seçimi Undo üretmez, yalnız Apply Selection tek persistent Contact
// scope transaction'ı oluşturur; Cancel ise document state'i değiştirmez.
//
// Domain kontratı:
//   * Draft Contact ve güncel CAD varsa dayanıklı Geometry/Face scope ile başlar.
//   * Bir taraf Mesh/Facet ise diğer taraf aynı Mesh domain'ini miras alır.
//   * Mesh generation değişince eski Facet kimlikleri preload edilmez.

#include "../commands/ContactCommands.h"
#include "../core/DocumentCommandManager.h"
#include "../core/SelectionTypes.h"
#include "../details/ConnectionsDetails.h"
#include "../details/ContactDetails.h"
#include "../services/ContactService.h"
#include "../services/GeometryService.h"
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

inline SelectionItem geometryFaceSelection(
    const quint64 revision,
    const femcae::geometry::GeometryEntityId faceId,
    const femcae::geometry::GeometryEntityId parentId)
{
    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.kind = SelectionKind::Face;
    item.geometryEntityId = faceId;
    item.parentGeometryId = parentId;
    item.sourceRevision = revision;
    return item;
}

inline bool isSingleGeometryFace(const ScopeReference &scope,
                                 const quint64 revision,
                                 const femcae::geometry::GeometryEntityId faceId)
{
    return scope.sourceRevision == revision && scope.entities.size() == 1
        && scope.entities.front().domain == SelectionDomain::Geometry
        && scope.entities.front().kind == SelectionKind::Face
        && scope.entities.front().geometryEntityId == faceId;
}

inline bool isSingleMeshFacet(const ScopeReference &scope,
                              const quint64 generation,
                              const femcae::meshing::MeshEntityId facetId)
{
    return scope.sourceRevision == generation && scope.entities.size() == 1
        && scope.entities.front().domain == SelectionDomain::Mesh
        && scope.entities.front().kind == SelectionKind::Facet
        && scope.entities.front().meshEntityId == facetId;
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
    check(services.project != nullptr && services.contacts != nullptr
              && services.geometry != nullptr && services.mesh != nullptr
              && details != nullptr && details->connectionsPage() != nullptr
              && details->contactPage() != nullptr && commands != nullptr
              && graphics != nullptr && selectionCoordinator != nullptr
              && selectionCoordinator->selectionManager() != nullptr,
          "Contact Inspector acceptance has authoritative services, Details, transient selection and document Undo stack");
    if (services.project == nullptr || services.contacts == nullptr
        || services.geometry == nullptr || services.mesh == nullptr
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
    if (contactId == InvalidObjectId || services.contacts->byId(contactId) == nullptr) {
        return failures + 1;
    }

    commands->stack()->undo();
    check(services.contacts->byId(contactId) == nullptr && services.project->object(contactId) == nullptr,
          "Undo Connections create removes draft Contact engineering state and tree identity");
    commands->stack()->redo();
    check(services.contacts->byId(contactId) != nullptr
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope,
          "Redo Connections create restores same draft Contact ObjectId and authoring state");
    if (services.contacts->byId(contactId) == nullptr) {
        return failures + 1;
    }

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
    const ContactDefinition *renamed = services.contacts->byId(contactId);
    check(commands->stack()->count() == beforeRenameCount + 1
              && renamed != nullptr
              && renamed->name == QStringLiteral("Inspector Bonded Interface")
              && services.project->object(contactId) != nullptr
              && services.project->object(contactId)->name == QStringLiteral("Inspector Bonded Interface"),
          "Contact Name widget creates one canonical rename transaction and keeps tree/service synchronized");
    commands->stack()->undo();
    details->refresh();
    const ContactDefinition *undoRenamed = services.contacts->byId(contactId);
    check(undoRenamed != nullptr && undoRenamed->name == QStringLiteral("Contact Region")
              && name->text() == QStringLiteral("Contact Region"),
          "Undo Contact Inspector rename restores authoritative service and widget state");
    commands->stack()->redo();
    details->refresh();

    // Draft Contact dayanıklı CAD Face scope ile başlamalıdır. Contact mesh'ten
    // önce tanımlanabilsin ve remesh engineering kimliğini gereksiz yere bozmasın.
    const auto &document = services.geometry->document();
    const auto geometryFaces = document.entitiesOfKind(femcae::geometry::GeometryEntityKind::Face);
    check(services.geometry->summary().hasGeometry && geometryFaces.size() >= 2,
          "Contact Inspector fixture exposes at least two canonical CAD Face identities");
    if (!services.geometry->summary().hasGeometry || geometryFaces.size() < 2) {
        return failures + 1;
    }
    const auto sourceFaceId = geometryFaces.at(0);
    const auto targetFaceId = geometryFaces.at(1);
    const auto *sourceFaceEntity = document.find(sourceFaceId);
    const auto *targetFaceEntity = document.find(targetFaceId);
    check(sourceFaceEntity != nullptr && targetFaceEntity != nullptr
              && sourceFaceEntity->parentId != femcae::geometry::InvalidGeometryId
              && targetFaceEntity->parentId != femcae::geometry::InvalidGeometryId,
          "Contact Inspector CAD Face fixture resolves canonical parent Body identities");
    if (sourceFaceEntity == nullptr || targetFaceEntity == nullptr
        || sourceFaceEntity->parentId == femcae::geometry::InvalidGeometryId
        || targetFaceEntity->parentId == femcae::geometry::InvalidGeometryId) {
        return failures + 1;
    }
    const quint64 geometryRevision = document.revision();
    const SelectionItem sourceFaceSelection =
        contact_inspector_acceptance_detail::geometryFaceSelection(
            geometryRevision, sourceFaceId, sourceFaceEntity->parentId);
    const SelectionItem targetFaceSelection =
        contact_inspector_acceptance_detail::geometryFaceSelection(
            geometryRevision, targetFaceId, targetFaceEntity->parentId);

    // --- Draft Source: Geometry/Face transient selection -> single Apply -------
    const int beforeSourceEditCount = commands->stack()->count();
    editSource->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContact() == contactId
              && selectionCoordinator->editingContactSource()
              && details->currentObject() == contactId
              && graphics->selectionFilter() == SelectionFilter::Face
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeSourceEditCount,
          "draft Edit Source opens durable Geometry/Face transient session while Contact context stays current");
    check(selectionCoordinator->selectionManager()->apply(sourceFaceSelection, SelectionOperation::Replace),
          "Source edit accepts real current CAD Face transient selection");
    check(commands->stack()->count() == beforeSourceEditCount
              && details->currentObject() == contactId
              && applySource->isEnabled(),
          "transient Source Face creates no document Undo entry and keeps Contact context");
    applySource->click();
    const ContactDefinition *afterSource = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeSourceEditCount + 1
              && afterSource != nullptr
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     afterSource->sourceScope, geometryRevision, sourceFaceId)
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope,
          "Apply Source creates exactly one persistent Geometry/Face transaction and advances to missing-Target state");

    // --- Draft Target: domain locked to Source Geometry/Face -------------------
    const int beforeTargetEditCount = commands->stack()->count();
    editTarget->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContactTarget()
              && graphics->selectionFilter() == SelectionFilter::Face
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeTargetEditCount,
          "Edit Target inherits Source Geometry/Face domain without document mutation");
    check(selectionCoordinator->selectionManager()->apply(targetFaceSelection, SelectionOperation::Replace),
          "Target edit accepts a second real current CAD Face transient selection");
    check(commands->stack()->count() == beforeTargetEditCount && applyTarget->isEnabled(),
          "transient Target Face remains outside document Undo history");
    applyTarget->click();
    const ContactDefinition *completed = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeTargetEditCount + 1
              && completed != nullptr
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     completed->targetScope, geometryRevision, targetFaceId)
              && services.contacts->validate(contactId).valid(),
          "Apply Target creates one persistent Geometry/Face transaction and completes valid Contact definition");
    details->refresh();
    check(sourceSummary->text().contains(QStringLiteral("Geometry / Face"))
              && targetSummary->text().contains(QStringLiteral("Geometry / Face"))
              && clearSource->isEnabled() && clearTarget->isEnabled(),
          "Contact Inspector reads completed persistent CAD Source/Target scope from ContactService");

    commands->stack()->undo();
    details->refresh();
    const ContactDefinition *undoTarget = services.contacts->byId(contactId);
    check(undoTarget != nullptr
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     undoTarget->sourceScope, geometryRevision, sourceFaceId)
              && undoTarget->targetScope.entities.isEmpty(),
          "Undo Target Apply restores exact CAD Source and missing-Target authoring state");
    commands->stack()->redo();
    details->refresh();
    const ContactDefinition *redoTarget = services.contacts->byId(contactId);
    check(redoTarget != nullptr && services.contacts->validate(contactId).valid()
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     redoTarget->targetScope, geometryRevision, targetFaceId),
          "Redo Target Apply restores completed CAD Contact exactly");

    // --- Cancel: changed transient CAD selection must not persist --------------
    const int beforeCancelCount = commands->stack()->count();
    editTarget->click();
    const bool targetPreloaded = selectionCoordinator->contactEditActive()
        && selectionCoordinator->editingContactTarget()
        && selectionCoordinator->selectionManager()->items().size() == 1
        && selectionCoordinator->selectionManager()->items().front().domain == SelectionDomain::Geometry
        && selectionCoordinator->selectionManager()->items().front().geometryEntityId == targetFaceId;
    check(targetPreloaded,
          "editing an existing current Target safely preloads its exact persistent CAD Face identity");
    if (selectionCoordinator->contactEditActive()) {
        check(selectionCoordinator->selectionManager()->apply(sourceFaceSelection, SelectionOperation::Replace),
              "Cancel fixture changes only transient Target CAD selection");
    } else {
        check(false, "Cancel fixture changes only transient Target CAD selection");
    }
    check(commands->stack()->count() == beforeCancelCount,
          "changed transient Target CAD selection still creates no document transaction");
    cancelTarget->click();
    const ContactDefinition *afterCancel = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeCancelCount
              && afterCancel != nullptr
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     afterCancel->targetScope, geometryRevision, targetFaceId),
          "Cancel Target closes session with zero document mutation and preserves persisted CAD Target scope");

    // --- Clear buttons remain canonical persistent operations ------------------
    const ContactDefinition *beforeClear = services.contacts->byId(contactId);
    if (beforeClear == nullptr) {
        return failures + 1;
    }
    const ScopeReference source = beforeClear->sourceScope;
    const ScopeReference target = beforeClear->targetScope;
    const int beforeClearSourceCount = commands->stack()->count();
    clearSource->click();
    check(commands->stack()->count() == beforeClearSourceCount + 1
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope,
          "Clear Source widget creates exactly one persistent scope transaction");
    commands->stack()->undo();
    details->refresh();
    const ContactDefinition *undoClearSource = services.contacts->byId(contactId);
    check(undoClearSource != nullptr && services.contacts->validate(contactId).valid()
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     undoClearSource->sourceScope, source.sourceRevision,
                     source.entities.isEmpty() ? femcae::geometry::InvalidGeometryId
                                               : source.entities.front().geometryEntityId),
          "Undo Clear Source restores exact previous CAD Face identity");

    const int beforeClearTargetCount = commands->stack()->count();
    clearTarget->click();
    check(commands->stack()->count() == beforeClearTargetCount + 1
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope,
          "Clear Target widget creates exactly one persistent scope transaction");
    commands->stack()->undo();
    details->refresh();
    const ContactDefinition *undoClearTarget = services.contacts->byId(contactId);
    check(undoClearTarget != nullptr && services.contacts->validate(contactId).valid()
              && contact_inspector_acceptance_detail::isSingleGeometryFace(
                     undoClearTarget->targetScope, target.sourceRevision,
                     target.entities.isEmpty() ? femcae::geometry::InvalidGeometryId
                                               : target.entities.front().geometryEntityId),
          "Undo Clear Target restores exact previous CAD Face identity");

    // --- Mesh/Facet edit path ---------------------------------------------------
    // Unit/command persistence testleri Mesh scope oluşturmayı zaten doğruluyor.
    // Burada mevcut Contact'ı current Mesh scope'a seed edip Inspector'ın domain
    // inheritance, preload, Apply ve stale-generation repair davranışını test ederiz.
    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(services.mesh->generate(), "Contact Inspector Mesh fixture generates current FEM mesh");
    }
    check(services.mesh->mesh().boundaryFacets.size() >= 2,
          "Contact Inspector Mesh fixture exposes real boundary Facet identities");
    if (services.mesh->mesh().boundaryFacets.size() < 2) {
        return failures + 1;
    }
    const quint64 generation = services.mesh->generation();
    const auto sourceFacet = services.mesh->mesh().boundaryFacets.at(0).id;
    const auto targetFacet = services.mesh->mesh().boundaryFacets.at(1).id;
    const ScopeReference meshSource =
        contact_inspector_acceptance_detail::meshFacetScope(generation, sourceFacet);
    check(services.contacts->replaceSourceScope(contactId, meshSource)
              && services.contacts->replaceTargetScope(contactId, ScopeReference{}),
          "Mesh edit fixture switches Contact to current Source Facet plus missing Target without ID coercion");
    commands->resetHistory();
    details->refresh();

    editSource->click();
    const bool meshSourcePreloaded = selectionCoordinator->contactEditActive()
        && selectionCoordinator->editingContactSource()
        && graphics->selectionFilter() == SelectionFilter::Facet
        && selectionCoordinator->selectionManager()->items().size() == 1
        && selectionCoordinator->selectionManager()->items().front().domain == SelectionDomain::Mesh
        && selectionCoordinator->selectionManager()->items().front().meshEntityId == sourceFacet;
    check(meshSourcePreloaded,
          "existing Mesh Source opens Facet session and safely preloads exact current MeshEntityId");
    cancelSource->click();
    check(!selectionCoordinator->contactEditActive() && commands->stack()->count() == 0,
          "Cancel existing Mesh Source edit closes with zero document transaction");

    const int beforeMeshTargetEditCount = commands->stack()->count();
    editTarget->click();
    check(selectionCoordinator->contactEditActive()
              && selectionCoordinator->editingContactTarget()
              && graphics->selectionFilter() == SelectionFilter::Facet
              && selectionCoordinator->selectionManager()->items().isEmpty()
              && commands->stack()->count() == beforeMeshTargetEditCount,
          "missing Target inherits existing Source Mesh/Facet domain with exact Facet filter");
    const SelectionItem targetFacetSelection =
        contact_inspector_acceptance_detail::meshFacetSelection(generation, targetFacet);
    check(selectionCoordinator->selectionManager()->apply(targetFacetSelection, SelectionOperation::Replace),
          "Mesh Target edit accepts real current FEM Facet transient selection");
    check(commands->stack()->count() == beforeMeshTargetEditCount && applyTarget->isEnabled(),
          "transient Mesh Target selection remains outside document Undo history");
    applyTarget->click();
    const ContactDefinition *meshCompleted = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeMeshTargetEditCount + 1
              && meshCompleted != nullptr
              && contact_inspector_acceptance_detail::isSingleMeshFacet(
                     meshCompleted->sourceScope, generation, sourceFacet)
              && contact_inspector_acceptance_detail::isSingleMeshFacet(
                     meshCompleted->targetScope, generation, targetFacet)
              && services.contacts->validate(contactId).valid(),
          "Mesh Target Apply creates one persistent Facet transaction and completes valid Contact");

    // --- Stale Mesh scope never preloads old MeshEntityId ----------------------
    const quint64 oldGeneration = services.mesh->generation();
    const int beforeStaleEditCount = commands->stack()->count();
    check(services.mesh->generate() && services.mesh->generation() != oldGeneration,
          "mesh regeneration advances generation before stale Contact edit acceptance");
    const ContactDefinition *staleDefinition = services.contacts->byId(contactId);
    check(staleDefinition != nullptr
              && services.contacts->validate(contactId).error == ContactValidationError::SourceScopeInvalid
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
    const ContactDefinition *afterStaleCancel = services.contacts->byId(contactId);
    check(!selectionCoordinator->contactEditActive()
              && commands->stack()->count() == beforeStaleEditCount
              && afterStaleCancel != nullptr
              && afterStaleCancel->sourceScope.sourceRevision == oldGeneration,
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
