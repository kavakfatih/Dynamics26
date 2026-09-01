#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — application-level Contact persistence
// acceptance.
//
// Bu test ContactService::toJson/fromJson'i yeniden taklit etmez. Gerçek
// Dynamics26MainWindow saveProjectToPath() -> newProjectWithoutPrompt() ->
// openProjectFromPath() zincirini çalıştırır. Böylece document composition,
// ProjectModel ObjectId restore ve Contact scope persistence aynı kabul testinde
// doğrulanır.
//
// Üretilmiş FEM mesh proje dosyasının MODEL STATE'i değildir ve saklanmaz.
// Bu nedenle Mesh/Facet Contact scope'u proje yeniden açıldığında eski generation
// kimliğini korumalı fakat current mesh'e ASLA sessizce rebind edilmemelidir.

#include "../core/DocumentCommandManager.h"
#include "../services/ContactService.h"
#include "../services/MeshService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>

#include <iostream>
#include <string>

namespace d26 {
namespace contact_persistence_acceptance_detail {

inline ScopeReference facetScope(const quint64 generation,
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

} // namespace contact_persistence_acceptance_detail

inline int runContactPersistenceAcceptanceTest(QApplication &app,
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
              && services.contacts != nullptr && window.documentCommands() != nullptr,
          "Contact persistence acceptance has Project/Mesh/Contact/document collaborators");
    if (services.project == nullptr || services.mesh == nullptr
        || services.contacts == nullptr || window.documentCommands() == nullptr) {
        return 1;
    }

    // Bu acceptance kendi persistent Contact fixture'ını kurar. Önceki shell
    // testlerinden Contact kalmışsa onları document reset davranışını taklit
    // etmeden doğrudan servis üzerinden temizleriz; bu test UI authoring testi
    // değildir.
    services.contacts->clear();
    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(services.mesh->generate(),
              "Contact persistence fixture generates a current FEM mesh");
    }
    check(services.mesh->mesh().boundaryFacets.size() >= 2,
          "Contact persistence fixture exposes two real FEM Facet identities");
    if (services.mesh->mesh().boundaryFacets.size() < 2) {
        return failures + 1;
    }

    constexpr ObjectId requestedContactId = 9007199254741999ULL; // > 2^53
    const quint64 savedGeneration = services.mesh->generation();
    const auto savedSourceFacet = services.mesh->mesh().boundaryFacets.at(0).id;
    const auto savedTargetFacet = services.mesh->mesh().boundaryFacets.at(1).id;

    ContactDefinition definition;
    definition.name = QStringLiteral("Persistence Contact");
    definition.formulation = ContactFormulation::Bonded;
    definition.sourceScope = contact_persistence_acceptance_detail::facetScope(
        savedGeneration, savedSourceFacet);
    definition.targetScope = contact_persistence_acceptance_detail::facetScope(
        savedGeneration, savedTargetFacet);

    const ObjectId contactId = services.contacts->createContact(
        definition, -1, requestedContactId);
    check(contactId == requestedContactId,
          "Contact fixture restores a requested ObjectId above IEEE-754 exact integer range");
    const int savedRow = services.contacts->rowOf(contactId);
    check(services.contacts->validate(contactId).valid(),
          "Contact fixture is valid against the current FEM mesh before save");

    QTemporaryDir temporary;
    check(temporary.isValid(), "Contact persistence temporary directory created");
    if (!temporary.isValid()) {
        return failures + 1;
    }
    const QString projectPath = temporary.filePath(
        QStringLiteral("contact-persistence.femcae.json"));
    check(window.saveProjectToPath(projectPath),
          "application save path persists Contact document state");

    QFile savedFile(projectPath);
    check(savedFile.open(QIODevice::ReadOnly),
          "saved Contact project can be inspected as JSON");
    QJsonDocument savedDocument;
    if (savedFile.isOpen()) {
        savedDocument = QJsonDocument::fromJson(savedFile.readAll());
        savedFile.close();
    }
    check(savedDocument.isObject(), "saved Contact project JSON is structurally readable");

