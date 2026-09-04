#pragma once

// Dynamics26 Alpha.3.6 — application-shell CAD + FEM selection acceptance helper.
//
// --selection-selftest geliştirici bayrağında gerçek SelectionCoordinator signal
// zinciri üzerinden transient Body/Face/Edge/Vertex ve Node/Element/Facet
// selection -> Navigator/Inspector senkronunu ve Alpha.3.6 Named Selection
// Edit/Apply/Cancel transaction akışını doğrular. Fiziksel mouse/trackpad kabulünün
// veya gerçek VTK pick testinin yerine geçmez.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/SelectionManager.h"
#include "../core/SelectionPolicy.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/ProjectNavigator.h"
#include "../shell/SelectionCoordinator.h"
#include "../viewport/ViewportSelectionBridge.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QPushButton>
#include <QUndoStack>

#include <iostream>
#include <string>

namespace d26 {

inline int runSelectionAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
{
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const std::string &message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };
    const auto flushUi = [&app] {
        app.processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        app.processEvents();
    };

    SelectionManager *selection = window.findChild<SelectionManager *>();
    SelectionCoordinator *coordinator = nullptr;
    for (QObject *child : window.children()) {
        if (auto *candidate = dynamic_cast<SelectionCoordinator *>(child)) {
            coordinator = candidate;
            break;
        }
    }
    const ServiceContext services = window.services();
    ProjectModel *project = services.project;
    ProjectNavigator *navigator = window.navigator();
    DetailsHost *details = window.detailsHost();
    GraphicsWorkspace *graphics = window.graphics();
    QUndoStack *undo = window.documentCommands()->stack();

    check(selection != nullptr, "SelectionManager is owned by the application composition tree");
    check(coordinator != nullptr,
          "SelectionCoordinator is owned directly by the application composition tree");
    check(project != nullptr && navigator != nullptr && details != nullptr && graphics != nullptr
              && undo != nullptr && services.geometry != nullptr && services.mesh != nullptr
              && services.namedSelections != nullptr,
          "selection acceptance has Project/Navigator/Details/Graphics/Undo/Geometry/Mesh/NamedSelection collaborators");
    if (selection == nullptr || coordinator == nullptr || project == nullptr || navigator == nullptr
        || details == nullptr || graphics == nullptr || undo == nullptr || services.geometry == nullptr
        || services.mesh == nullptr || services.namedSelections == nullptr) {
        return 1;
    }

    // Fiziksel Beta.3 regresyonu: New Project içindeki Parametric Box yalnız
    // gri display üçgeni değildir. Gerçek analytic Face/Edge/Vertex provenance
    // yayınlamalı ve aynı UI consumer üzerinden Named Selection üretmelidir.
    window.selectObject(project->geometryNode());
    flushUi();
    auto *geometryBridge = window.findChild<ViewportSelectionBridge *>();
    check(geometryBridge != nullptr && geometryBridge->hasFaceProvenance()
              && geometryBridge->hasEdgeProvenance() && geometryBridge->hasVertexProvenance(),
          "Parametric Box viewport publishes complete analytic topology provenance");
    check(graphics->filterAvailable(SelectionFilter::Face)
              && graphics->filterAvailable(SelectionFilter::Edge)
              && graphics->filterAvailable(SelectionFilter::Vertex),
          "Parametric Box keeps Face/Edge/Vertex selection toolbar actions enabled");

    const auto &parametricDocument = services.mesh->selectionGeometryDocument();
    const auto parametricBodies = parametricDocument.entitiesOfKind(
        femcae::geometry::GeometryEntityKind::Body);
    const auto parametricFaces = parametricDocument.entitiesOfKind(
        femcae::geometry::GeometryEntityKind::Face);
    if (geometryBridge != nullptr && parametricBodies.size() == 1 && parametricFaces.size() == 6) {
        graphics->setSelectionFilter(SelectionFilter::Face);
        emit geometryBridge->selectionRequested(
            SelectionKind::Face, parametricBodies.front(), parametricFaces.front(),
            SelectionOperation::Replace);
        flushUi();
        const NamedSelectionCreateResult created =
            coordinator->createNamedSelectionFromCurrentSelection(QStringLiteral("Parametric Face Scope"));
        check(created.success()
                  && services.namedSelections->validate(created.id)
                         == ScopeReferenceValidationError::None,
              "real Parametric Box Face selection creates a persistent Named Selection");
        if (created.success()) {
            undo->undo();
            flushUi();
            check(services.namedSelections->byId(created.id) == nullptr,
                  "one Undo removes the Parametric Face Named Selection transaction");
        }
    } else {
        check(false, "Parametric Box exposes one Body and six Face engineering identities");
    }

