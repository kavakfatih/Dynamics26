#pragma once

// Dynamics26 Alpha.3.6 — Fixed Support / Force persistent scope consumer
// acceptance. Bu test fiziksel pointer UX testi değildir; gerçek application
// servisleri üzerinde persistence, Details -> DomainCommand -> Undo/Redo,
// preflight, dependency state ve solver regresyonunu çalıştırır. OCCT topology
// gate gerçek STEP import ettiğinde aynı test valid CAD Face Named Selection ->
// FEM Mesh -> solver zincirini de yürütür.

#include "../core/DependencyEngine.h"
#include "../core/ProjectModel.h"
#include "../core/ScopeReferenceBuilder.h"
#include "../core/SelectionTypes.h"
#include "../details/BoundaryConditionDetails.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QEvent>
#include <QLabel>
#include <QUndoStack>

#include <cmath>
#include <iostream>
#include <string>

namespace d26 {

inline int runBoundaryConsumerAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
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

    const ServiceContext services = window.services();
    check(services.project != nullptr && services.geometry != nullptr && services.mesh != nullptr
              && services.analysis != nullptr && services.namedSelections != nullptr
              && services.dependencies != nullptr,
          "boundary consumer acceptance has Project/Geometry/Mesh/Analysis/NamedSelection/Dependency collaborators");
    if (services.project == nullptr || services.geometry == nullptr || services.mesh == nullptr
        || services.analysis == nullptr || services.namedSelections == nullptr
        || services.dependencies == nullptr) {
        return 1;
    }

    // ------------------------------------------------------------------
    // Exact 64-bit persistence contract
    // ------------------------------------------------------------------
    const ObjectId hugeNamedSelectionId = static_cast<ObjectId>((quint64{1} << 53) + 731ULL);

    SupportDefinition supportRoundTrip;
    supportRoundTrip.name = QStringLiteral("Huge NS Support");
    supportRoundTrip.scopingMethod = BoundaryScopingMethod::NamedSelection;
    supportRoundTrip.namedSelectionId = hugeNamedSelectionId;
    const QJsonObject supportJson = supportRoundTrip.toJson();
    const SupportDefinition restoredSupport = SupportDefinition::fromJson(supportJson);
    check(supportJson.value(QStringLiteral("named_selection_id")).isString()
              && restoredSupport.scopingMethod == BoundaryScopingMethod::NamedSelection
              && restoredSupport.namedSelectionId == hugeNamedSelectionId,
          "Fixed Support persists Named Selection ObjectId >2^53 as exact decimal string");

    LoadDefinition loadRoundTrip;
    loadRoundTrip.name = QStringLiteral("Huge NS Force");
    loadRoundTrip.scopingMethod = BoundaryScopingMethod::NamedSelection;
    loadRoundTrip.namedSelectionId = hugeNamedSelectionId;
    loadRoundTrip.fxN = 1234.5;
    const QJsonObject loadJson = loadRoundTrip.toJson();
    const LoadDefinition restoredLoad = LoadDefinition::fromJson(loadJson);
    check(loadJson.value(QStringLiteral("named_selection_id")).isString()
              && restoredLoad.scopingMethod == BoundaryScopingMethod::NamedSelection
              && restoredLoad.namedSelectionId == hugeNamedSelectionId
              && qFuzzyCompare(restoredLoad.fxN, loadRoundTrip.fxN),
          "Force persists Named Selection ObjectId >2^53 without IEEE-754 precision loss");

    // Legacy files without scoping_method remain Geometry Selection.
    QJsonObject legacySupport;
    legacySupport[QStringLiteral("name")] = QStringLiteral("Legacy Support");
    legacySupport[QStringLiteral("scope")] = static_cast<int>(BoxFace::XMin);
    const SupportDefinition restoredLegacy = SupportDefinition::fromJson(legacySupport);
    check(restoredLegacy.scopingMethod == BoundaryScopingMethod::GeometrySelection
              && restoredLegacy.namedSelectionId == InvalidObjectId,
          "legacy boundary JSON without scoping method remains Geometry Selection");

