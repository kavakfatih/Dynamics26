#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.3 — Mesh Inspector application acceptance.
//
// Gerçek MainWindow + DetailsHost + MeshService + DocumentCommandManager
// kompozisyonunu kullanır. Mesh definition document state'tir ve Undoable'dır;
// generated FEM mesh ise derived state'tir. Generate/Clear bu nedenle yeni Undo
// transaction'ı üretmez. Persistent FEM scope eski generation'a sessizce rebind
// edilmez ve Inspector bunu stale scope sayacıyla görünür kılar.

#include "../core/DocumentCommandManager.h"
#include "../details/MeshDetails.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QUndoStack>

#include <iostream>

namespace d26 {
namespace mesh_inspector_acceptance_detail {

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

} // namespace mesh_inspector_acceptance_detail

inline int runMeshInspectorAcceptanceTest(QApplication &app,
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
    MeshDetails *details = window.findChild<MeshDetails *>();
    check(services.project != nullptr && services.geometry != nullptr
              && services.mesh != nullptr && services.namedSelections != nullptr
              && window.documentCommands() != nullptr && details != nullptr,
          "Mesh Inspector acceptance has authoritative project/geometry/mesh/scope/Undo composition");
    if (services.project == nullptr || services.geometry == nullptr
        || services.mesh == nullptr || services.namedSelections == nullptr
        || window.documentCommands() == nullptr || details == nullptr) {
        return 1;
    }

    window.newProjectWithoutPrompt();
    services.namedSelections->clear();
    window.documentCommands()->resetHistory();
    window.selectObject(services.project->meshNode());
    details->refresh();

    auto *source = details->findChild<QComboBox *>(QStringLiteral("Dynamics26MeshSource"));
    auto *nx = details->findChild<QSpinBox *>(QStringLiteral("Dynamics26MeshNx"));
    auto *status = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshStatus"));
    auto *generation = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshGeneration"));
    auto *settingsRevision = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshSettingsRevision"));
    auto *sourceRevision = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshSourceGeometryRevision"));
    auto *meshedRevision = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshMeshedGeometryRevision"));
    auto *staleScopes = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshStaleScopes"));
    auto *nodes = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshNodes"));
    auto *elements = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshElements"));
    auto *facets = details->findChild<QLabel *>(QStringLiteral("Dynamics26MeshFacets"));
    auto *generate = details->findChild<QPushButton *>(QStringLiteral("Dynamics26MeshGenerate"));
    auto *clear = details->findChild<QPushButton *>(QStringLiteral("Dynamics26MeshClearGenerated"));

    check(source != nullptr && nx != nullptr && status != nullptr && generation != nullptr
              && settingsRevision != nullptr && sourceRevision != nullptr
              && meshedRevision != nullptr && staleScopes != nullptr
              && nodes != nullptr && elements != nullptr && facets != nullptr
              && generate != nullptr && clear != nullptr,
          "Mesh Inspector exposes stable Definition/Lifecycle/Statistics/Action bindings");
    if (source == nullptr || nx == nullptr || status == nullptr || generation == nullptr
        || settingsRevision == nullptr || sourceRevision == nullptr
        || meshedRevision == nullptr || staleScopes == nullptr
        || nodes == nullptr || elements == nullptr || facets == nullptr
        || generate == nullptr || clear == nullptr) {
        return failures + 1;
    }

    check(source->currentIndex() == 0 && !source->isEnabled()
              && status->text().contains(QStringLiteral("üretilmedi"), Qt::CaseInsensitive)
              && generation->text() == QString::number(services.mesh->generation())
              && settingsRevision->text() == QString::number(services.mesh->settingsRevision())
              && sourceRevision->text().contains(QStringLiteral("Parametric"), Qt::CaseInsensitive)
              && meshedRevision->text().contains(QStringLiteral("—"))
              && staleScopes->text() == QStringLiteral("0")
              && !clear->isEnabled(),
          "fresh Mesh Inspector reflects parametric no-mesh lifecycle without invented CAD revision");

    QUndoStack *stack = window.documentCommands()->stack();
    const MeshService::Definition initialDefinition = services.mesh->definition();
    const quint64 initialSettingsRevision = services.mesh->settingsRevision();
    const int beforeDefinitionIndex = stack->index();
    nx->setValue(initialDefinition.nx + 1);
    check(stack->index() == beforeDefinitionIndex + 1
              && services.mesh->definition().nx == initialDefinition.nx + 1
              && services.mesh->settingsRevision() > initialSettingsRevision,
          "Mesh Nx widget creates exactly one document definition transaction");

    stack->undo();
    details->refresh();
    check(stack->index() == beforeDefinitionIndex
              && services.mesh->definition() == initialDefinition
              && nx->value() == initialDefinition.nx,
          "Undo Mesh definition edit restores exact authoritative definition and widget value");

    const int beforeGenerateIndex = stack->index();
    const quint64 beforeGenerate = services.mesh->generation();
    generate->click();
    details->refresh();
    check(services.mesh->hasMesh() && services.mesh->isUpToDate()
              && services.mesh->generation() == beforeGenerate + 1
              && stack->index() == beforeGenerateIndex,
          "Generate Mesh creates derived FEM state and advances generation without document Undo entry");
    check(generation->text() == QString::number(services.mesh->generation())
              && nodes->text() == QString::number(services.mesh->nodeCount())
              && elements->text() == QString::number(services.mesh->elementCount())
              && facets->text() == QString::number(services.mesh->boundaryFacetCount())
              && status->text().contains(QStringLiteral("Up to date"), Qt::CaseInsensitive)
              && clear->isEnabled(),
          "generated Mesh Inspector reads real lifecycle and FEM statistics from MeshService");

    // Definition değişikliği generated mesh'i silmez; yalnız Out-of-Date yapar.
    // Undo aynı generatedDefinition içeriğine döndüğünde mesh tekrar current olur.
    const MeshService::Definition generatedDefinition = services.mesh->definition();
    const int beforeStaleEditIndex = stack->index();
    nx->setValue(generatedDefinition.nx + 2);
    details->refresh();
    check(stack->index() == beforeStaleEditIndex + 1 && services.mesh->isOutOfDate()
              && status->text().contains(QStringLiteral("Out of date"), Qt::CaseInsensitive),
          "Mesh definition edit marks existing generated mesh explicitly Out-of-Date");
    stack->undo();
    details->refresh();
    check(services.mesh->definition() == generatedDefinition && services.mesh->isUpToDate()
              && status->text().contains(QStringLiteral("Up to date"), Qt::CaseInsensitive),
          "Undo definition edit revalidates unchanged generated mesh by exact definition content");

    check(!services.mesh->mesh().boundaryFacets.empty(),
          "Mesh Inspector stale-scope fixture has a real FEM boundary Facet identity");
    if (services.mesh->mesh().boundaryFacets.empty()) {
        return failures + 1;
    }

    NamedSelectionDefinition scopeDefinition;
    scopeDefinition.name = QStringLiteral("Mesh Inspector Facet Scope");
    scopeDefinition.scope = mesh_inspector_acceptance_detail::meshFacetScope(
        services.mesh->generation(), services.mesh->mesh().boundaryFacets.front().id);
    const ObjectId scopeId = services.namedSelections->createWithScope(scopeDefinition);
    check(scopeId != InvalidObjectId
              && services.namedSelections->validate(scopeId) == ScopeReferenceValidationError::None,
          "Mesh Inspector fixture persists a current FEM Facet scope on current generation");
    details->refresh();
    check(staleScopes->text() == QStringLiteral("0"),
          "current FEM scope is not reported as stale before regeneration");

    const int beforeRegenerateIndex = stack->index();
    const quint64 scopeGeneration = services.mesh->generation();
    generate->click();
    details->refresh();
    check(stack->index() == beforeRegenerateIndex
              && services.mesh->generation() == scopeGeneration + 1
              && services.namedSelections->validate(scopeId)
                     == ScopeReferenceValidationError::StaleMeshGeneration,
          "regenerate advances FEM generation without Undo and makes old persistent scope stale");
    check(staleScopes->text() == QStringLiteral("1"),
          "Mesh Inspector surfaces stale persistent FEM scope count after regeneration");

    const MeshService::Definition beforeClearDefinition = services.mesh->definition();
    const quint64 beforeClearGeneration = services.mesh->generation();
    const int beforeClearIndex = stack->index();
    clear->click();
    details->refresh();
    check(!services.mesh->hasMesh()
              && services.mesh->generation() == beforeClearGeneration + 1
              && services.mesh->definition() == beforeClearDefinition
              && stack->index() == beforeClearIndex,
          "Clear Generated Mesh removes only derived FEM state, preserves definition and creates no Undo entry");
    check(status->text().contains(QStringLiteral("üretilmedi"), Qt::CaseInsensitive)
              && !clear->isEnabled() && staleScopes->text() == QStringLiteral("1"),
          "cleared Mesh Inspector shows no generated mesh while preserving explicit stale-scope lifecycle warning");

    services.namedSelections->remove(scopeId);
    window.documentCommands()->resetHistory();
    std::cout << "Mesh Inspector acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
