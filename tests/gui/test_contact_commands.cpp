#include "commands/ContactCommands.h"

#include <QCoreApplication>
#include <QUndoStack>

#include <iostream>
#include <string>

// Contact command regression'i production ContactCommands.h kodunu doğrudan
// çalıştırır. DomainCommand ortak tabanının bu testte gereken tek davranışı
// ServiceContext + undo text sahipliğidir; merge-window davranışı Contact create/
// delete/rename/scope transaction'larında kullanılmaz.
namespace d26::commands {
DomainCommand::DomainCommand(const ServiceContext &services, const QString &text)
    : QUndoCommand(text), services_(services), timestampMs_(0)
{
}
} // namespace d26::commands

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

d26::ScopeReference meshFacetScope(const quint64 generation,
                                   const femcae::meshing::MeshEntityId facetId)
{
    d26::ScopeEntityReference reference;
    reference.domain = d26::SelectionDomain::Mesh;
    reference.kind = d26::SelectionKind::Facet;
    reference.meshEntityId = facetId;

    d26::ScopeReference scope;
    scope.sourceRevision = generation;
    scope.entities.push_back(reference);
    return scope;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    Q_UNUSED(app);

    d26::ProjectModel project;
    d26::GeometryService geometry;
    d26::MeshService mesh(&geometry);
    d26::ContactService contacts(&project, &geometry, &mesh);

    mesh.setDivisions(1, 1, 1);
    check(mesh.generate(), "contact command fixture generates FEM mesh");
    check(mesh.mesh().boundaryFacets.size() >= 4,
          "fixture exposes enough independent boundary Facet identities");
    if (mesh.mesh().boundaryFacets.size() < 4) {
        return 1;
    }

    d26::ServiceContext services;
    services.project = &project;
    services.geometry = &geometry;
    services.mesh = &mesh;
    services.contacts = &contacts;

    const quint64 generation = mesh.generation();
    const auto sourceA = mesh.mesh().boundaryFacets.at(0).id;
    const auto targetA = mesh.mesh().boundaryFacets.at(1).id;
    const auto sourceB = mesh.mesh().boundaryFacets.at(2).id;
    const auto targetB = mesh.mesh().boundaryFacets.at(3).id;

    d26::ContactDefinition definition;
    definition.name = QStringLiteral("Contact Region");
    definition.sourceScope = meshFacetScope(generation, sourceA);
    definition.targetScope = meshFacetScope(generation, targetA);

    QUndoStack stack;
    auto *create = new d26::commands::CreateContactCommand(services, definition);
    stack.push(create);
    const d26::ObjectId contactId = create->createdId();
    check(contactId != d26::InvalidObjectId && contacts.byId(contactId) != nullptr,
          "CreateContactCommand creates persistent engineering object");
    check(project.typeOf(contactId) == d26::ObjectType::ContactRegion
              && project.parentOf(contactId) == project.connectionsNode(),
          "create command keeps ProjectModel tree identity under Connections");
    const int originalRow = contacts.rowOf(contactId);

    stack.undo();
    check(contacts.byId(contactId) == nullptr && project.object(contactId) == nullptr,
          "Undo create removes ContactService definition and tree identity");
    stack.redo();
    check(contacts.byId(contactId) != nullptr && contacts.rowOf(contactId) == originalRow,
          "Redo create restores same ObjectId and tree row");

    stack.push(new d26::commands::RenameContactCommand(
        services, contactId, QStringLiteral("Bonded Interface")));
    check(contacts.byId(contactId)->name == QStringLiteral("Bonded Interface")
              && project.object(contactId)->name == QStringLiteral("Bonded Interface"),
          "rename command synchronizes service definition and tree display name");
    stack.undo();
    check(contacts.byId(contactId)->name == QStringLiteral("Contact Region"),
          "Undo rename restores original engineering name");
    stack.redo();
    check(contacts.byId(contactId)->name == QStringLiteral("Bonded Interface"),
          "Redo rename restores final engineering name");

    const d26::ScopeReference originalSource = contacts.byId(contactId)->sourceScope;
    const d26::ScopeReference replacementSource = meshFacetScope(generation, sourceB);
    stack.push(new d26::commands::ReplaceContactSourceScopeCommand(
        services, contactId, replacementSource));
    check(contacts.byId(contactId)->sourceScope.entities.front().meshEntityId == sourceB,
          "source scope replacement is applied as one document command");
    stack.undo();
    check(contacts.byId(contactId)->sourceScope.entities.front().meshEntityId
              == originalSource.entities.front().meshEntityId,
          "Undo source scope restores previous engineering Facet identity");
    stack.redo();
    check(contacts.byId(contactId)->sourceScope.entities.front().meshEntityId == sourceB,
          "Redo source scope restores replacement identity");

    const d26::ScopeReference originalTarget = contacts.byId(contactId)->targetScope;
    const d26::ScopeReference replacementTarget = meshFacetScope(generation, targetB);
    stack.push(new d26::commands::ReplaceContactTargetScopeCommand(
        services, contactId, replacementTarget));
    check(contacts.byId(contactId)->targetScope.entities.front().meshEntityId == targetB,
          "target scope replacement is applied as one document command");
    stack.undo();
    check(contacts.byId(contactId)->targetScope.entities.front().meshEntityId
              == originalTarget.entities.front().meshEntityId,
          "Undo target scope restores previous engineering Facet identity");
    stack.redo();
    check(contacts.byId(contactId)->targetScope.entities.front().meshEntityId == targetB,
          "Redo target scope restores replacement identity");

    contacts.setSuppressed(contactId, true);
    const d26::ContactDefinition beforeDelete = *contacts.byId(contactId);
    const int rowBeforeDelete = contacts.rowOf(contactId);
    stack.push(new d26::commands::DeleteContactCommand(services, contactId));
    check(contacts.byId(contactId) == nullptr && project.object(contactId) == nullptr,
          "DeleteContactCommand removes contact engineering state");
    stack.undo();
    const d26::ContactDefinition *restored = contacts.byId(contactId);
    check(restored != nullptr && contacts.rowOf(contactId) == rowBeforeDelete,
          "Undo delete restores same ObjectId and tree row");
    check(restored != nullptr
              && restored->sourceScope.entities.front().meshEntityId
                     == beforeDelete.sourceScope.entities.front().meshEntityId
              && restored->targetScope.entities.front().meshEntityId
                     == beforeDelete.targetScope.entities.front().meshEntityId,
          "Undo delete restores exact source/target engineering scopes");
    check(project.isSuppressed(contactId)
              && project.object(contactId)->state == d26::ObjectState::Suppressed,
          "Undo delete restores suppressed document state");

    stack.redo();
    check(contacts.byId(contactId) == nullptr,
          "Redo delete removes same Contact ObjectId again");
    stack.undo();
    check(contacts.byId(contactId) != nullptr && project.isSuppressed(contactId),
          "second Undo delete remains deterministic");

    d26::ContactDefinition duplicateDefinition = definition;
    auto *duplicate = new d26::commands::CreateContactCommand(services, duplicateDefinition);
    stack.push(duplicate);
    const d26::ObjectId duplicateId = duplicate->createdId();
    check(duplicateId != d26::InvalidObjectId && duplicateId != contactId
              && contacts.byId(duplicateId)->name == QStringLiteral("Contact Region"),
          "create command can add a second independent ContactRegion");
    stack.undo();
    stack.redo();
    check(contacts.byId(duplicateId) != nullptr
              && contacts.byId(duplicateId)->name == QStringLiteral("Contact Region"),
          "duplicate create redo preserves assigned ObjectId and final name");

    std::cout << "Contact document command contract: "
              << (failures == 0 ? "PASS" : "FAIL") << '\n';
    return failures == 0 ? 0 : 1;
}
