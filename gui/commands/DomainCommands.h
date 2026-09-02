#pragma once

// Domain command'ler.
//
// Her komut MÜHENDİSLİK ANLAMI taşır: "Add Force", "Change Mesh Divisions",
// "Suppress Fixed Support". Tek bir generic "set property" komutu kullanılmaz;
// böylece Undo metni kullanıcıya ne geri alacağını söyler ve komutlar
// gelecekte doğru şekilde birleştirilebilir/kaydedilebilir.
//
// Ortak sözleşme:
//   * redo() ilk itişte de çalışır (QUndoStack::push davranışı).
//   * Oluşturma komutları ürettikleri ObjectId'yi saklar ve redo tekrarında
//     AYNI kimliği yeniden kullanır; böylece diğer komutlar ve proje dosyası
//     tutarlı kalır.
//   * Silme komutları nesnenin tam durumunu ve ağaçtaki satırını saklar.

#include "../core/ProjectTypes.h"
#include "../core/ServiceContext.h"
#include "../services/AnalysisService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QString>
#include <QUndoCommand>

namespace d26::commands {

// Ardışık düzenlemelerin tek Undo adımında birleşmesi için kimlikler.
enum CommandId {
    IdRenameObject = 1001,
    IdSetMeshDefinition,
    IdSetForce,
    IdSetSupport,
    IdSetMaterial,
    IdSetNonlinearSolverControls
};

class DomainCommand : public QUndoCommand
{
public:
    DomainCommand(const ServiceContext &services, const QString &text);

    // Ardışık düzenlemelerin tek Undo adımında birleşeceği zaman penceresi.
    // Spinbox sürüklemesi / hızlı yazım tek adım olur; kullanıcı durup ayrı bir
    // düzenleme yaptığında yeni bir Undo adımı başlar (Word/ANSYS davranışı).
    [[nodiscard]] static qint64 mergeWindowMs() noexcept { return 700; }

protected:
    // Bu komut `other` ile birleşebilecek kadar yakın zamanda mı yapıldı?
    [[nodiscard]] bool withinMergeWindow(const DomainCommand *other) const;
    void stampNow();

    ServiceContext services_;
    qint64 timestampMs_{0};
};

// --- ad değiştirme -----------------------------------------------------------

class RenameObjectCommand final : public DomainCommand
{
public:
    RenameObjectCommand(const ServiceContext &services, ObjectId id, const QString &newName);
    void redo() override;
    void undo() override;
    [[nodiscard]] int id() const override { return IdRenameObject; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    void apply(const QString &name);
    ObjectId object_;
    QString before_;
    QString after_;
};

// --- malzeme -----------------------------------------------------------------

class CreateMaterialCommand final : public DomainCommand
{
public:
    CreateMaterialCommand(const ServiceContext &services, MaterialDefinition definition, int row,
                          const QString &text);
    void redo() override;
    void undo() override;
    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    MaterialDefinition definition_;
    int row_;
    ObjectId created_{InvalidObjectId};
};

class DeleteMaterialCommand final : public DomainCommand
{
public:
    DeleteMaterialCommand(const ServiceContext &services, ObjectId id);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    MaterialDefinition definition_;
    int row_{-1};
    bool wasAssigned_{false};
};

class SetMaterialPropertiesCommand final : public DomainCommand
{
public:
    SetMaterialPropertiesCommand(const ServiceContext &services, ObjectId id, MaterialDefinition before,
                                 MaterialDefinition after);
    void redo() override;
    void undo() override;
    [[nodiscard]] int id() const override { return IdSetMaterial; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    ObjectId object_;
    MaterialDefinition before_;
    MaterialDefinition after_;
};

class AssignMaterialCommand final : public DomainCommand
{
public:
    AssignMaterialCommand(const ServiceContext &services, ObjectId id);
    void redo() override;
    void undo() override;

private:
    ObjectId after_;
    ObjectId before_;
};

// --- mesh --------------------------------------------------------------------

class SetMeshDefinitionCommand final : public DomainCommand
{
public:
    SetMeshDefinitionCommand(const ServiceContext &services, MeshService::Definition before,
                             MeshService::Definition after, const QString &text);
    void redo() override;
    void undo() override;
    [[nodiscard]] int id() const override { return IdSetMeshDefinition; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    void apply(const MeshService::Definition &definition);
    MeshService::Definition before_;
    MeshService::Definition after_;
};

// --- analiz ------------------------------------------------------------------

class CreateAnalysisCommand final : public DomainCommand
{
public:
    CreateAnalysisCommand(const ServiceContext &services, AnalysisType type);
    void redo() override;
    void undo() override;
    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    AnalysisType type_;
    ObjectId created_{InvalidObjectId};
    QJsonObject snapshot_;
    int row_{-1};
};

class DeleteAnalysisCommand final : public DomainCommand
{
public:
    DeleteAnalysisCommand(const ServiceContext &services, ObjectId analysisId);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    QJsonObject snapshot_;
    int row_{-1};
};

class SetIncompressibilityCommand final : public DomainCommand
{
public:
    SetIncompressibilityCommand(const ServiceContext &services, ObjectId analysisId,
                                IncompressibilityIntent before, IncompressibilityIntent after);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    IncompressibilityIntent before_;
    IncompressibilityIntent after_;
};

class SetLargeDeflectionCommand final : public DomainCommand
{
public:
    SetLargeDeflectionCommand(const ServiceContext &services, ObjectId analysisId, bool before, bool after);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    bool before_;
    bool after_;
};

// Nonlinear solver ayarlarının tek authoritative snapshot komutu. Komut generic
// bir "set property" değildir: tüm nonlinear control contract'ını taşır ve Undo
// metni hangi engineering ayarının değiştiğini açıkça söyler. Aynı alanın hızlı
// spinbox düzenlemeleri 700 ms penceresinde tek Undo adımında birleşebilir.
class SetNonlinearSolverControlsCommand final : public DomainCommand
{
public:
    SetNonlinearSolverControlsCommand(const ServiceContext &services, ObjectId analysisId,
                                      NonlinearSolverControls before, NonlinearSolverControls after,
                                      const QString &text)
        : DomainCommand(services, text), object_(analysisId), before_(before), after_(after)
    {
    }