    constexpr qint64 geometryBodyId = 910001;
    constexpr qint64 geometryFaceId = 910101;
    constexpr qint64 geometryEdgeId = 910201;
    constexpr qint64 geometryVertexId = 910301;
    const ObjectId restoreObject = navigator->selectedObject();
    const int undoIndexBefore = undo->index();
    const ObjectId projectBody = project->addObject(project->geometryNode(), ObjectType::Body,
                                                    QStringLiteral("Alpha34 Sync Body"), geometryBodyId);
    check(projectBody != InvalidObjectId && static_cast<qint64>(projectBody) != geometryBodyId,
          "Project Body identity stays distinct from CAD GeometryEntityId");

    const quint64 revision = services.geometry->summary().revision;
    const auto topologyItem = [revision](const SelectionKind kind,
                                         const qint64 geometryId,
                                         const qint64 parentId) {
        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.kind = kind;
        item.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryId);
        item.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(parentId);
        item.sourceRevision = revision;
        return item;
    };

    SelectionItem body;
    body.domain = SelectionDomain::Geometry;
    body.kind = SelectionKind::Body;
    body.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryBodyId);
    body.sourceRevision = revision;
    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry));
    check(selection->apply(body, SelectionOperation::Replace),
          "viewport-originated Body selection changes transient selection state");
    app.processEvents();
    check(navigator->selectedObject() == projectBody,
          "Body selection synchronizes Project Navigator current object");
    check(details->currentObject() == projectBody,
          "Body selection synchronizes Inspector project-object context");
    check(undo->index() == undoIndexBefore,
          "transient Body selection does not create a document Undo entry");

    const auto checkChildSelection = [&](const SelectionPolicyPreset preset,
                                         const SelectionKind kind,
                                         const qint64 geometryId,
                                         const QString &kindText) {
        selection->setPolicy(SelectionPolicy::preset(preset));
        const SelectionItem item = topologyItem(kind, geometryId, geometryBodyId);
        check(selection->apply(item, SelectionOperation::Replace),
              QStringLiteral("%1 selection is accepted under its topology policy").arg(kindText).toStdString());
        app.processEvents();
        check(navigator->selectedObject() == projectBody,
              QStringLiteral("%1 selection keeps parent Body as Navigator current object").arg(kindText).toStdString());
        check(details->currentObject() == projectBody,
              QStringLiteral("%1 selection keeps parent Body as Inspector project-object context").arg(kindText).toStdString());
        if (QLabel *summary = details->findChild<QLabel *>(QStringLiteral("Dynamics26SelectionSummary"))) {
            check(!summary->isHidden() && summary->text().contains(kindText),
                  QStringLiteral("Inspector exposes committed %1 selection summary").arg(kindText).toStdString());
        } else {
            check(false, "Inspector selection-summary widget is discoverable for CAD acceptance testing");
        }
        check(undo->index() == undoIndexBefore,
              QStringLiteral("transient %1 selection does not create a document Undo entry").arg(kindText).toStdString());
    };

    checkChildSelection(SelectionPolicyPreset::SurfaceScope, SelectionKind::Face,
                        geometryFaceId, QStringLiteral("Face"));
    checkChildSelection(SelectionPolicyPreset::EdgeScope, SelectionKind::Edge,
                        geometryEdgeId, QStringLiteral("Edge"));
    checkChildSelection(SelectionPolicyPreset::VertexScope, SelectionKind::Vertex,
                        geometryVertexId, QStringLiteral("Vertex"));

    // FEM subentity'leri ProjectModel agacina ayri node olarak eklenmez. Transient
    // Node/Element/Facet selection current project-object baglamini Mesh dugumunde
    // tutar ve raw selection document Undo stack'ine girmez.
    const ObjectId meshObject = project->meshNode();
    constexpr quint64 meshGeneration = 77;
    const auto meshItem = [](const SelectionKind kind,
                             const femcae::meshing::MeshEntityId id,
                             const quint64 generation) {
        SelectionItem item;
        item.domain = SelectionDomain::Mesh;
        item.kind = kind;
        item.meshEntityId = id;
        item.sourceRevision = generation;
        return item;
    };

    const auto checkMeshSelection = [&](const SelectionPolicyPreset preset,
                                        const SelectionKind kind,
                                        const femcae::meshing::MeshEntityId id,
                                        const QString &kindText) {
        selection->setPolicy(SelectionPolicy::preset(preset));
        const SelectionItem item = meshItem(kind, id, meshGeneration);
        check(selection->apply(item, SelectionOperation::Replace),
              QStringLiteral("%1 selection is accepted under its FEM policy").arg(kindText).toStdString());
        app.processEvents();
        check(navigator->selectedObject() == meshObject,
              QStringLiteral("%1 selection keeps Mesh as Navigator current object").arg(kindText).toStdString());
        check(details->currentObject() == meshObject,
              QStringLiteral("%1 selection keeps Mesh as Inspector project-object context").arg(kindText).toStdString());
        if (QLabel *summary = details->findChild<QLabel *>(QStringLiteral("Dynamics26SelectionSummary"))) {
            check(!summary->isHidden() && summary->text().contains(kindText)
                      && summary->text().contains(QStringLiteral("Mesh Gen")),
                  QStringLiteral("Inspector exposes committed %1 FEM selection summary").arg(kindText).toStdString());
        } else {
            check(false, "Inspector selection-summary widget is discoverable for FEM acceptance testing");
        }
        check(undo->index() == undoIndexBefore,
              QStringLiteral("transient %1 selection does not create a document Undo entry").arg(kindText).toStdString());
    };

    check(meshObject != InvalidObjectId,
          "ProjectModel exposes the canonical Mesh project-object context");
    checkMeshSelection(SelectionPolicyPreset::MeshNodeScope, SelectionKind::Node, 920001,
                       QStringLiteral("Node"));

    SelectionItem secondNode = meshItem(SelectionKind::Node, 920002, meshGeneration);
    check(selection->apply(secondNode, SelectionOperation::Add)
              && selection->items().size() == 2
              && selection->primary().has_value()
              && selection->primary()->sameIdentity(secondNode),
          "multi-Node Add keeps the newly added Node as primary selection");
    app.processEvents();
    check(navigator->selectedObject() == meshObject && details->currentObject() == meshObject,
          "multi-Node selection preserves Mesh Navigator/Inspector context");
    check(undo->index() == undoIndexBefore,
          "multi-Node transient selection does not create a document Undo entry");

    checkMeshSelection(SelectionPolicyPreset::MeshElementScope, SelectionKind::Element, 930001,
                       QStringLiteral("Element"));
    checkMeshSelection(SelectionPolicyPreset::MeshFacetScope, SelectionKind::Facet, 940001,
                       QStringLiteral("Facet"));

    check(selection->invalidateMeshGeneration(meshGeneration + 1)
              && selection->items().isEmpty(),
          "mesh regeneration invalidates stale shell-level FEM selection state");
    check(undo->index() == undoIndexBefore,
          "mesh-selection invalidation remains outside document Undo history");

    // ---------------------------------------------------------------------
    // Alpha.3.6 Named Selection Edit Selection acceptance
    // ---------------------------------------------------------------------
    services.mesh->setDivisions(2, 1, 1);
    check(services.mesh->generate(),
          "Named Selection edit acceptance generates a real FEM mesh fixture");
    flushUi();

    const auto &editMesh = services.mesh->mesh();
    check(editMesh.nodes.size() >= 2,
          "edit acceptance fixture exposes at least two real FEM Node identities");
    if (editMesh.nodes.size() >= 2) {
        const quint64 editGeneration = services.mesh->generation();
        const SelectionItem firstRealNode = meshItem(SelectionKind::Node,
                                                     editMesh.nodes[0].id, editGeneration);
        const SelectionItem secondRealNode = meshItem(SelectionKind::Node,
                                                      editMesh.nodes[1].id, editGeneration);
        const NamedSelectionCreateResult named = services.namedSelections->createFromSelection(
            QVector<SelectionItem>{firstRealNode}, QStringLiteral("Edit Scope Acceptance"));
        check(named.success(),
              "real FEM Node scope creates Named Selection edit fixture");

        if (named.success()) {
            window.selectObject(named.id);
            flushUi();
            check(navigator->selectedObject() == named.id && details->currentObject() == named.id,
                  "Named Selection is current in Navigator and Details before edit session");
            check(graphics->viewport()->context() == ViewportContext::Mesh
                      && graphics->selectionFilter() == SelectionFilter::Node,
                  "normal persistent Node Named Selection resolves Mesh/Node viewport context from stored scope");
            check(selection->items().isEmpty(),
                  "normal persistent Named Selection overlay does not masquerade as transient SelectionManager state");

            QPushButton *editButton = details->findChild<QPushButton *>(
                QStringLiteral("Dynamics26NamedSelectionEdit"));
            check(editButton != nullptr,
                  "Named Selection Details exposes Edit Selection control");
            if (editButton != nullptr) {
                editButton->click();
                flushUi();
            }

            check(coordinator->namedSelectionEditActive()
                      && coordinator->editingNamedSelection() == named.id,
                  "Edit Selection control opens transient Named Selection edit session");
            check(graphics->viewport()->context() == ViewportContext::Mesh
                      && graphics->selectionFilter() == SelectionFilter::Node,
                  "Node Named Selection edit switches viewport to Mesh/Node selection context");
            check(selection->items().size() == 1
                      && selection->items().front().sameIdentity(firstRealNode)
                      && selection->items().front().sourceRevision == editGeneration,
                  "valid stored FEM scope safely preloads exact current Node identity");
            check(navigator->selectedObject() == named.id && details->currentObject() == named.id,
                  "preloaded edit selection does not replace Named Selection project-object context");

            const int undoBeforeApply = undo->index();
            check(selection->apply(secondRealNode, SelectionOperation::Replace),
                  "edit session accepts a different current FEM Node selection");
            flushUi();
            check(navigator->selectedObject() == named.id && details->currentObject() == named.id,
                  "transient edit selection keeps Navigator and Details on Named Selection");
            check(undo->index() == undoBeforeApply,
                  "transient edit selection itself creates no document Undo entry");

            QPushButton *applyButton = details->findChild<QPushButton *>(
                QStringLiteral("Dynamics26NamedSelectionApply"));
            QPushButton *cancelButtonWhileEditing = details->findChild<QPushButton *>(
                QStringLiteral("Dynamics26NamedSelectionCancel"));
            check(applyButton != nullptr && cancelButtonWhileEditing != nullptr,
                  "active edit session exposes Apply Selection and Cancel controls");
            if (applyButton != nullptr) {
                applyButton->click();
                flushUi();
            }

            check(!coordinator->namedSelectionEditActive()
                      && undo->index() == undoBeforeApply + 1,
                  "Apply Selection closes edit session and creates exactly one Undo transaction");
            const NamedSelectionDefinition *afterApply = services.namedSelections->byId(named.id);
            check(afterApply != nullptr && afterApply->scope.entities.size() == 1
                      && afterApply->scope.entities.front().kind == SelectionKind::Node
                      && afterApply->scope.entities.front().meshEntityId == secondRealNode.meshEntityId
                      && afterApply->scope.sourceRevision == editGeneration,
                  "Apply Selection persists only the newly selected real FEM Node scope");

            undo->undo();
            flushUi();
            const NamedSelectionDefinition *afterUndo = services.namedSelections->byId(named.id);
            check(afterUndo != nullptr
                      && afterUndo->scope.entities.front().meshEntityId == firstRealNode.meshEntityId,
                  "Undo restores previous Named Selection scope identity");
            undo->redo();
            flushUi();
            const NamedSelectionDefinition *afterRedo = services.namedSelections->byId(named.id);
            check(afterRedo != nullptr
                      && afterRedo->scope.entities.front().meshEntityId == secondRealNode.meshEntityId,
                  "Redo reapplies edited Named Selection scope identity");

            window.selectObject(named.id);
            flushUi();
            editButton = details->findChild<QPushButton *>(QStringLiteral("Dynamics26NamedSelectionEdit"));
            check(editButton != nullptr,
                  "Named Selection Details returns to Edit Selection control after Apply");
            if (editButton != nullptr) {
                editButton->click();
                flushUi();
            }
            check(coordinator->namedSelectionEditActive()
                      && selection->items().size() == 1
                      && selection->items().front().sameIdentity(secondRealNode),
                  "second edit session preloads the currently persisted scope");

            const int undoBeforeCancel = undo->index();
            check(selection->apply(firstRealNode, SelectionOperation::Replace),
                  "Cancel acceptance changes only transient selection before cancellation");
            flushUi();
            QPushButton *cancelButton = details->findChild<QPushButton *>(
                QStringLiteral("Dynamics26NamedSelectionCancel"));
            check(cancelButton != nullptr,
                  "active edit session keeps Cancel control discoverable after selection changes");
            if (cancelButton != nullptr) {
                cancelButton->click();
                flushUi();
            }
            const NamedSelectionDefinition *afterCancel = services.namedSelections->byId(named.id);
            check(!coordinator->namedSelectionEditActive()
                      && undo->index() == undoBeforeCancel
                      && afterCancel != nullptr
                      && afterCancel->scope.entities.front().meshEntityId == secondRealNode.meshEntityId,
                  "Cancel closes edit session without document mutation or Undo entry");

            const quint64 generationBeforeStale = services.mesh->generation();
            check(services.mesh->generate()
                      && services.mesh->generation() != generationBeforeStale,
                  "mesh regenerate advances generation before stale edit acceptance");
            flushUi();
            check(project->object(named.id) != nullptr
                      && project->object(named.id)->state == ObjectState::OutOfDate,
                  "mesh regeneration automatically marks stored Named Selection OutOfDate");

            window.selectObject(named.id);
            flushUi();
            check(graphics->viewport()->context() == ViewportContext::Mesh
                      && graphics->selectionFilter() == SelectionFilter::Node
                      && selection->items().isEmpty(),
                  "stale persistent Named Selection keeps current Mesh/Node context without preloading old generation IDs");
            editButton = details->findChild<QPushButton *>(QStringLiteral("Dynamics26NamedSelectionEdit"));
            check(editButton != nullptr,
                  "stale Named Selection still offers explicit Edit Selection repair workflow");
            if (editButton != nullptr) {
                editButton->click();
                flushUi();
            }
            check(coordinator->namedSelectionEditActive()
                      && coordinator->editPreloadError() == ScopeReferenceValidationError::StaleMeshGeneration
                      && selection->items().isEmpty(),
                  "stale mesh scope opens edit session with zero old-ID preload and explicit stale diagnostic");
            check(navigator->selectedObject() == named.id && details->currentObject() == named.id,
                  "stale repair session still preserves Named Selection project-object context");
            coordinator->cancelNamedSelectionEdit();
            flushUi();
        }
    }

    selection->clear();
    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry));
    project->removeObject(projectBody);
    if (restoreObject != InvalidObjectId && project->object(restoreObject) != nullptr) {
        window.selectObject(restoreObject);
    } else {
        window.selectObject(project->geometryNode());
    }
    flushUi();

    std::cout << (failures == 0 ? "selection shell acceptance PASS\n"
                                : "selection shell acceptance FAIL\n");
    std::cout << "checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
