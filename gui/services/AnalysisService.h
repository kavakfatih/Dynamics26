#pragma once

// Analiz servisi.
//
// Analiz nesnelerinin, sınır şartlarının, yüklerin, SONUÇ TANIMLARININ ve
// hesaplanmış sonuçların sahibidir. Analiz alt ağacını da bu servis kurar;
// böylece "analiz nesnesi" kavramının tek bir sahibi olur.
//
// Model durumu / türetilmiş veri ayrımı:
//   MODEL STATE     → analiz ayarları, BC/yük tanımları, sonuç TANIMLARI
//                     (undoable, projede saklanır)
//   DERIVED STATE   → üretilmiş mesh sonuçları, ResultDatabase alan değerleri,
//                     solve-session telemetry (Undo/persistence'a girmez)
//
// §11 gereği kullanıcı niyeti (Incompressibility: Automatic) ile solver
// implementasyonu (mixed u-p / HEX8-P0) ayrılır.

#include "../core/ProjectModel.h"
#include "../core/ProjectTypes.h"
#include "../core/AnalysisCapability.h"
#include "../core/ScopeReferenceBuilder.h"
#include "../core/SolverTelemetry.h"
#include "MaterialService.h"
#include "MeshService.h"

#include <QHash>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

#include <femcae/meshing/Assignments.h>
#include <femcae/meshing/ResultDatabase.h>

#include <vector>

namespace d26 {

class ContactService;
class NamedSelectionService;

// Boundary-condition consumer scope tipi. Geometry Selection legacy/current
// doğrudan yüz kapsamını korur. Named Selection seçildiğinde consumer entity
// kimliklerini KOPYALAMAZ; yalnız persistent Named Selection ObjectId'sini tutar.
// Böylece scope değişikliği tek kaynakta yaşar ve bağımlılık motoru tarafından
// tüketicilere yayılır.
enum class BoundaryScopingMethod {
    GeometrySelection = 0,
    NamedSelection = 1
};

struct SupportDefinition {
    QString name;
    BoundaryScopingMethod scopingMethod{BoundaryScopingMethod::GeometrySelection};
    BoxFace scope{BoxFace::XMin};
    ObjectId namedSelectionId{InvalidObjectId};
    bool fixX{true};
    bool fixY{true};
    bool fixZ{true};
    [[nodiscard]] QJsonObject toJson() const;
    static SupportDefinition fromJson(const QJsonObject &object);
};

struct LoadDefinition {
    QString name;
    BoundaryScopingMethod scopingMethod{BoundaryScopingMethod::GeometrySelection};
    BoxFace scope{BoxFace::XMax};
    ObjectId namedSelectionId{InvalidObjectId};
    double fxN{1000.0};
    double fyN{0.0};
    double fzN{0.0};
    [[nodiscard]] double magnitudeN() const;
    [[nodiscard]] QJsonObject toJson() const;
    static LoadDefinition fromJson(const QJsonObject &object);
};

// Consumer-resolution sonucu. Solver/UI raw Named Selection scope verisini
// yorumlamaz; tek resolver NamedSelection ObjectId -> current CAD Face kimlikleri
// zincirini doğrular. Alpha.3.6 BC consumer yalnız Geometry/Face Named Selection
// kabul eder. Stale/dangling/yanlış-domain kapsamlar açıkça invalid döner.
struct BoundaryScopeResolution {
    bool valid{false};
    BoundaryScopingMethod method{BoundaryScopingMethod::GeometrySelection};
    ObjectId namedSelectionId{InvalidObjectId};
    QVector<femcae::geometry::GeometryEntityId> geometryFaceIds;
    QString label;
    QString error;
    ScopeReferenceValidationError validationError{ScopeReferenceValidationError::None};
};

struct ResultDefinition {
    QString name;
    ResultDefinitionKind kind{ResultDefinitionKind::TotalDeformation};
    [[nodiscard]] QJsonObject toJson() const;
    static ResultDefinition fromJson(const QJsonObject &object);
};

// Beta.2 B2.3 persistent nonlinear solver authoring contract.
//
// Bu yapı solver'ın ikinci bir çalışma durumu değildir. Analysis Settings altında
// kullanıcının kalıcı mühendislik niyetidir; Undo/Redo ve proje persistence'a
// girer. General model nonlinear consumer henüz bağlı olmadığından bu alanlar
// mevcut lineer solver input signature'ına özellikle dahil edilmez.
//
// Integer kimlikler fem_nonlinear_solver.f90 ile bilinçli olarak aynıdır:
// NONLINEAR_FULL_NEWTON=1, NONLINEAR_MODIFIED_NEWTON=2. Böylece gelecekteki
// consumer bridge'de sessiz 0/1 -> 1/2 semantik kayması oluşmaz.
enum class NonlinearMethodIntent {
    FullNewton = 1,
    ModifiedNewton = 2
};

struct NonlinearSolverControls {
    NonlinearMethodIntent method{NonlinearMethodIntent::FullNewton};
    int maximumIterations{25};
    bool adaptiveStepping{true};
    double initialLoadIncrement{0.25};
    double minimumLoadIncrement{1.0e-4};
    double maximumLoadIncrement{0.50};
    bool lineSearch{true};
    double residualRelativeTolerance{1.0e-8};
    double displacementRelativeTolerance{1.0e-8};

