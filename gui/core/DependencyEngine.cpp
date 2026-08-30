#include "DependencyEngine.h"

#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "ProjectModel.h"

namespace d26 {

DependencyEngine::DependencyEngine(const ServiceContext &services, QObject *parent)
    : QObject(parent), services_(services)
{
}

void DependencyEngine::setSolvingAnalysis(const ObjectId analysisId)
{
    solvingAnalysis_ = analysisId;
    evaluate();
}

bool DependencyEngine::meshIsOutOfDate() const
{
    return services_.mesh->isOutOfDate();
}

bool DependencyEngine::anySolutionOutOfDate() const
{
    for (const ObjectId analysisId : services_.project->analyses()) {
        if (services_.analysis->solutionIsOutOfDate(analysisId)) {
            return true;
        }
    }
    return false;
}

void DependencyEngine::evaluate()
{
    evaluateModel();
    for (const ObjectId analysisId : services_.project->analyses()) {
        evaluateAnalysis(analysisId);
    }
}

void DependencyEngine::evaluateModel()
{
    ProjectModel *project = services_.project;
    const GeometrySummary geometry = services_.geometry->summary();

    // --- Geometry ---
    const ObjectId geometryNode = project->geometryNode();
    if (geometry.hasGeometry) {
        project->setState(geometryNode, ObjectState::UpToDate,
                          QObject::tr("%1 — %2 body").arg(geometry.sourceFileName).arg(geometry.bodyCount));
    } else {
        project->setState(geometryNode, ObjectState::UpToDate, QObject::tr("Parametrik kutu gövdesi"));
    }
    for (const ObjectId body : project->childrenOfType(geometryNode, ObjectType::Body)) {
        if (project->isSuppressed(body)) {
            project->setState(body, ObjectState::Suppressed, QObject::tr("Bastırıldı — çözüme katılmıyor"));
        } else {
            project->setState(body, ObjectState::UpToDate,
                              geometry.hasGeometry ? QObject::tr("CAD gövdesi") : QObject::tr("Parametrik kutu"));
        }
    }

    // --- Materials ---
    const ObjectId materialsNode = project->materialsNode();
    project->setState(materialsNode, ObjectState::None);
    for (const ObjectId id : project->childrenOfType(materialsNode, ObjectType::Material)) {
        const MaterialDefinition *definition = services_.materials->byId(id);
        if (definition == nullptr) {
            continue;
        }
        const bool assigned = id == services_.materials->assignedMaterialId();
        project->setState(id, assigned ? ObjectState::UpToDate : ObjectState::Ready,
                          assigned ? QObject::tr("%1 — gövdeye atandı").arg(displayName(definition->model))
                                   : displayName(definition->model));
    }

    // --- Mesh ---
    const ObjectId meshNode = project->meshNode();
    if (!services_.mesh->hasMesh()) {
        project->setState(meshNode, ObjectState::NotReady, QObject::tr("Mesh üretilmedi"));
    } else if (services_.mesh->isOutOfDate()) {
        // Geometri veya mesh ayarı üretimden sonra değişti.
        project->setState(meshNode, ObjectState::OutOfDate,
                          QObject::tr("Girdiler değişti — Generate Mesh çalıştırın"));
    } else if (services_.mesh->quality().invertedElementCount > 0) {
        project->setState(meshNode, ObjectState::Error,
                          QObject::tr("%1 ters eleman").arg(services_.mesh->quality().invertedElementCount));
    } else {
        project->setState(meshNode, ObjectState::UpToDate,
                          QObject::tr("%1 node · %2 HEX8")
                              .arg(services_.mesh->nodeCount())
                              .arg(services_.mesh->elementCount()));
    }
}

void DependencyEngine::evaluateAnalysis(const ObjectId analysisId)
{
    ProjectModel *project = services_.project;
    AnalysisService *analysisService = services_.analysis;
    const AnalysisRecord *record = analysisService->analysis(analysisId);
    if (record == nullptr) {
        return;
    }

    // --- sınır şartları ve yükler ---
    for (const ObjectId id : record->supports) {
        const SupportDefinition *definition = analysisService->support(id);
        if (definition == nullptr) {
            continue;
        }
        if (project->isSuppressed(id)) {
            project->setState(id, ObjectState::Suppressed, QObject::tr("Bastırıldı — çözüme katılmıyor"));
        } else {
            project->setState(id, ObjectState::Ready, QObject::tr("Kapsam: %1").arg(displayName(definition->scope)));
        }
    }
    for (const ObjectId id : record->loads) {
        const LoadDefinition *definition = analysisService->load(id);
        if (definition == nullptr) {
            continue;
        }
        if (project->isSuppressed(id)) {
            project->setState(id, ObjectState::Suppressed, QObject::tr("Bastırıldı — çözüme katılmıyor"));
        } else {
            project->setState(id, ObjectState::Ready,
                              QObject::tr("Kapsam: %1  •  |F| = %2 N")
                                  .arg(displayName(definition->scope))
                                  .arg(definition->magnitudeN(), 0, 'g', 6));
        }
    }

    project->setState(record->settingsNode, ObjectState::None);

    const bool suppressed = project->isSuppressed(analysisId);
    const bool solving = analysisId == solvingAnalysis_;
    const bool stale = analysisService->solutionIsOutOfDate(analysisId);
    const PreflightReport report = analysisService->preflight(analysisId);

    // --- sonuç tanımları ---
    for (const ObjectId id : record->results) {
        const ResultDefinition *definition = analysisService->resultDefinition(id);
        if (definition == nullptr) {
            continue;
        }
        if (project->isSuppressed(id)) {
            project->setState(id, ObjectState::Suppressed, QObject::tr("Bastırıldı"));
            continue;
        }
        if (!record->solved) {
            project->setState(id, ObjectState::NotReady, QObject::tr("Çözüm çalıştırılmadı"));
            continue;
        }
        if (stale) {
            project->setState(id, ObjectState::OutOfDate, QObject::tr("Girdiler değişti — yeniden çözün"));
            continue;
        }
        QString detail;
        switch (definition->kind) {
        case ResultDefinitionKind::TotalDeformation:
            detail = QObject::tr("Max %1 mm").arg(record->solveResults.maxDisplacementMm, 0, 'g', 6);
            break;
        case ResultDefinitionKind::EquivalentStress:
            detail = QObject::tr("Max %1 MPa").arg(record->solveResults.maxVonMisesMPa, 0, 'g', 6);
            break;
        case ResultDefinitionKind::ReactionForce:
            detail = QObject::tr("ΣRx %1 N").arg(record->solveResults.reactionXN, 0, 'g', 6);
            break;
        }
        project->setState(id, ObjectState::UpToDate, detail);
    }

    // --- Solution ---
    const ObjectId solutionNode = record->solutionNode;
    if (suppressed) {
        project->setState(solutionNode, ObjectState::Suppressed, QObject::tr("Analiz bastırıldı"));
    } else if (solving) {
        project->setState(solutionNode, ObjectState::Solving, QObject::tr("Çözülüyor"));
    } else if (!record->solved) {
        project->setState(solutionNode, ObjectState::NotReady, QObject::tr("Çözüm çalıştırılmadı"));
    } else if (stale) {
        project->setState(solutionNode, ObjectState::OutOfDate, QObject::tr("Girdiler değişti — yeniden çözün"));
    } else {
        project->setState(solutionNode, ObjectState::UpToDate, QObject::tr("Çözüm güncel"));
    }

    // --- Analysis ---
    if (suppressed) {
        project->setState(analysisId, ObjectState::Suppressed, QObject::tr("Bastırıldı"));
    } else if (solving) {
        project->setState(analysisId, ObjectState::Solving, QObject::tr("Çözülüyor"));
    } else if (record->solved && stale) {
        project->setState(analysisId, ObjectState::OutOfDate, QObject::tr("Sonuçlar güncel değil"));
    } else if (record->solved) {
        project->setState(analysisId, ObjectState::UpToDate, QObject::tr("Çözüldü"));
    } else if (report.passed()) {
        project->setState(analysisId, report.hasWarnings() ? ObjectState::Warning : ObjectState::Ready,
                          QObject::tr("Çözüme hazır"));
    } else {
        project->setState(analysisId, ObjectState::NotReady, report.firstFailure());
    }
}

} // namespace d26
