#pragma once

// Dynamics26 — proje nesne modeli temel tipleri.
//
// Model ağacındaki her düğüm gerçek bir nesnedir: görünen metin değil,
// kararlı bir ObjectId + ObjectType taşır. Details paneli, komut yüzeyi ve
// viewport bağlamı bu tipe göre çözülür; hiçbir yerde etiket metnine bakarak
// nesne tanıma yapılmaz.

#include <QMetaType>
#include <QString>

#include <cstdint>

namespace d26 {

using ObjectId = quint64;
inline constexpr ObjectId InvalidObjectId = 0;

enum class ObjectType {
    Project,            // Proje kökü
    Model,              // Model (geometri + malzeme + mesh)
    GeometryFolder,     // Geometry klasörü
    Body,               // CAD katı gövde
    MaterialsFolder,
    Material,
    SectionsFolder,
    Section,
    ConnectionsFolder,
    ContactRegion,
    Mesh,
    Analysis,           // Static Structural 1 / Modal 1 / Nonlinear Static 1
    AnalysisSettings,
    FixedSupport,
    Force,
    Solution,
    TotalDeformation,
    EquivalentStress,
    ReactionForce,
    ModeShape,

    // Alpha.3.6 — enum değerleri bilinçli olarak listenin SONUNA eklenir.
    // Project persistence ObjectType'ı integer olarak sakladığı için araya yeni
    // değer eklemek eski dosyalardaki Material/Analysis/Result kimliğini kaydırır.
    NamedSelectionsFolder,
    NamedSelection
};

// Nesne durumu ANSYS Mechanical'daki ikon rozetine karşılık gelir.
// Kullanıcıya gösterilen mesajın önem derecesi (Messages sekmesi + status bar).
enum class Severity { Info, Success, Warning, Error };

enum class ObjectState {
    None,        // durum rozeti yok (klasör vb.)
    NotReady,    // tanım eksik
    Ready,       // tanımlı, çözülmeyi bekliyor
    UpToDate,    // güncel
    OutOfDate,   // girdi değişti, yeniden üretilmeli
    Warning,     // kullanılabilir ama dikkat gerektiriyor
    Error,
    Suppressed,  // modelde duruyor fakat çözüme katılmıyor
    Solving
};

// Analiz türü. Backend bağlanabilirliği AnalysisService tarafından belirlenir.
enum class AnalysisType {
    StaticStructural,
    Modal,
    NonlinearStatic
};

// Kullanıcı seviyesindeki sıkışmazlık tercihi (§11: user intent).
// Solver implementasyonu (mixed u-p / P0 / penalty) bundan türetilir.
enum class IncompressibilityIntent {
    Automatic,
    Compressible,
    NearlyIncompressible
};

// Solver'ın gerçekte çözdüğü formülasyon. Advanced bölümünde gösterilir.
enum class ResolvedFormulation {
    DisplacementBased,
    MixedUP
};

enum class MaterialModel {
    LinearElastic = 0,
    NeoHookean = 1,
    MooneyRivlin = 2,
    Yeoh = 3,
    Ogden = 4
};

// Mesh kaynağı: içe aktarılan CAD gövdesinin sınır kutusu mu, yoksa
// kullanıcı tanımlı parametrik kutu mu? CAD B-Rep hiçbir zaman FEM mesh
// olarak kullanılmaz; yalnız sınır kutusu ölçüsü devralınır.
enum class MeshSource {
    ParametricBox,
    GeometryBoundingBox
};

// Bir sonuç TANIMI (result definition) model durumudur ve undoable'dır;
// hesaplanan alan değerleri türetilmiş veridir ve Undo yığınına girmez.
enum class ResultDefinitionKind {
    TotalDeformation,
    EquivalentStress,
    ReactionForce
};

QString displayName(ResultDefinitionKind kind);
ObjectType objectTypeFor(ResultDefinitionKind kind);
[[nodiscard]] bool isResultDefinition(ObjectType type);
// Suppress/Unsuppress hangi nesne türlerinde anlamlıdır?
[[nodiscard]] bool supportsSuppression(ObjectType type);
// Rename / Duplicate / Delete hangi türlerde anlamlıdır?
[[nodiscard]] bool supportsRename(ObjectType type);
[[nodiscard]] bool supportsDuplicate(ObjectType type);
[[nodiscard]] bool supportsDelete(ObjectType type);

QString displayName(ObjectType type);
QString displayName(AnalysisType type);
QString displayName(MaterialModel model);
QString displayName(ObjectState state);

// Bir nesnenin viewport bağlamını belirler (§16).
enum class ViewportContext {
    Empty,
    Geometry,
    Materials,
    Connections,
    Mesh,
    Loads,
    Analysis,
    Results,
    Modal
};

ViewportContext viewportContextFor(ObjectType type);

} // namespace d26

Q_DECLARE_METATYPE(d26::ObjectType)
Q_DECLARE_METATYPE(d26::Severity)
