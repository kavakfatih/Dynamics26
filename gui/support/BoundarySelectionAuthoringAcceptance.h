#pragma once

// Dynamics26 V1.1.0-beta.3 / B3.1 — Face selection quick-authoring acceptance.
//
// Bu kabul gerçek application composition içindeki SelectionCoordinator yolunu
// çağırır. Transient CAD Face seçimi önce persistent Named Selection'a, ardından
// Fixed Support / Total Force içindeki ObjectId referansına dönüşmelidir. Raw CAD
// topology kimlikleri BC/Load nesnesine kopyalanmaz.

#include "../core/DocumentCommandManager.h"
#include "../core/SelectionManager.h"
#include "../core/SelectionPolicy.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/SelectionCoordinator.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>
#include <QTemporaryDir>
#include <QUndoStack>

#include <iostream>

namespace d26 {
namespace boundary_selection_acceptance_detail {

inline SelectionCoordinator *coordinator(Dynamics26MainWindow &window)
{
    for (QObject *child : window.children()) {
        if (auto *candidate = dynamic_cast<SelectionCoordinator *>(child)) {
            return candidate;
        }
    }
    return nullptr;
}

inline SelectionItem faceItem(const GeometryService &geometry,
                              const femcae::geometry::GeometryEntityId faceId,
                              const quint64 revision)
{
    SelectionItem item;
    item.domain = SelectionDomain::Geometry;
    item.kind = SelectionKind::Face;
    item.geometryEntityId = faceId;
    item.sourceRevision = revision;
    if (const auto *entity = geometry.document().find(faceId)) {
        item.parentGeometryId = entity->parentId;
    }
    return item;
}

inline SelectionPolicy facePolicy()
{
    SelectionPolicy policy = SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry);
    policy.allowedKinds = {SelectionKind::Face};
    policy.allowMultiple = true;
    return policy;
}

} // namespace boundary_selection_acceptance_detail