    [[nodiscard]] QJsonObject toJson() const;
    static NonlinearSolverControls fromJson(const QJsonObject &object);
    [[nodiscard]] bool isValid(QString *error = nullptr) const;

    friend bool operator==(const NonlinearSolverControls &, const NonlinearSolverControls &) = default;
};

struct SolveResults {
    bool valid{false};
    double maxDisplacementMm{0.0};
    double maxVonMisesMPa{0.0};
    double minVonMisesMPa{0.0};
    double reactionXN{0.0};
    double reactionYN{0.0};
    double reactionZN{0.0};
    qint64 probeNodeId{-1};
    double probeUxMm{0.0};
    int dofCount{0};
    int nodeCount{0};
    int elementCount{0};
    double wallClockSeconds{0.0};
};

// Preflight tek bir kontrol satırı.
struct PreflightCheck {
    enum class Status { Passed, Failed, Warning };
    Status status{PreflightCheck::Status::Passed};
    QString label;
    QString detail;
    // Hatanın hangi nesneden kaynaklandığı (tree'de gösterilebilir).
    ObjectId subject{InvalidObjectId};
};

struct PreflightReport {
    QVector<PreflightCheck> checks;
    [[nodiscard]] bool passed() const;
    [[nodiscard]] bool hasWarnings() const;
    [[nodiscard]] QString firstFailure() const;
    [[nodiscard]] QStringList failureMessages() const;
};

// Çözüm yaşam döngüsü (§26). Şu an eşzamanlı çalışır; state machine
// gelecekteki asenkron/iptal edilebilir çözüme hazırdır.
enum class SolveState { Idle, Preflight, Ready, Solving, Completed, Failed, Cancelled };

struct AnalysisRecord {
    AnalysisType type{AnalysisType::StaticStructural};
    IncompressibilityIntent incompressibility{IncompressibilityIntent::Automatic};
    bool largeDeflection{false};
    NonlinearSolverControls nonlinearControls;
    ObjectId settingsNode{InvalidObjectId};
    ObjectId solutionNode{InvalidObjectId};
    QVector<ObjectId> supports;
    QVector<ObjectId> loads;
    QVector<ObjectId> results;

    // --- türetilmiş durum (projede saklanmaz) ---
    bool solved{false};
    SolveResults solveResults;
    femcae::meshing::ResultDatabase resultDatabase;
    SolveState solveState{SolveState::Idle};
    SolverConvergenceSnapshot solverTelemetry;
    // Çözümün üretildiği GİRDİ İMZASI. Monoton sayaç yerine içerik imzası
    // kullanılır: bir değişikliği Undo ile geri almak sonuçları yeniden
    // geçerli kılar, çünkü solver girdisi tekrar aynı hale gelir.
    QByteArray solvedSignature;
};

class AnalysisService final : public QObject
{
    Q_OBJECT
public:
    AnalysisService(ProjectModel *project, MeshService *mesh, MaterialService *materials,
                    QObject *parent = nullptr);