    // ------------------------------------------------------------------
    // Real mesh + application consumer fixtures
    // ------------------------------------------------------------------
    services.mesh->setDivisions(2, 1, 1);
    check(services.mesh->generate(),
          "boundary consumer acceptance generates a real FEM mesh fixture");
    flushUi();

    const QVector<ObjectId> analyses = services.project->analyses();
    check(!analyses.isEmpty(), "application exposes an analysis for boundary consumer acceptance");
    if (analyses.isEmpty()) {
        return 1;
    }
    const ObjectId analysisId = analyses.first();
    const AnalysisRecord *record = services.analysis->analysis(analysisId);
    check(record != nullptr && !record->supports.isEmpty() && !record->loads.isEmpty(),
          "Static Structural analysis exposes default Fixed Support and Force consumers");
    if (record == nullptr || record->supports.isEmpty() || record->loads.isEmpty()) {
        return 1;
    }
    const ObjectId supportId = record->supports.first();
    const ObjectId loadId = record->loads.first();
    const SupportDefinition originalSupport = *services.analysis->support(supportId);
    const LoadDefinition originalLoad = *services.analysis->load(loadId);

    // ------------------------------------------------------------------
    // Boundary Details -> DomainCommand -> Undo/Redo acceptance
    // ------------------------------------------------------------------
    // Programatik setCurrentIndex burada fiziksel pointer UX yerine Qt widget
    // sinyal yolunu çalıştırır. Amaç Details'ın servisi doğrudan mutate etmediğini,
    // her kullanıcı-semantik değişiminin tam bir Undo transaction ürettiğini
    // kanıtlamaktır.
    QUndoStack *undoStack = window.documentCommands() != nullptr
        ? window.documentCommands()->stack() : nullptr;
    DetailsHost *detailsHost = window.detailsHost();
    BoundaryConditionDetails *boundaryPage = detailsHost != nullptr
        ? detailsHost->boundaryConditionPage() : nullptr;
    check(undoStack != nullptr && boundaryPage != nullptr,
          "boundary Details acceptance exposes real Details page and document Undo stack");

    const auto exerciseScopingMethodUndoRedo = [&](const ObjectId objectId, const bool expectLoad,
                                                    const std::string &subject) {
        if (undoStack == nullptr || boundaryPage == nullptr) {
            check(false, subject + " Details interaction prerequisites are available");
            return;
        }

        window.selectObject(objectId);
        flushUi();
        auto *methodCombo = boundaryPage->findChild<QComboBox *>(
            QStringLiteral("Dynamics26BoundaryScopingMethod"));
        auto *resolvedLabel = boundaryPage->findChild<QLabel *>(
            QStringLiteral("Dynamics26BoundaryScopeResolved"));
        check(methodCombo != nullptr && resolvedLabel != nullptr,
              subject + " Details exposes scoping method and resolved-scope widgets");
        if (methodCombo == nullptr || resolvedLabel == nullptr) {
            return;
        }

        const int geometryIndex = methodCombo->findData(
            static_cast<int>(BoundaryScopingMethod::GeometrySelection));
        const int namedIndex = methodCombo->findData(
            static_cast<int>(BoundaryScopingMethod::NamedSelection));
        check(geometryIndex >= 0 && namedIndex >= 0
                  && methodCombo->currentIndex() == geometryIndex,
              subject + " starts from persisted Geometry Selection state");
        if (geometryIndex < 0 || namedIndex < 0) {
            return;
        }

        const int beforeIndex = undoStack->index();
        methodCombo->setCurrentIndex(namedIndex);
        flushUi();

        const bool changedToNamed = expectLoad
            ? (services.analysis->load(objectId) != nullptr
               && services.analysis->load(objectId)->scopingMethod == BoundaryScopingMethod::NamedSelection
               && services.analysis->load(objectId)->namedSelectionId == InvalidObjectId)
            : (services.analysis->support(objectId) != nullptr
               && services.analysis->support(objectId)->scopingMethod == BoundaryScopingMethod::NamedSelection
               && services.analysis->support(objectId)->namedSelectionId == InvalidObjectId);
        check(changedToNamed && undoStack->index() == beforeIndex + 1,
              subject + " scoping-method widget creates exactly one undoable DomainCommand");
        check(resolvedLabel->text().contains(QStringLiteral("Named Selection seçilmedi")),
              subject + " unresolved Named Selection is surfaced instead of legacy face fallback");

        undoStack->undo();
        flushUi();
        const bool undoRestored = expectLoad
            ? (services.analysis->load(objectId) != nullptr
               && services.analysis->load(objectId)->scopingMethod == BoundaryScopingMethod::GeometrySelection)
            : (services.analysis->support(objectId) != nullptr
               && services.analysis->support(objectId)->scopingMethod == BoundaryScopingMethod::GeometrySelection);
        check(undoRestored && undoStack->index() == beforeIndex
                  && methodCombo->currentIndex() == geometryIndex,
              subject + " Undo restores model and Details widget state atomically");

        undoStack->redo();
        flushUi();
        const bool redoRestored = expectLoad
            ? (services.analysis->load(objectId) != nullptr
               && services.analysis->load(objectId)->scopingMethod == BoundaryScopingMethod::NamedSelection)
            : (services.analysis->support(objectId) != nullptr
               && services.analysis->support(objectId)->scopingMethod == BoundaryScopingMethod::NamedSelection);
        check(redoRestored && undoStack->index() == beforeIndex + 1
                  && methodCombo->currentIndex() == namedIndex,
              subject + " Redo reapplies persistent consumer state and Details state");

        // Acceptance sonraki resolver/solver testlerine original model ile girer.
        undoStack->undo();
        flushUi();
        const bool finalRestored = expectLoad
            ? (services.analysis->load(objectId) != nullptr
               && services.analysis->load(objectId)->scopingMethod == originalLoad.scopingMethod
               && services.analysis->load(objectId)->scope == originalLoad.scope
               && services.analysis->load(objectId)->namedSelectionId == originalLoad.namedSelectionId)
            : (services.analysis->support(objectId) != nullptr
               && services.analysis->support(objectId)->scopingMethod == originalSupport.scopingMethod
               && services.analysis->support(objectId)->scope == originalSupport.scope
               && services.analysis->support(objectId)->namedSelectionId == originalSupport.namedSelectionId);
        check(finalRestored,
              subject + " acceptance leaves persistent consumer definition unchanged");
    };

