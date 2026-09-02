#pragma once

// Dynamics26 V1.1.0-beta.1 / B1.2 — Contact Preflight acceptance.
//
// Bu kabul testi ikinci bir validation sistemi kurmaz. Gerçek ContactService
// engineering state'i ile gerçek AnalysisService::preflight()/solve() zincirini
// çalıştırır. Amaç ContactRegion tanımlandığında solver'ın desteklemediği contact
// fiziğini sessizce yok saymasını önlemektir.
//
// Beta.1 sözleşmesi:
//   * Project/Connections altındaki aktif ContactRegion'lar model-level state'tir.
//   * draft Contact Source/Target eksikleri Preflight'ta Contact ObjectId ile bloklayıcıdır.
//   * suppressed ContactRegion Solve'u bloklamaz.
//   * stale / mixed-domain / identical / dangling contact bloklayıcıdır.
//   * scope tamamen geçerli olsa bile model-tabanlı Contact solver consumer henüz
//     bağlı değilse Preflight açık bir "Contact Çözücü Desteği" engeli üretir.

#include "../services/AnalysisService.h"
#include "../services/ContactService.h"
#include "../services/MeshService.h"
#include "../shell/Dynamics26MainWindow.h"

#include <QApplication>

#include <iostream>

namespace d26 {
namespace contact_preflight_acceptance_detail {

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

inline ScopeReference syntheticGeometryFaceScope()
{
    ScopeEntityReference reference;
    reference.domain = SelectionDomain::Geometry;
    reference.kind = SelectionKind::Face;
    reference.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(700001);
    reference.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(700000);
    reference.persistentKey = QStringLiteral("contact-preflight-synthetic-face");

    ScopeReference scope;
    scope.sourceRevision = 1;
    scope.entities.push_back(reference);
    return scope;
}

inline bool hasCheck(const PreflightReport &report,
                     const QString &label,
                     const PreflightCheck::Status status,
                     const ObjectId subject,
                     const QString &detailFragment = {})
{
    for (const PreflightCheck &check : report.checks) {
        if (check.label != label || check.status != status || check.subject != subject) {
            continue;
        }
        if (detailFragment.isEmpty() || check.detail.contains(detailFragment, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

} // namespace contact_preflight_acceptance_detail

inline int runContactPreflightAcceptanceTest(QApplication &app,
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
    const ObjectId analysisId = window.currentAnalysis();
    check(services.project != nullptr && services.mesh != nullptr
              && services.contacts != nullptr && services.analysis != nullptr
              && analysisId != InvalidObjectId,
          "Contact Preflight acceptance has project/mesh/contact/analysis composition");
    if (services.project == nullptr || services.mesh == nullptr
        || services.contacts == nullptr || services.analysis == nullptr
        || analysisId == InvalidObjectId) {
        return 1;
    }

    services.contacts->clear();
    services.analysis->clearSolution(analysisId);
    if (!services.mesh->hasMesh() || services.mesh->isOutOfDate()) {
        check(services.mesh->generate(),
              "Contact Preflight fixture generates a current FEM mesh");
    }
    check(services.mesh->mesh().boundaryFacets.size() >= 2,
          "Contact Preflight fixture exposes two real boundary Facet identities");
    if (services.mesh->mesh().boundaryFacets.size() < 2) {
        return failures + 1;
    }

    const PreflightReport baseline = services.analysis->preflight(analysisId);
    check(baseline.passed(),
          "zero active ContactRegion preserves existing ready-to-solve analysis behavior");

    const quint64 generation = services.mesh->generation();
    const auto sourceFacet = services.mesh->mesh().boundaryFacets.at(0).id;
    const auto targetFacet = services.mesh->mesh().boundaryFacets.at(1).id;
    const ScopeReference sourceScope =
        contact_preflight_acceptance_detail::meshFacetScope(generation, sourceFacet);
    const ScopeReference targetScope =
        contact_preflight_acceptance_detail::meshFacetScope(generation, targetFacet);

    // Gerçek authoring sırası: Contact önce kalıcı fakat eksik bir document
    // object olarak oluşur. Source/Target tamamlanana kadar Preflight exact
    // Contact ObjectId'yi bloklayıcı diagnostic subject olarak taşımalıdır.
    ContactDefinition draft;
    draft.name = QStringLiteral("Preflight Contact");
    draft.formulation = ContactFormulation::Bonded;
    const ObjectId contactId = services.contacts->createContact(draft);
    check(contactId != InvalidObjectId
              && services.contacts->validate(contactId).error == ContactValidationError::MissingSourceScope,
          "draft Contact fixture persists explicit missing-Source authoring state");

    PreflightReport report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  contactId, QStringLiteral("Source")),
          "Preflight blocks missing Contact Source with exact Contact ObjectId");

    check(services.contacts->replaceSourceScope(contactId, sourceScope)
              && services.contacts->validate(contactId).error == ContactValidationError::MissingTargetScope,
          "setting Source advances draft Contact to explicit missing-Target state");
    report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  contactId, QStringLiteral("Target")),
          "Preflight blocks missing Contact Target with exact Contact ObjectId");

