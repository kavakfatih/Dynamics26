#include "DomainCommands.h"

#include "../core/ProjectModel.h"

namespace d26::commands {
namespace {

// Ad değiştirme nesne türüne göre doğru sahibe yönlendirilir; hiçbir yerde
// ağaç metni doğrudan düzenlenmez.
void applyRename(const ServiceContext &services, const ObjectId id, const QString &name)
{
    const ObjectType type = services.project->typeOf(id);
    if (type == ObjectType::Material) {
        services.materials->renameMaterial(id, name);
    } else if (type == ObjectType::Analysis || type == ObjectType::FixedSupport || type == ObjectType::Force
               || isResultDefinition(type)) {
        services.analysis->renameObject(id, name);
    } else {
        services.project->setName(id, name);
    }
}

QString currentName(const ServiceContext &services, const ObjectId id)
{
    const ProjectObject *object = services.project->object(id);
    return object != nullptr ? object->name : QString();
}

} // namespace

namespace {
// Süreç başlangıcından beri geçen milisaniye — komut zaman damgası için.
qint64 nowMs()
{
    static QElapsedTimer clock = [] {
        QElapsedTimer timer;
        timer.start();
        return timer;
    }();
    return clock.elapsed();
}
} // namespace

DomainCommand::DomainCommand(const ServiceContext &services, const QString &text)
    : QUndoCommand(text), services_(services), timestampMs_(nowMs())
{
}

void DomainCommand::stampNow()
{
    timestampMs_ = nowMs();
}

bool DomainCommand::withinMergeWindow(const DomainCommand *other) const
{
    return other != nullptr && (other->timestampMs_ - timestampMs_) <= mergeWindowMs();
}

// --- ad değiştirme -----------------------------------------------------------

RenameObjectCommand::RenameObjectCommand(const ServiceContext &services, const ObjectId id, const QString &newName)
    : DomainCommand(services, QObject::tr("Rename %1").arg(currentName(services, id))),
      object_(id), before_(currentName(services, id)), after_(newName.trimmed())
{
}

void RenameObjectCommand::apply(const QString &name)
{
    applyRename(services_, object_, name);
}

void RenameObjectCommand::redo()
{
    apply(after_);
}

void RenameObjectCommand::undo()
{
    apply(before_);
}

bool RenameObjectCommand::mergeWith(const QUndoCommand *other)
{
    const auto *rename = dynamic_cast<const RenameObjectCommand *>(other);
    if (rename == nullptr || rename->object_ != object_ || !withinMergeWindow(rename)) {
        return false;
    }
    after_ = rename->after_;
    stampNow();
    return true;
}

// --- malzeme -----------------------------------------------------------------

CreateMaterialCommand::CreateMaterialCommand(const ServiceContext &services, MaterialDefinition definition,
                                             const int row, const QString &text)
    : DomainCommand(services, text), definition_(std::move(definition)), row_(row)
{
}

void CreateMaterialCommand::redo()
{
    // İlk çalıştırmada yeni kimlik üretilir; sonraki redo'larda AYNI kimlik
    // yeniden kullanılır, böylece diğer komutlar geçerliliğini korur.
    const ObjectId id = services_.materials->createMaterial(definition_, row_, created_);
    if (created_ == InvalidObjectId) {
        created_ = id;
        if (const MaterialDefinition *stored = services_.materials->byId(id)) {
            definition_ = *stored; // benzersizleştirilmiş ad korunur
        }
        row_ = services_.materials->rowOf(id);
    }
}

void CreateMaterialCommand::undo()
{
    services_.materials->removeMaterial(created_);
}

DeleteMaterialCommand::DeleteMaterialCommand(const ServiceContext &services, const ObjectId id)
    : DomainCommand(services, QObject::tr("Delete %1").arg(currentName(services, id))), object_(id)
{
    if (const MaterialDefinition *definition = services.materials->byId(id)) {
        definition_ = *definition;
    }
    row_ = services.materials->rowOf(id);
    wasAssigned_ = services.materials->assignedMaterialId() == id;
}

void DeleteMaterialCommand::redo()
{
    services_.materials->removeMaterial(object_);
}

void DeleteMaterialCommand::undo()
{
    const ObjectId restored = services_.materials->createMaterial(definition_, row_, object_);
    if (restored != InvalidObjectId && wasAssigned_) {
        services_.materials->setAssignedMaterial(restored);
    }
}

SetMaterialPropertiesCommand::SetMaterialPropertiesCommand(const ServiceContext &services, const ObjectId id,
                                                           MaterialDefinition before, MaterialDefinition after)
    : DomainCommand(services, QObject::tr("Change %1").arg(after.name)), object_(id),
      before_(std::move(before)), after_(std::move(after))
{
}

void SetMaterialPropertiesCommand::redo()
{
    services_.materials->updateMaterial(object_, after_);
}

void SetMaterialPropertiesCommand::undo()
{
    services_.materials->updateMaterial(object_, before_);
}

bool SetMaterialPropertiesCommand::mergeWith(const QUndoCommand *other)
{
    const auto *command = dynamic_cast<const SetMaterialPropertiesCommand *>(other);
    if (command == nullptr || command->object_ != object_ || !withinMergeWindow(command)) {
        return false;
    }
    after_ = command->after_;
    stampNow();
    return true;
}

AssignMaterialCommand::AssignMaterialCommand(const ServiceContext &services, const ObjectId id)
    : DomainCommand(services, QObject::tr("Assign %1").arg(currentName(services, id))), after_(id),
      before_(services.materials->assignedMaterialId())
{
}

void AssignMaterialCommand::redo()
{
    services_.materials->setAssignedMaterial(after_);
}

void AssignMaterialCommand::undo()
{
    services_.materials->setAssignedMaterial(before_);
}

// --- mesh --------------------------------------------------------------------

SetMeshDefinitionCommand::SetMeshDefinitionCommand(const ServiceContext &services, MeshService::Definition before,
                                                   MeshService::Definition after, const QString &text)
    : DomainCommand(services, text), before_(before), after_(after)
{
}

void SetMeshDefinitionCommand::apply(const MeshService::Definition &definition)
{
    // Kaynak önce ayarlanır: türetilmiş ölçüler bundan sonra geçerli olur.
    services_.mesh->setSource(definition.source);
    services_.mesh->setDimensions(definition.lengthMm, definition.widthMm, definition.heightMm);
    services_.mesh->setDivisions(definition.nx, definition.ny, definition.nz);
}

void SetMeshDefinitionCommand::redo()
{
    apply(after_);
}

void SetMeshDefinitionCommand::undo()
{
    apply(before_);
}

bool SetMeshDefinitionCommand::mergeWith(const QUndoCommand *other)
{
    const auto *command = dynamic_cast<const SetMeshDefinitionCommand *>(other);
    if (command == nullptr || command->text() != text() || !withinMergeWindow(command)) {
        return false;
    }
    after_ = command->after_;
    stampNow();
    return true;
}

// --- analiz ------------------------------------------------------------------

CreateAnalysisCommand::CreateAnalysisCommand(const ServiceContext &services, const AnalysisType type)
    : DomainCommand(services, QObject::tr("Add %1").arg(displayName(type))), type_(type)
{
}

void CreateAnalysisCommand::redo()
{
    if (created_ == InvalidObjectId) {
        created_ = services_.analysis->createAnalysis(type_);
        row_ = services_.analysis->rowOfAnalysis(created_);
        snapshot_ = services_.analysis->analysisToJson(created_);
    } else {
        // Yeniden yapıldığında analiz tüm alt nesneleriyle birebir geri kurulur.
        (void)services_.analysis->restoreAnalysis(snapshot_, row_);
    }
}

void CreateAnalysisCommand::undo()
{
    snapshot_ = services_.analysis->analysisToJson(created_);
    row_ = services_.analysis->rowOfAnalysis(created_);
    services_.analysis->removeAnalysis(created_);
}

DeleteAnalysisCommand::DeleteAnalysisCommand(const ServiceContext &services, const ObjectId analysisId)
    : DomainCommand(services, QObject::tr("Delete %1").arg(currentName(services, analysisId))), object_(analysisId)
{
    snapshot_ = services.analysis->analysisToJson(analysisId);
    row_ = services.analysis->rowOfAnalysis(analysisId);
}

void DeleteAnalysisCommand::redo()
{
    snapshot_ = services_.analysis->analysisToJson(object_);
    row_ = services_.analysis->rowOfAnalysis(object_);
    services_.analysis->removeAnalysis(object_);
}

void DeleteAnalysisCommand::undo()
{
    (void)services_.analysis->restoreAnalysis(snapshot_, row_);
}

SetIncompressibilityCommand::SetIncompressibilityCommand(const ServiceContext &services, const ObjectId analysisId,
                                                         const IncompressibilityIntent before,
                                                         const IncompressibilityIntent after)
    : DomainCommand(services, QObject::tr("Change Incompressibility")), object_(analysisId), before_(before),
      after_(after)
{
}

void SetIncompressibilityCommand::redo()
{
    services_.analysis->setIncompressibility(object_, after_);
}

void SetIncompressibilityCommand::undo()
{
    services_.analysis->setIncompressibility(object_, before_);
}

SetLargeDeflectionCommand::SetLargeDeflectionCommand(const ServiceContext &services, const ObjectId analysisId,
                                                     const bool before, const bool after)
    : DomainCommand(services, QObject::tr("Change Large Deflection")), object_(analysisId), before_(before),
      after_(after)
{
}

void SetLargeDeflectionCommand::redo()
{
    services_.analysis->setLargeDeflection(object_, after_);
}

void SetLargeDeflectionCommand::undo()
{
    services_.analysis->setLargeDeflection(object_, before_);
}

// --- sınır şartları / yükler --------------------------------------------------

CreateFixedSupportCommand::CreateFixedSupportCommand(const ServiceContext &services, const ObjectId analysisId,
                                                     SupportDefinition definition, const int row,
                                                     const QString &text)
    : DomainCommand(services, text), analysis_(analysisId), definition_(std::move(definition)), row_(row)
{
}

void CreateFixedSupportCommand::redo()
{
    const ObjectId id = services_.analysis->insertFixedSupport(analysis_, definition_, row_, created_);
    if (created_ == InvalidObjectId) {
        created_ = id;
        if (const SupportDefinition *stored = services_.analysis->support(id)) {
            definition_ = *stored;
        }
        row_ = services_.project->rowOf(id);
    }
}

void CreateFixedSupportCommand::undo()
{
    services_.analysis->removeBoundaryCondition(created_);
}

CreateForceCommand::CreateForceCommand(const ServiceContext &services, const ObjectId analysisId,
                                       LoadDefinition definition, const int row, const QString &text)
    : DomainCommand(services, text), analysis_(analysisId), definition_(std::move(definition)), row_(row)
{
}

void CreateForceCommand::redo()
{
    const ObjectId id = services_.analysis->insertForce(analysis_, definition_, row_, created_);
    if (created_ == InvalidObjectId) {
        created_ = id;
        if (const LoadDefinition *stored = services_.analysis->load(id)) {
            definition_ = *stored;
        }
        row_ = services_.project->rowOf(id);
    }
}

void CreateForceCommand::undo()
{
    services_.analysis->removeBoundaryCondition(created_);
}

DeleteBoundaryConditionCommand::DeleteBoundaryConditionCommand(const ServiceContext &services, const ObjectId id)
    : DomainCommand(services, QObject::tr("Delete %1").arg(currentName(services, id))), object_(id)
{
    analysis_ = services.analysis->owningAnalysis(id);
    row_ = services.project->rowOf(id);
    suppressed_ = services.project->isSuppressed(id);
    if (const LoadDefinition *load = services.analysis->load(id)) {
        isLoad_ = true;
        load_ = *load;
    } else if (const SupportDefinition *support = services.analysis->support(id)) {
        support_ = *support;
    }
}

void DeleteBoundaryConditionCommand::redo()
{
    services_.analysis->removeBoundaryCondition(object_);
}

void DeleteBoundaryConditionCommand::undo()
{
    const ObjectId restored = isLoad_
        ? services_.analysis->insertForce(analysis_, load_, row_, object_)
        : services_.analysis->insertFixedSupport(analysis_, support_, row_, object_);
    if (restored != InvalidObjectId && suppressed_) {
        services_.analysis->setSuppressed(restored, true);
    }
}

SetSupportCommand::SetSupportCommand(const ServiceContext &services, const ObjectId id, SupportDefinition before,
                                     SupportDefinition after)
    : DomainCommand(services, QObject::tr("Change %1").arg(after.name)), object_(id), before_(std::move(before)),
      after_(std::move(after))
{
}

void SetSupportCommand::redo()
{
    services_.analysis->updateSupport(object_, after_);
}

void SetSupportCommand::undo()
{
    services_.analysis->updateSupport(object_, before_);
}

bool SetSupportCommand::mergeWith(const QUndoCommand *other)
{
    const auto *command = dynamic_cast<const SetSupportCommand *>(other);
    if (command == nullptr || command->object_ != object_ || !withinMergeWindow(command)) {
        return false;
    }
    after_ = command->after_;
    stampNow();
    return true;
}

SetForceCommand::SetForceCommand(const ServiceContext &services, const ObjectId id, LoadDefinition before,
                                 LoadDefinition after)
    : DomainCommand(services, QObject::tr("Change %1").arg(after.name)), object_(id), before_(std::move(before)),
      after_(std::move(after))
{
}

void SetForceCommand::redo()
{
    services_.analysis->updateLoad(object_, after_);
}

void SetForceCommand::undo()
{
    services_.analysis->updateLoad(object_, before_);
}

bool SetForceCommand::mergeWith(const QUndoCommand *other)
{
    const auto *command = dynamic_cast<const SetForceCommand *>(other);
    if (command == nullptr || command->object_ != object_ || !withinMergeWindow(command)) {
        return false;
    }
    after_ = command->after_;
    stampNow();
    return true;
}

// --- sonuç tanımları ----------------------------------------------------------

CreateResultDefinitionCommand::CreateResultDefinitionCommand(const ServiceContext &services,
                                                             const ObjectId analysisId,
                                                             const ResultDefinitionKind kind)
    : DomainCommand(services, QObject::tr("Insert %1").arg(displayName(kind))), analysis_(analysisId), kind_(kind)
{
}

void CreateResultDefinitionCommand::redo()
{
    const ObjectId id = services_.analysis->insertResultDefinition(analysis_, kind_, -1, created_);
    if (created_ == InvalidObjectId) {
        created_ = id;
    }
}

void CreateResultDefinitionCommand::undo()
{
    services_.analysis->removeResultDefinition(created_);
}

DeleteResultDefinitionCommand::DeleteResultDefinitionCommand(const ServiceContext &services, const ObjectId id)
    : DomainCommand(services, QObject::tr("Delete %1").arg(currentName(services, id))), object_(id)
{
    analysis_ = services.analysis->owningAnalysis(id);
    row_ = services.project->rowOf(id);
    suppressed_ = services.project->isSuppressed(id);
    if (const ResultDefinition *definition = services.analysis->resultDefinition(id)) {
        definition_ = *definition;
    }
}

void DeleteResultDefinitionCommand::redo()
{
    services_.analysis->removeResultDefinition(object_);
}

void DeleteResultDefinitionCommand::undo()
{
    const ObjectId restored =
        services_.analysis->insertResultDefinition(analysis_, definition_.kind, row_, object_, definition_.name);
    if (restored != InvalidObjectId && suppressed_) {
        services_.analysis->setSuppressed(restored, true);
    }
}

// --- bastırma ----------------------------------------------------------------

SuppressObjectCommand::SuppressObjectCommand(const ServiceContext &services, const ObjectId id,
                                             const bool suppressed)
    : DomainCommand(services, suppressed ? QObject::tr("Suppress %1").arg(currentName(services, id))
                                         : QObject::tr("Unsuppress %1").arg(currentName(services, id))),
      object_(id), after_(suppressed), before_(services.project->isSuppressed(id))
{
}

void SuppressObjectCommand::apply(const bool suppressed)
{
    services_.analysis->setSuppressed(object_, suppressed);
    // Analiz dışı nesneler (ör. Body) için de model durumu güncellenmelidir.
    services_.project->setSuppressed(object_, suppressed);
}

void SuppressObjectCommand::redo()
{
    apply(after_);
}

void SuppressObjectCommand::undo()
{
    apply(before_);
}

} // namespace d26::commands