    // Persistent engineering servisleri AnalysisService'ten bağımsız kurulur ve
    // composition root tarafından bir kez bağlanır. Setter'lar yalnız pointer
    // wiring yapar; servis sahipliği QObject parent tree'sinde kalır.
    void setNamedSelectionService(NamedSelectionService *service) noexcept { namedSelections_ = service; }
    void setContactService(ContactService *service) noexcept { contacts_ = service; }

    // --- analiz nesneleri ---
    ObjectId createAnalysis(AnalysisType type, int row = -1, ObjectId requestedId = InvalidObjectId,
                            const QString &name = QString(), bool withDefaults = true,
                            ObjectId requestedSettingsId = InvalidObjectId,
                            ObjectId requestedSolutionId = InvalidObjectId);
    bool removeAnalysis(ObjectId analysisId);
    void clearAll();

    [[nodiscard]] const AnalysisRecord *analysis(ObjectId analysisId) const;
    [[nodiscard]] const SupportDefinition *support(ObjectId id) const;
    [[nodiscard]] const LoadDefinition *load(ObjectId id) const;
    [[nodiscard]] const ResultDefinition *resultDefinition(ObjectId id) const;
    [[nodiscard]] ObjectId owningAnalysis(ObjectId id) const;

    void setIncompressibility(ObjectId analysisId, IncompressibilityIntent intent);
    void setLargeDeflection(ObjectId analysisId, bool enabled);
    void setNonlinearSolverControls(ObjectId analysisId, const NonlinearSolverControls &controls);
    void renameObject(ObjectId id, const QString &name);

    // Boundary consumer resolver. Geometry Selection veya persistent Named
    // Selection referansı current model/mesh provenance'ına karşı burada
    // doğrulanır; UI, DependencyEngine ve solver aynı sonucu tüketir.
    [[nodiscard]] BoundaryScopeResolution resolveBoundaryScope(const SupportDefinition &definition) const;
    [[nodiscard]] BoundaryScopeResolution resolveBoundaryScope(const LoadDefinition &definition) const;
    [[nodiscard]] int resolvedBoundaryNodeCount(const SupportDefinition &definition) const;
    [[nodiscard]] int resolvedBoundaryNodeCount(const LoadDefinition &definition) const;

    // --- sınır şartları / yükler / sonuç tanımları ---
    ObjectId insertFixedSupport(ObjectId analysisId, const SupportDefinition &definition = {}, int row = -1,
                                ObjectId requestedId = InvalidObjectId);
    ObjectId insertForce(ObjectId analysisId, const LoadDefinition &definition = {}, int row = -1,
                         ObjectId requestedId = InvalidObjectId);
    ObjectId insertResultDefinition(ObjectId analysisId, ResultDefinitionKind kind, int row = -1,
                                    ObjectId requestedId = InvalidObjectId, const QString &name = QString());
    bool removeBoundaryCondition(ObjectId id);
    bool removeResultDefinition(ObjectId id);
    void updateSupport(ObjectId id, const SupportDefinition &definition);
    void updateLoad(ObjectId id, const LoadDefinition &definition);

    // --- suppression ---
    void setSuppressed(ObjectId id, bool suppressed);

    // --- solver niyeti / implementasyonu ---
    [[nodiscard]] ResolvedFormulation resolvedFormulation(ObjectId analysisId) const;
    [[nodiscard]] QString resolvedElementTechnology(ObjectId analysisId) const;
    [[nodiscard]] QString resolvedLinearSolver() const;

    // --- yaşam döngüsü ---
    [[nodiscard]] AnalysisCapabilityResolution resolveCapabilities(ObjectId analysisId) const;
    [[nodiscard]] PreflightReport preflight(ObjectId analysisId) const;
    bool solve(ObjectId analysisId);
    void clearSolution(ObjectId analysisId);
    [[nodiscard]] SolveState solveState(ObjectId analysisId) const;
    [[nodiscard]] const SolverConvergenceSnapshot *solverTelemetry(ObjectId analysisId) const
    {
        const AnalysisRecord *record = analysis(analysisId);
        return record != nullptr ? &record->solverTelemetry : nullptr;
    }

