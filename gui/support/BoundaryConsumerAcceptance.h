#pragma once

// Dynamics26 Alpha.3.6 — Fixed Support / Force persistent scope consumer
// acceptance. Bu test fiziksel pointer UX testi değildir; gerçek application
// servisleri üzerinde persistence, preflight, dependency state ve legacy solver
// regresyonunu çalıştırır.

#include "../core/DependencyEngine.h"
#include "../core/ProjectModel.h"
#include "../core/SelectionTypes.h"
#include "../services/AnalysisService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QEvent>

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
    check(services.project != nullptr && services.mesh != nullptr && services.analysis != nullptr
              && services.namedSelections != nullptr && services.dependencies != nullptr,
          "boundary consumer acceptance has Project/Mesh/Analysis/NamedSelection/Dependency collaborators");
    if (services.project == nullptr || services.mesh == nullptr || services.analysis == nullptr
        || services.namedSelections == nullptr || services.dependencies == nullptr) {
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
    // Real mesh + application consumer behavior
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

    std::cout << (failures == 0 ? "Boundary consumer acceptance PASS" : "Boundary consumer acceptance FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