    exerciseScopingMethodUndoRedo(supportId, false, "Fixed Support");
    exerciseScopingMethodUndoRedo(loadId, true, "Force");

    // ------------------------------------------------------------------
    // Invalid consumer behavior
    // ------------------------------------------------------------------
    const auto &mesh = services.mesh->mesh();
    check(!mesh.nodes.empty(), "boundary consumer fixture contains real FEM Node identity");
    if (mesh.nodes.empty()) {
        return 1;
    }

    SelectionItem nodeItem;
    nodeItem.domain = SelectionDomain::Mesh;
    nodeItem.kind = SelectionKind::Node;
    nodeItem.meshEntityId = mesh.nodes.front().id;
    nodeItem.sourceRevision = services.mesh->generation();
    const NamedSelectionCreateResult meshNamed = services.namedSelections->createFromSelection(
        QVector<SelectionItem>{nodeItem}, QStringLiteral("BC Wrong Domain Fixture"));
    check(meshNamed.success(), "real FEM Node scope creates wrong-domain consumer fixture");

    if (meshNamed.success()) {
        SupportDefinition wrongDomainSupport = originalSupport;
        wrongDomainSupport.scopingMethod = BoundaryScopingMethod::NamedSelection;
        wrongDomainSupport.namedSelectionId = meshNamed.id;
        services.analysis->updateSupport(supportId, wrongDomainSupport);

        const BoundaryScopeResolution wrongDomainResolution =
            services.analysis->resolveBoundaryScope(wrongDomainSupport);
        check(!wrongDomainResolution.valid,
              "Fixed Support rejects Mesh Node Named Selection instead of coercing it to CAD Face");
        check(!services.analysis->preflight(analysisId).passed(),
              "wrong-domain Named Selection fails preflight before solver execution");

        // Regeneration makes the referenced mesh scope stale. Resolver must
        // surface StaleMeshGeneration before any attempted entity coercion.
        check(services.mesh->generate(), "mesh regeneration creates a new generation for stale-scope test");
        flushUi();
        const BoundaryScopeResolution staleResolution = services.analysis->resolveBoundaryScope(wrongDomainSupport);
        check(!staleResolution.valid
                  && staleResolution.validationError == ScopeReferenceValidationError::StaleMeshGeneration,
              "boundary consumer detects stale referenced Named Selection generation");
        services.dependencies->evaluate();
        const ProjectObject *supportObject = services.project->object(supportId);
        check(supportObject != nullptr && supportObject->state == ObjectState::OutOfDate,
              "DependencyEngine propagates stale Named Selection to Fixed Support OutOfDate state");

        // Dangling reference is an explicit error, never a fallback to legacy face.
        SupportDefinition danglingSupport = originalSupport;
        danglingSupport.scopingMethod = BoundaryScopingMethod::NamedSelection;
        danglingSupport.namedSelectionId = hugeNamedSelectionId;
        services.analysis->updateSupport(supportId, danglingSupport);
        const BoundaryScopeResolution danglingResolution = services.analysis->resolveBoundaryScope(danglingSupport);
        check(!danglingResolution.valid,
              "dangling Named Selection ObjectId is rejected without geometry fallback");
        services.dependencies->evaluate();
        supportObject = services.project->object(supportId);
        check(supportObject != nullptr && supportObject->state == ObjectState::Error,
              "DependencyEngine marks dangling Named Selection consumer as Error");

        (void)services.namedSelections->remove(meshNamed.id);
    }