    check(services.contacts->replaceTargetScope(contactId, targetScope)
              && services.contacts->validate(contactId).valid(),
          "setting Target completes a valid active Mesh/Facet Contact definition");

    report = services.analysis->preflight(analysisId);
    check(!report.passed(),
          "active valid Contact blocks model solve until Contact solver consumer exists");
    check(contact_preflight_acceptance_detail::hasCheck(
              report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Passed,
              contactId, QStringLiteral("geçerli")),
          "Preflight distinguishes valid Contact engineering scope from solver support");
    check(contact_preflight_acceptance_detail::hasCheck(
              report, QStringLiteral("Contact Çözücü Desteği"), PreflightCheck::Status::Failed,
              contactId, QStringLiteral("henüz etkin değil")),
          "unsupported Contact physics is a blocking diagnostic with exact Contact ObjectId");

    check(!services.analysis->solve(analysisId),
          "direct AnalysisService::solve cannot bypass active Contact Preflight guard");
    check(services.analysis->solveState(analysisId) == SolveState::Failed,
          "blocked direct solve finishes in Failed state before solver execution");

    services.contacts->setSuppressed(contactId, true);
    report = services.analysis->preflight(analysisId);
    check(report.passed(),
          "suppressed ContactRegion does not block otherwise valid analysis");
    check(!contact_preflight_acceptance_detail::hasCheck(
              report, QStringLiteral("Contact Çözücü Desteği"), PreflightCheck::Status::Failed,
              contactId),
          "suppressed ContactRegion is absent from active Contact diagnostics");

    services.contacts->setSuppressed(contactId, false);
    const quint64 previousGeneration = services.mesh->generation();
    check(services.mesh->generate() && services.mesh->generation() != previousGeneration,
          "mesh regeneration advances generation for stale Contact regression");
    report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  contactId, QStringLiteral("Mesh")),
          "stale Mesh/Facet Contact blocks Preflight with Contact subject identity");
    check(services.project->object(contactId) != nullptr
              && services.project->object(contactId)->state == ObjectState::OutOfDate,
          "stale ContactRegion remains explicitly OutOfDate in ProjectModel");

    check(services.contacts->replaceTargetScope(
              contactId, contact_preflight_acceptance_detail::syntheticGeometryFaceScope()),
          "Contact fixture can persist structurally valid cross-domain target scope");
    report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  contactId, QStringLiteral("aynı engineering domain")),
          "mixed Mesh/Geometry Contact domain is a blocking Contact diagnostic");

    const ScopeReference persistedSourceScope = services.contacts->byId(contactId)->sourceScope;
    check(services.contacts->replaceTargetScope(contactId, persistedSourceScope),
          "Contact fixture can persist identical source/target scope for validation regression");
    report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  contactId, QStringLiteral("aynı surface")),
          "identical Contact source/target is blocked before stale scope rebinding can occur");

    services.contacts->remove(contactId);
    constexpr ObjectId danglingId = 9007199254742999ULL;
    const ObjectId dangling = services.project->addObjectAt(
        services.project->connectionsNode(), -1, ObjectType::ContactRegion,
        QStringLiteral("Dangling Contact"), 0, danglingId);
    check(dangling == danglingId,
          "dangling ContactRegion fixture restores requested >2^53 tree ObjectId");
    report = services.analysis->preflight(analysisId);
    check(!report.passed()
              && contact_preflight_acceptance_detail::hasCheck(
                  report, QStringLiteral("Contact Kapsamı"), PreflightCheck::Status::Failed,
                  danglingId, QStringLiteral("engineering tanımı bulunamadı")),
          "tree ContactRegion without ContactService definition is blocked with exact ObjectId");
    services.project->removeObject(danglingId);

    services.contacts->clear();
    std::cout << "Contact Preflight safety acceptance "
              << (failures == 0 ? "PASS" : "FAIL")
              << " checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