    void redo() override
    {
        services_.analysis->setNonlinearSolverControls(object_, after_);
        stampNow();
    }

    void undo() override
    {
        services_.analysis->setNonlinearSolverControls(object_, before_);
    }

    [[nodiscard]] int id() const override { return IdSetNonlinearSolverControls; }

    bool mergeWith(const QUndoCommand *other) override
    {
        const auto *command = dynamic_cast<const SetNonlinearSolverControlsCommand *>(other);
        if (command == nullptr || command->object_ != object_ || command->text() != text()
            || !withinMergeWindow(command)) {
            return false;
        }
        after_ = command->after_;
        timestampMs_ = command->timestampMs_;
        return true;
    }

private:
    ObjectId object_;
    NonlinearSolverControls before_;
    NonlinearSolverControls after_;
};

// --- sınır şartları / yükler --------------------------------------------------

class CreateFixedSupportCommand final : public DomainCommand
{
public:
    CreateFixedSupportCommand(const ServiceContext &services, ObjectId analysisId, SupportDefinition definition,
                              int row, const QString &text);
    void redo() override;
    void undo() override;
    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    ObjectId analysis_;
    SupportDefinition definition_;
    int row_;
    ObjectId created_{InvalidObjectId};
};

class CreateForceCommand final : public DomainCommand
{
public:
    CreateForceCommand(const ServiceContext &services, ObjectId analysisId, LoadDefinition definition, int row,
                       const QString &text);
    void redo() override;
    void undo() override;
    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    ObjectId analysis_;
    LoadDefinition definition_;
    int row_;
    ObjectId created_{InvalidObjectId};
};

class DeleteBoundaryConditionCommand final : public DomainCommand
{
public:
    DeleteBoundaryConditionCommand(const ServiceContext &services, ObjectId id);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    ObjectId analysis_{InvalidObjectId};
    bool isLoad_{false};
    SupportDefinition support_;
    LoadDefinition load_;
    int row_{-1};
    bool suppressed_{false};
};

class SetSupportCommand final : public DomainCommand
{
public:
    SetSupportCommand(const ServiceContext &services, ObjectId id, SupportDefinition before,
                      SupportDefinition after);
    void redo() override;
    void undo() override;
    [[nodiscard]] int id() const override { return IdSetSupport; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    ObjectId object_;
    SupportDefinition before_;
    SupportDefinition after_;
};

class SetForceCommand final : public DomainCommand
{
public:
    SetForceCommand(const ServiceContext &services, ObjectId id, LoadDefinition before, LoadDefinition after);
    void redo() override;
    void undo() override;
    [[nodiscard]] int id() const override { return IdSetForce; }
    bool mergeWith(const QUndoCommand *other) override;

private:
    ObjectId object_;
    LoadDefinition before_;
    LoadDefinition after_;
};

// --- sonuç tanımları ----------------------------------------------------------

class CreateResultDefinitionCommand final : public DomainCommand
{
public:
    CreateResultDefinitionCommand(const ServiceContext &services, ObjectId analysisId, ResultDefinitionKind kind);
    void redo() override;
    void undo() override;
    [[nodiscard]] ObjectId createdId() const noexcept { return created_; }

private:
    ObjectId analysis_;
    ResultDefinitionKind kind_;
    ObjectId created_{InvalidObjectId};
};

class DeleteResultDefinitionCommand final : public DomainCommand
{
public:
    DeleteResultDefinitionCommand(const ServiceContext &services, ObjectId id);
    void redo() override;
    void undo() override;

private:
    ObjectId object_;
    ObjectId analysis_{InvalidObjectId};
    ResultDefinition definition_;
    int row_{-1};
    bool suppressed_{false};
};

// --- bastırma ----------------------------------------------------------------

class SuppressObjectCommand final : public DomainCommand
{
public:
    SuppressObjectCommand(const ServiceContext &services, ObjectId id, bool suppressed);
    void redo() override;
    void undo() override;

private:
    void apply(bool suppressed);
    ObjectId object_;
    bool after_;
    bool before_;
};

} // namespace d26::commands