inline int runBoundarySelectionAuthoringAcceptanceTest(QApplication &app,
                                                        Dynamics26MainWindow &window)
{
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const char *message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };
    const auto flushUi = [&app] {
        app.processEvents();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        app.processEvents();
    };

    const ServiceContext services = window.services();
    SelectionCoordinator *selectionCoordinator =
        boundary_selection_acceptance_detail::coordinator(window);
    SelectionManager *selection = selectionCoordinator != nullptr
        ? selectionCoordinator->selectionManager() : nullptr;
    DocumentCommandManager *commands = window.documentCommands();
    QUndoStack *undo = commands != nullptr ? commands->stack() : nullptr;

    check(services.project != nullptr && services.geometry != nullptr
              && services.mesh != nullptr && services.analysis != nullptr
              && services.namedSelections != nullptr && selectionCoordinator != nullptr
              && selection != nullptr && undo != nullptr,
          "B3.1 acceptance has application selection/scope/analysis/Undo composition");
    if (services.project == nullptr || services.geometry == nullptr
        || services.mesh == nullptr || services.analysis == nullptr
        || services.namedSelections == nullptr || selectionCoordinator == nullptr
        || selection == nullptr || undo == nullptr) {
        return 1;
    }

    // Wrong-domain seçim hiçbir document mutation üretmeden reddedilir.
    if (!services.mesh->hasMesh()) {
        (void)services.mesh->generate();
    }
    check(services.mesh->hasMesh() && !services.mesh->mesh().nodes.empty(),
          "B3.1 rejection fixture has a real FEM Node identity");
    if (services.mesh->hasMesh() && !services.mesh->mesh().nodes.empty()) {
        SelectionItem node;
        node.domain = SelectionDomain::Mesh;
        node.kind = SelectionKind::Node;
        node.meshEntityId = services.mesh->mesh().nodes.front().id;
        node.sourceRevision = services.mesh->generation();
        selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::MeshNodeScope));
        (void)selection->apply(node, SelectionOperation::Replace);
        const int before = undo->index();
        const BoundaryFromSelectionCreateResult rejected =
            selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
                BoundaryFromSelectionKind::FixedSupport);
        check(!rejected.success() && rejected.buildError == ScopeReferenceBuildError::UnsupportedDomain
                  && undo->index() == before,
              "wrong-domain selection is rejected without an Undo entry");
    }

    // Gerçek CAD topology yalnız --import-step topology gate'inde mevcuttur.
    // OCCT'siz hosted gate bu pozitif yolu sahte topology ID ile taklit etmez.
    const auto bodies = services.geometry->bodies();
    if (bodies.size() != 1) {
        std::cout << "SKIP  B3.1 positive Face workflow requires one imported box-compatible CAD Body\n";
        std::cout << (failures == 0 ? "B3.1 boundary selection authoring acceptance PASS"
                                    : "B3.1 boundary selection authoring acceptance FAIL")
                  << " checks=" << checks << " failures=" << failures << '\n';
        return failures == 0 ? 0 : 1;
    }
    const auto descriptor = services.geometry->boxDescriptor(bodies.front());
    check(descriptor.has_value(),
          "B3.1 imported CAD Body is boxDescriptor-compatible");
    if (!descriptor.has_value()) {
        return failures + 1;
    }

    QTemporaryDir temporary;
    check(temporary.isValid(), "B3.1 save/reopen fixture owns a temporary project directory");
    if (!temporary.isValid()) {
        return failures + 1;
    }
    const QString baselinePath = temporary.filePath(QStringLiteral("b3_1_baseline.femcae.json"));
    const QString authoredPath = temporary.filePath(QStringLiteral("b3_1_authored.femcae.json"));
    check(window.saveProjectToPath(baselinePath),
          "B3.1 captures the pre-test application document for exact restoration");

    const quint64 revision = services.geometry->summary().revision;
    const SelectionItem xMin = boundary_selection_acceptance_detail::faceItem(
        *services.geometry, descriptor->xMinFace, revision);
    const SelectionItem xMax = boundary_selection_acceptance_detail::faceItem(
        *services.geometry, descriptor->xMaxFace, revision);
    const SelectionItem yMax = boundary_selection_acceptance_detail::faceItem(
        *services.geometry, descriptor->yMaxFace, revision);
    check(xMin.isValid() && xMax.isValid() && yMax.isValid(),
          "B3.1 CAD Face fixtures carry current revision and parent Body identity");

    // Stale CAD revision açıkça reddedilir; eski numeric Face kimliği kullanılmaz.
    SelectionItem stale = xMin;
    ++stale.sourceRevision;
    selection->setPolicy(boundary_selection_acceptance_detail::facePolicy());
    (void)selection->apply(stale, SelectionOperation::Replace);
    int before = undo->index();
    const BoundaryFromSelectionCreateResult staleRejected =
        selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
            BoundaryFromSelectionKind::FixedSupport);
    check(!staleRejected.success()
              && staleRejected.buildError == ScopeReferenceBuildError::StaleGeometryRevision
              && undo->index() == before,
          "stale Face selection is rejected without document mutation");

    // one Face -> Named Selection -> Fixed Support, tek Undo transaction.
    (void)selection->apply(xMin, SelectionOperation::Replace);
    before = undo->index();
    const BoundaryFromSelectionCreateResult supportCreated =
        selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
            BoundaryFromSelectionKind::FixedSupport);
    flushUi();
    const SupportDefinition *support = services.analysis->support(
        supportCreated.boundaryConditionId);
    const NamedSelectionDefinition *supportScope = services.namedSelections->byId(
        supportCreated.namedSelectionId);
    check(supportCreated.success() && undo->index() == before + 1,
          "one Face creates Fixed Support and Named Selection in one Undo transaction");
    check(support != nullptr
              && support->scopingMethod == BoundaryScopingMethod::NamedSelection
              && support->namedSelectionId == supportCreated.namedSelectionId
              && supportScope != nullptr && supportScope->scope.entities.size() == 1
              && supportScope->scope.entities.front().geometryEntityId == descriptor->xMinFace,
          "Fixed Support stores only the persistent Named Selection relationship");

    undo->undo();
    flushUi();
    check(services.analysis->support(supportCreated.boundaryConditionId) == nullptr
              && services.namedSelections->byId(supportCreated.namedSelectionId) == nullptr,
          "one Undo removes both Fixed Support and its persistent scope");
    undo->redo();
    flushUi();
    support = services.analysis->support(supportCreated.boundaryConditionId);
    check(support != nullptr && support->namedSelectionId == supportCreated.namedSelectionId
              && services.namedSelections->byId(supportCreated.namedSelectionId) != nullptr,
          "Redo restores the same Fixed Support/Named Selection ObjectIds and reference");

    // multi Face -> one Named Selection -> one Total Force. Total semantics are
    // one load object for the complete surface scope, never one per Face.
    selection->setPolicy(boundary_selection_acceptance_detail::facePolicy());
    (void)selection->apply(QVector<SelectionItem>{xMax, yMax}, SelectionOperation::Replace);
    before = undo->index();
    const BoundaryFromSelectionCreateResult forceCreated =
        selectionCoordinator->createBoundaryConditionFromCurrentFaceSelection(
            BoundaryFromSelectionKind::TotalForce);
    flushUi();
    const LoadDefinition *force = services.analysis->load(forceCreated.boundaryConditionId);
    const NamedSelectionDefinition *forceScope = services.namedSelections->byId(
        forceCreated.namedSelectionId);
    check(forceCreated.success() && undo->index() == before + 1,
          "multi Face creates one Total Force relationship in one Undo transaction");
    check(force != nullptr
              && force->scopingMethod == BoundaryScopingMethod::NamedSelection
              && force->namedSelectionId == forceCreated.namedSelectionId
              && forceScope != nullptr && forceScope->scope.entities.size() == 2
              && services.analysis->resolveBoundaryScope(*force).geometryFaceIds.size() == 2,
          "multi Face scope remains one persistent Named Selection referenced by one Force");

    undo->undo();
    flushUi();
    check(services.analysis->load(forceCreated.boundaryConditionId) == nullptr
              && services.namedSelections->byId(forceCreated.namedSelectionId) == nullptr,
          "multi Face Force and scope are removed by one Undo");
    undo->redo();
    flushUi();
    force = services.analysis->load(forceCreated.boundaryConditionId);
    check(force != nullptr && force->namedSelectionId == forceCreated.namedSelectionId
              && services.namedSelections->byId(forceCreated.namedSelectionId) != nullptr,
          "multi Face Redo restores exact Force/scope engineering identity");

    check(window.saveProjectToPath(authoredPath),
          "Face-authored support/load document saves through the real project path");
    window.newProjectWithoutPrompt();
    flushUi();
    check(window.openProjectFromPath(authoredPath),
          "Face-authored support/load document reopens through the real project path");
    flushUi();

    const SupportDefinition *reopenedSupport = services.analysis->support(
        supportCreated.boundaryConditionId);
    const LoadDefinition *reopenedForce = services.analysis->load(
        forceCreated.boundaryConditionId);
    const NamedSelectionDefinition *reopenedSupportScope = services.namedSelections->byId(
        supportCreated.namedSelectionId);
    const NamedSelectionDefinition *reopenedForceScope = services.namedSelections->byId(
        forceCreated.namedSelectionId);
    check(reopenedSupport != nullptr && reopenedForce != nullptr
              && reopenedSupport->namedSelectionId == supportCreated.namedSelectionId
              && reopenedForce->namedSelectionId == forceCreated.namedSelectionId
              && reopenedSupportScope != nullptr && reopenedSupportScope->scope.entities.size() == 1
              && reopenedForceScope != nullptr && reopenedForceScope->scope.entities.size() == 2,
          "save/reopen preserves BC/Load ObjectIds, Named Selection ObjectIds and scope cardinality");
    if (reopenedSupport != nullptr && reopenedForce != nullptr) {
        check(services.analysis->resolveBoundaryScope(*reopenedSupport).valid
                  && services.analysis->resolveBoundaryScope(*reopenedForce).valid,
              "save/reopen rebinds stable CAD identity and restores valid engineering relationships");
    }

    check(window.openProjectFromPath(baselinePath),
          "B3.1 restores the exact pre-test application document");
    flushUi();

    std::cout << (failures == 0 ? "B3.1 boundary selection authoring acceptance PASS"
                                : "B3.1 boundary selection authoring acceptance FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