    // Restore legacy Geometry Selection consumers and prove the pre-existing
    // solver path still executes after the resolver refactor.
    services.analysis->updateSupport(supportId, originalSupport);
    services.analysis->updateLoad(loadId, originalLoad);
    check(services.mesh->generate(), "legacy solver regression uses a current FEM mesh");
    services.dependencies->evaluate();
    const PreflightReport baselinePreflight = services.analysis->preflight(analysisId);
    check(baselinePreflight.passed(),
          "legacy Geometry Selection Fixed Support / Force still pass preflight");
    check(services.analysis->solve(analysisId),
          "legacy Geometry Selection Static Structural solve still completes after consumer refactor");

    // ------------------------------------------------------------------
    // OCCT-enabled topology gate: valid CAD Face NS -> FEM -> solver
    // ------------------------------------------------------------------
    // Normal hosted/self-hosted acceptance parametric geometry ile çalışabilir.
    // Topology workflow uygulamayı --import-step ile başlattığında bu blok gerçek
    // B-Rep identity/provenance zincirini ayrıca doğrular.
    if (services.geometry->summary().hasGeometry) {
        const auto bodies = services.geometry->bodies();
        check(bodies.size() == 1,
              "imported CAD consumer fixture exposes exactly one Body");
        if (bodies.size() == 1) {
            const auto descriptor = services.geometry->boxDescriptor(bodies.front());
            check(descriptor.has_value(),
                  "imported CAD consumer fixture exposes axis-aligned real Face provenance");
            if (descriptor.has_value()) {
                services.mesh->setDivisions(2, 2, 1);
                check(services.mesh->generate(),
                      "CAD Named Selection consumer regenerates current geometry-driven HEX8 mesh");

                const auto makeFaceItem = [&](const femcae::geometry::GeometryEntityId faceId) {
                    SelectionItem item;
                    const auto *entity = services.geometry->document().find(faceId);
                    item.domain = SelectionDomain::Geometry;
                    item.kind = SelectionKind::Face;
                    item.geometryEntityId = faceId;
                    item.parentGeometryId = entity != nullptr
                        ? entity->parentId : femcae::geometry::InvalidGeometryId;
                    item.sourceRevision = services.geometry->summary().revision;
                    return item;
                };

                const SelectionItem supportFace = makeFaceItem(descriptor->xMinFace);
                const SelectionItem loadFaceA = makeFaceItem(descriptor->xMaxFace);
                const SelectionItem loadFaceB = makeFaceItem(descriptor->yMaxFace);
                check(supportFace.isValid() && loadFaceA.isValid() && loadFaceB.isValid(),
                      "real CAD Face transient identities preserve parent Body and revision");

                const NamedSelectionCreateResult supportNamed = services.namedSelections->createFromSelection(
                    QVector<SelectionItem>{supportFace}, QStringLiteral("OCCT Fixed End"));
                const NamedSelectionCreateResult loadNamed = services.namedSelections->createFromSelection(
                    QVector<SelectionItem>{loadFaceA, loadFaceB}, QStringLiteral("OCCT Loaded Faces"));
                check(supportNamed.success() && loadNamed.success(),
                      "real CAD Face scopes create persistent Named Selection consumers");

                if (supportNamed.success() && loadNamed.success()) {
                    SupportDefinition namedSupport = originalSupport;
                    namedSupport.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    namedSupport.namedSelectionId = supportNamed.id;
                    services.analysis->updateSupport(supportId, namedSupport);

                    LoadDefinition namedLoad = originalLoad;
                    namedLoad.scopingMethod = BoundaryScopingMethod::NamedSelection;
                    namedLoad.namedSelectionId = loadNamed.id;
                    namedLoad.fxN = 1200.0;
                    namedLoad.fyN = 0.0;
                    namedLoad.fzN = 0.0;
                    services.analysis->updateLoad(loadId, namedLoad);

                    const BoundaryScopeResolution supportResolution =
                        services.analysis->resolveBoundaryScope(namedSupport);
                    const BoundaryScopeResolution loadResolution =
                        services.analysis->resolveBoundaryScope(namedLoad);
                    check(supportResolution.valid && supportResolution.geometryFaceIds.size() == 1,
                          "Fixed Support resolves Named Selection ObjectId to one real CAD Face");
                    check(loadResolution.valid && loadResolution.geometryFaceIds.size() == 2,
                          "Force resolves Named Selection ObjectId to two real CAD Faces");

                    const int loadUnionNodes = services.analysis->resolvedBoundaryNodeCount(namedLoad);
                    const int separateNodeCount = services.mesh->nodeCountFor(BoxFace::XMax)
                        + services.mesh->nodeCountFor(BoxFace::YMax);
                    check(loadUnionNodes > 0 && loadUnionNodes < separateNodeCount,
                          "multi-Face Named Selection deduplicates shared FEM edge/corner nodes");

                    check(services.analysis->preflight(analysisId).passed(),
                          "valid CAD Face Named Selection consumers pass preflight");
                    check(services.analysis->solve(analysisId),
                          "Static Structural solver consumes valid CAD Face Named Selection scopes");

                    const AnalysisRecord *namedSolved = services.analysis->analysis(analysisId);
                    if (namedSolved != nullptr && namedSolved->solved) {
                        const double reactionMagnitudeX = std::abs(namedSolved->solveResults.reactionXN);
                        check(std::abs(reactionMagnitudeX - 1200.0) < 1.0e-5,
                              "two-Face Named Selection applies one total 1200 N Force, not 2400 N");
                    } else {
                        check(false, "Named Selection solve produces solved result state");
                    }

                    const NamedSelectionDefinition *loadDefinition =
                        services.namedSelections->byId(loadNamed.id);
                    if (loadDefinition != nullptr) {
                        const ScopeReference originalNamedScope = loadDefinition->scope;
                        const ScopeReferenceBuildResult singleFace = buildGeometryScopeReference(
                            QVector<SelectionItem>{loadFaceA}, services.geometry->document());
                        check(singleFace.success(),
                              "alternate valid CAD Face scope builds for solution staleness test");
                        if (singleFace.success()) {
                            services.namedSelections->replaceScope(loadNamed.id, singleFace.scope);
                            check(services.analysis->solutionIsOutOfDate(analysisId),
                                  "referenced Named Selection scope change makes solution OutOfDate");
                            services.namedSelections->replaceScope(loadNamed.id, originalNamedScope);
                            check(!services.analysis->solutionIsOutOfDate(analysisId),
                                  "restoring identical Named Selection scope restores solver signature validity");
                        }
                    }

                    services.analysis->updateSupport(supportId, originalSupport);
                    services.analysis->updateLoad(loadId, originalLoad);
                    (void)services.namedSelections->remove(supportNamed.id);
                    (void)services.namedSelections->remove(loadNamed.id);
                }
            }
        }
    }

    std::cout << (failures == 0 ? "Boundary consumer acceptance PASS" : "Boundary consumer acceptance FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