    QJsonObject savedRoot = savedDocument.object();
    QJsonObject documentObject = savedRoot.value(QStringLiteral("dynamics26_document")).toObject();
    QJsonObject contactsObject = documentObject.value(QStringLiteral("contacts")).toObject();
    QJsonArray items = contactsObject.value(QStringLiteral("items")).toArray();
    check(items.size() == 1, "application project JSON contains exactly one Contact item");
    if (items.size() == 1) {
        const QJsonObject item = items.at(0).toObject();
        const QJsonObject source = item.value(QStringLiteral("source_scope")).toObject();
        const QJsonObject target = item.value(QStringLiteral("target_scope")).toObject();
        const QJsonArray sourceEntities = source.value(QStringLiteral("entities")).toArray();
        const QJsonArray targetEntities = target.value(QStringLiteral("entities")).toArray();
        check(item.value(QStringLiteral("object_id")).isString()
                  && item.value(QStringLiteral("object_id")).toString()
                         == QString::number(requestedContactId),
              "application JSON stores Contact ObjectId >2^53 as exact decimal string");
        check(source.value(QStringLiteral("source_revision")).isString()
                  && source.value(QStringLiteral("source_revision")).toString()
                         == QString::number(savedGeneration),
              "application JSON stores mesh generation as exact decimal string");
        check(sourceEntities.size() == 1 && targetEntities.size() == 1
                  && sourceEntities.at(0).toObject().value(QStringLiteral("mesh_entity_id")).isString()
                  && targetEntities.at(0).toObject().value(QStringLiteral("mesh_entity_id")).isString(),
              "application JSON stores FEM Facet identities as strings, never JSON double IDs");
    }

    window.newProjectWithoutPrompt();
    check(services.contacts->count() == 0,
          "new-project reset clears ContactService before ProjectModel identity reset");
    check(window.openProjectFromPath(projectPath),
          "application open path restores project containing Contact data");

    const ContactDefinition *restored = services.contacts->byId(requestedContactId);
    check(restored != nullptr,
          "Contact project reopen restores the exact >2^53 ObjectId");
    check(restored != nullptr && services.contacts->rowOf(requestedContactId) == savedRow,
          "Contact project reopen preserves Connections tree ordering");
    check(restored != nullptr && restored->name == definition.name,
          "Contact project reopen preserves engineering display name");
    check(restored != nullptr
              && restored->sourceScope.sourceRevision == savedGeneration
              && restored->targetScope.sourceRevision == savedGeneration
              && restored->sourceScope.entities.front().meshEntityId == savedSourceFacet
              && restored->targetScope.entities.front().meshEntityId == savedTargetFacet,
          "Contact project reopen preserves exact original generation and FEM Facet identities");

    const ContactValidationResult reopenedValidation = services.contacts->validate(requestedContactId);
    check(!reopenedValidation.valid()
              && (reopenedValidation.sourceScopeError
                      == ScopeReferenceValidationError::StaleMeshGeneration
                  || reopenedValidation.targetScopeError
                      == ScopeReferenceValidationError::StaleMeshGeneration),
          "reopened Mesh/Facet Contact is stale instead of silently rebinding to a new mesh generation");
    check(services.project->object(requestedContactId) != nullptr
              && services.project->object(requestedContactId)->state == ObjectState::OutOfDate,
          "stale reopened ContactRegion is surfaced as OutOfDate in ProjectModel");
    check(!window.documentCommands()->isDirty(),
          "successful project reopen leaves document history clean");

    // Corrupt one persisted 64-bit ObjectId beyond uint64 max. The application
    // loader must reject it explicitly; truncation, double conversion or partial
    // Contact restore is not acceptable.
    if (items.size() == 1) {
        QJsonObject corruptItem = items.at(0).toObject();
        corruptItem[QStringLiteral("object_id")] =
            QStringLiteral("18446744073709551616"); // uint64 max + 1
        items[0] = corruptItem;
        contactsObject[QStringLiteral("items")] = items;
        documentObject[QStringLiteral("contacts")] = contactsObject;
        savedRoot[QStringLiteral("dynamics26_document")] = documentObject;

        const QString corruptPath = temporary.filePath(
            QStringLiteral("contact-overflow.femcae.json"));
        QFile corruptFile(corruptPath);
        const bool wroteCorruptFile = corruptFile.open(QIODevice::WriteOnly | QIODevice::Truncate)
            && corruptFile.write(QJsonDocument(savedRoot).toJson(QJsonDocument::Indented)) > 0;
        corruptFile.close();
        check(wroteCorruptFile, "overflow Contact project fixture written");
        check(!window.openProjectFromPath(corruptPath),
              "application loader rejects Contact ObjectId uint64 overflow");
        check(services.contacts->count() == 0
                  && services.project->object(requestedContactId) == nullptr,
              "failed Contact load leaves no partially restored persistent Contact state");
        check(!window.documentCommands()->isDirty(),
              "failed Contact load returns to a clean safe new-project state");
    }

    std::cout << "Contact project persistence acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