    [[nodiscard]] bool hasResults(ObjectId analysisId) const;
    // Çözüm var ama girdiler değiştiyse sonuçlar bayattır.
    [[nodiscard]] bool solutionIsOutOfDate(ObjectId analysisId) const;
    [[nodiscard]] const femcae::meshing::ResultDatabase *resultDatabase(ObjectId analysisId) const;

    [[nodiscard]] static int warningDofThreshold() noexcept { return 2400; }
    [[nodiscard]] static int maximumDofThreshold() noexcept { return 6000; }

    // --- kalıcılık ---
    // Tek bir analizin tam anlık görüntüsü. Undo (Delete Analysis) ve proje
    // kaydı aynı temsili kullanır. solverTelemetry özellikle bu temsile girmez.
    [[nodiscard]] QJsonObject analysisToJson(ObjectId analysisId) const;
    ObjectId restoreAnalysis(const QJsonObject &entry, int row = -1);
    [[nodiscard]] int rowOfAnalysis(ObjectId analysisId) const;
    [[nodiscard]] QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);
    [[nodiscard]] QJsonObject toLegacyLoadJson() const;
    void applyLegacyLoadJson(const QJsonObject &object);

signals:
    void changed();
    void message(const QString &text, d26::Severity severity);
    void solverOutput(const QString &text);
    void solveStateChanged(ObjectId analysisId, d26::SolveState state);
    void solverTelemetryChanged(ObjectId analysisId);
    void resultsChanged(ObjectId analysisId);

private:
    [[nodiscard]] PreflightReport preflight(
        ObjectId analysisId, const AnalysisCapabilityResolution &capabilities) const;
    // Solver'ın GERÇEKTEN tükettiği girdilerin imzası: mesh üretimi ve ölçüleri,
    // atanmış malzemenin elastik parametreleri, AKTİF sınır şartı/yük kapsam ve
    // değerleri, çözülen formülasyon. Ad değişikliği ve henüz consumer'a bağlı
    // olmayan nonlinear authoring controls gibi solver'ı etkilemeyen düzenlemeler
    // imzayı değiştirmez, dolayısıyla mevcut lineer sonuçları sahte bayatlatmaz.
    [[nodiscard]] QByteArray solverInputSignature(ObjectId analysisId) const;
    [[nodiscard]] BoundaryScopeResolution resolveBoundaryScope(BoundaryScopingMethod method,
                                                                BoxFace geometryScope,
                                                                ObjectId namedSelectionId) const;
    [[nodiscard]] std::vector<femcae::meshing::MeshEntityId>
    resolvedBoundaryNodeIds(const BoundaryScopeResolution &scope) const;
    [[nodiscard]] QJsonObject boundaryScopeSignature(BoundaryScopingMethod method,
                                                     BoxFace geometryScope,
                                                     ObjectId namedSelectionId) const;
    void touchDefinition(ObjectId analysisId);
    void refreshBoundaryNode(ObjectId id);
    // supports/loads/results vektörlerini ağaç sırasından yeniden kurar.
    // Ağaç satırı ile liste indeksi iki farklı uzaydır; tek doğruluk kaynağı
    // ağaç sırası olsun diye her ekleme/silme sonrası yeniden senkronlanır.
    void resyncChildLists(ObjectId analysisId);
    void refreshResultNodes(ObjectId analysisId);
    [[nodiscard]] QString uniqueChildName(ObjectId analysisId, ObjectType type, const QString &base) const;
    [[nodiscard]] bool isActive(ObjectId id) const;

    ProjectModel *project_;
    MeshService *mesh_;
    MaterialService *materials_;
    NamedSelectionService *namedSelections_{nullptr};
    ContactService *contacts_{nullptr};
    QHash<ObjectId, AnalysisRecord> analyses_;
    QHash<ObjectId, SupportDefinition> supports_;
    QHash<ObjectId, LoadDefinition> loads_;
    QHash<ObjectId, ResultDefinition> results_;
    int analysisCounter_{0};
};

} // namespace d26
