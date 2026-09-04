#include "AnalysisCapability.h"

#include <QCoreApplication>

namespace d26 {
namespace {

using State = CapabilityState;
using Axis = CapabilityAxis;

void add(AnalysisCapabilityResolution &resolution, const Axis axis, const State state,
         const QString &label, const QString &detail, const ObjectId subject)
{
    resolution.decisions.push_back(CapabilityDecision{axis, state, label, detail, subject});
}

QString resultCapabilityName(const ResultDefinitionKind kind)
{
    return displayName(kind);
}

} // namespace

bool AnalysisCapabilityResolution::solveReady() const noexcept
{
    for (const CapabilityDecision &item : decisions) {
        if (!item.ready()) {
            return false;
        }
    }
    return !decisions.isEmpty();
}

const CapabilityDecision *AnalysisCapabilityResolution::firstBlocking() const noexcept
{
    for (const CapabilityDecision &item : decisions) {
        if (!item.ready()) {
            return &item;
        }
    }
    return nullptr;
}

const CapabilityDecision *AnalysisCapabilityResolution::decision(const CapabilityAxis axis) const noexcept
{
    for (const CapabilityDecision &item : decisions) {
        if (item.axis == axis) {
            return &item;
        }
    }
    return nullptr;
}

QString capabilityStateName(const CapabilityState state)
{
    switch (state) {
    case CapabilityState::Ready:
        return QCoreApplication::translate("d26", "Ready");
    case CapabilityState::SetupOnly:
        return QCoreApplication::translate("d26", "Setup Only");
    case CapabilityState::Unavailable:
        return QCoreApplication::translate("d26", "Unavailable");
    case CapabilityState::Stale:
        return QCoreApplication::translate("d26", "Stale");
    case CapabilityState::Invalid:
        return QCoreApplication::translate("d26", "Invalid");
    }
    return QCoreApplication::translate("d26", "Invalid");
}

QString matrixCapabilityName(const MatrixCapability &matrix)
{
    QString symmetry;
    switch (matrix.symmetry) {
    case MatrixSymmetry::Symmetric:
        symmetry = QCoreApplication::translate("d26", "symmetric");
        break;
    case MatrixSymmetry::Unsymmetric:
        symmetry = QCoreApplication::translate("d26", "unsymmetric");
        break;
    case MatrixSymmetry::Unknown:
        symmetry = QCoreApplication::translate("d26", "symmetry unknown");
        break;
    }

    QString definiteness;
    switch (matrix.definiteness) {
    case MatrixDefiniteness::SpdExpected:
        definiteness = QCoreApplication::translate("d26", "SPD expected");
        break;
    case MatrixDefiniteness::Indefinite:
        definiteness = QCoreApplication::translate("d26", "indefinite");
        break;
    case MatrixDefiniteness::Unknown:
        definiteness = QCoreApplication::translate("d26", "definiteness unknown");
        break;
    }
    return QStringLiteral("%1 • %2").arg(symmetry, definiteness);
}

AnalysisCapabilityResolution AnalysisCapabilityResolver::resolve(const AnalysisCapabilityInput &input)
{
    AnalysisCapabilityResolution resolution;

    if (!input.analysisPresent) {
        add(resolution, Axis::AnalysisType, State::Invalid,
            QCoreApplication::translate("d26", "Analiz"),
            QCoreApplication::translate("d26", "Analiz nesnesi bulunamadı."), input.analysisSubject);
        return resolution;
    }

    switch (input.analysisType) {
    case AnalysisType::StaticStructural:
        add(resolution, Axis::AnalysisType, State::Ready,
            QCoreApplication::translate("d26", "Analiz Türü"), displayName(input.analysisType),
            input.analysisSubject);
        break;
    case AnalysisType::NonlinearStatic:
        add(resolution, Axis::AnalysisType,
            input.nonlinearProductConsumerAvailable ? State::Ready : State::SetupOnly,
            QCoreApplication::translate("d26", "Analiz Türü"),
            input.nonlinearProductConsumerAvailable
                ? QCoreApplication::translate("d26", "Nonlinear Static product consumer hazır.")
                : QCoreApplication::translate(
                      "d26", "Nonlinear Static authoring kullanılabilir; general model consumer henüz bağlı değil."),
            input.analysisSubject);
        break;
    case AnalysisType::Modal:
        add(resolution, Axis::AnalysisType, State::Unavailable,
            QCoreApplication::translate("d26", "Analiz Türü"),
            QCoreApplication::translate("d26", "Modal analysis bu product workflow consumer'ında kullanılamaz."),
            input.analysisSubject);
        break;
    }

    switch (input.geometry) {
    case GeometryCapability::ParametricBox:
        add(resolution, Axis::GeometrySource, State::Ready,
            QCoreApplication::translate("d26", "Geometri"),
            QCoreApplication::translate("d26", "Parametric Box • structured-mesh compatible"),
            input.geometrySubject);
        break;
    case GeometryCapability::BoxCompatibleCad:
        add(resolution, Axis::GeometrySource, State::Ready,
            QCoreApplication::translate("d26", "Geometri"),
            QCoreApplication::translate("d26", "CAD boxDescriptor doğrulandı • gerçek Face provenance"),
            input.geometrySubject);
        break;
    case GeometryCapability::UnsupportedCad:
        add(resolution, Axis::GeometrySource, State::SetupOnly,
            QCoreApplication::translate("d26", "Geometri"),
            QCoreApplication::translate(
                "d26", "CAD gövdesi box-compatible değil; arbitrary volume mesh consumer mevcut değil."),
            input.geometrySubject);
        break;
    }

    if (!input.materialAssigned) {
        add(resolution, Axis::MaterialModel, State::Invalid,
            QCoreApplication::translate("d26", "Malzeme"),
            QCoreApplication::translate("d26", "Modele malzeme atanmadı."), input.materialSubject);
    } else if (input.materialModel != MaterialModel::LinearElastic) {
        add(resolution, Axis::MaterialModel, State::Unavailable,
            QCoreApplication::translate("d26", "Malzeme"),
            QCoreApplication::translate(
                "d26", "%1 authoring kartı mevcut; general product solve consumer'ı Beta.3 kapsamında değil.")
                .arg(displayName(input.materialModel)),
            input.materialSubject);
    } else {
        add(resolution, Axis::MaterialModel, State::Ready,
            QCoreApplication::translate("d26", "Malzeme"),
            input.analysisType == AnalysisType::NonlinearStatic
                ? QCoreApplication::translate(
                      "d26", "Linear Elastic authoring • St. Venant–Kirchhoff reference constitutive response")
                : QCoreApplication::translate("d26", "Linear Elastic • small-strain isotropic response"),
            input.materialSubject);
    }

    if (!input.meshPresent) {
        add(resolution, Axis::MeshTopology, State::Invalid,
            QCoreApplication::translate("d26", "Mesh"),
            QCoreApplication::translate("d26", "Mesh üretilmedi. Önce Generate Mesh çalıştırın."),
            input.meshSubject);
    } else if (input.meshStale) {
        add(resolution, Axis::MeshTopology, State::Stale,
            QCoreApplication::translate("d26", "Mesh"),
            QCoreApplication::translate("d26", "Mesh güncel değil. Yeniden üretin."), input.meshSubject);
    } else if (!input.allElementsHex8) {
        add(resolution, Axis::MeshTopology, State::Unavailable,
            QCoreApplication::translate("d26", "Mesh"),
            QCoreApplication::translate("d26", "Current product consumer yalnız Structured HEX8 destekler."),
            input.meshSubject);
    } else {
        add(resolution, Axis::MeshTopology, State::Ready,
            QCoreApplication::translate("d26", "Mesh"),
            QCoreApplication::translate("d26", "Structured HEX8"), input.meshSubject);
    }

    if (input.analysisType == AnalysisType::StaticStructural) {
        add(resolution, Axis::ElementFormulation, State::Ready,
            QCoreApplication::translate("d26", "Element Formülasyonu"),
            QCoreApplication::translate("d26", "HEX8 • linear displacement formulation"), input.settingsSubject);
    } else if (input.analysisType == AnalysisType::NonlinearStatic) {
        add(resolution, Axis::ElementFormulation,
            input.nonlinearProductConsumerAvailable ? State::Ready : State::SetupOnly,
            QCoreApplication::translate("d26", "Element Formülasyonu"),
            QCoreApplication::translate("d26", "Total-Lagrangian HEX8 • full integration"),
            input.settingsSubject);
    } else {
        add(resolution, Axis::ElementFormulation, State::Unavailable,
            QCoreApplication::translate("d26", "Element Formülasyonu"),
            QCoreApplication::translate("d26", "Current workflow için element consumer bulunmuyor."),
            input.settingsSubject);
    }

    if (input.analysisType == AnalysisType::NonlinearStatic) {
        add(resolution, Axis::Kinematics,
            input.largeDeformation ? State::Ready : State::Invalid,
            QCoreApplication::translate("d26", "Kinematik"),
            input.largeDeformation
                ? QCoreApplication::translate("d26", "Large Deformation • Total Lagrangian")
                : QCoreApplication::translate(
                      "d26", "Beta.3 Nonlinear Static consumer için Large Deformation açık olmalıdır."),
            input.settingsSubject);
    } else if (input.analysisType == AnalysisType::StaticStructural) {
        add(resolution, Axis::Kinematics,
            input.largeDeformation ? State::Unavailable : State::Ready,
            QCoreApplication::translate("d26", "Kinematik"),
            input.largeDeformation
                ? QCoreApplication::translate(
                      "d26", "Static Structural DirectLinear consumer Large Deformation kullanamaz; Nonlinear Static seçin.")
                : QCoreApplication::translate("d26", "Small deformation"),
            input.settingsSubject);
    } else {
        add(resolution, Axis::Kinematics, State::Unavailable,
            QCoreApplication::translate("d26", "Kinematik"),
            QCoreApplication::translate("d26", "Current workflow için kinematic consumer bulunmuyor."),
            input.settingsSubject);
    }

    if (input.formulation == ResolvedFormulation::MixedUP) {
        add(resolution, Axis::IncompressibilityFormulation, State::Unavailable,
            QCoreApplication::translate("d26", "Formülasyon"),
            QCoreApplication::translate(
                "d26", "Mixed u-p authoring/verification mevcut; general product consumer henüz etkin değil."),
            input.settingsSubject);
    } else {
        add(resolution, Axis::IncompressibilityFormulation, State::Ready,
            QCoreApplication::translate("d26", "Formülasyon"),
            QCoreApplication::translate("d26", "Displacement-based (u)"), input.settingsSubject);
    }

    State boundaryState = State::Ready;
    QString boundaryDetail;
    ObjectId boundarySubject = input.boundarySubject;
    if (input.activeFixedSupportCount <= 0) {
        boundaryState = State::Invalid;
        boundaryDetail = QCoreApplication::translate(
            "d26", "En az bir aktif Fixed Support tanımlanmalıdır.");
    } else if (input.invalidFixedSupportCount > 0) {
        boundaryState = State::Invalid;
        boundaryDetail = QCoreApplication::translate(
            "d26", "Fixed Support en az bir kısıtlı yer değiştirme bileşeni gerektirir.");
        boundarySubject = input.invalidBoundarySubject;
    } else {
        boundaryDetail = QCoreApplication::translate(
            "d26", "%n aktif Fixed Support", nullptr, input.activeFixedSupportCount);
    }
    add(resolution, Axis::BoundaryCondition, boundaryState,
        QCoreApplication::translate("d26", "Sınır Şartı"), boundaryDetail, boundarySubject);

    State loadState = State::Ready;
    QString loadDetail;
    if (input.activeTotalForceCount <= 0) {
        loadState = State::Invalid;
        loadDetail = QCoreApplication::translate("d26", "En az bir aktif Total Force tanımlanmalıdır.");
    } else if (input.invalidTotalForceCount > 0) {
        loadState = State::Invalid;
        loadDetail = QCoreApplication::translate(
            "d26", "Total Force resultant vektörü finite ve sıfırdan farklı olmalıdır.");
    } else if (!input.totalForceConsumerAvailable) {
        loadState = State::SetupOnly;
        loadDetail = QCoreApplication::translate(
            "d26", "Total Force scope kaydedilebilir; consistent surface integration consumer'ı hazır değil.");
    } else {
        loadDetail = QCoreApplication::translate("d26", "%n aktif Total Force • reference configuration",
                                                 nullptr, input.activeTotalForceCount);
    }
    add(resolution, Axis::LoadType, loadState,
        QCoreApplication::translate("d26", "Yük"), loadDetail,
        input.invalidTotalForceCount > 0 ? input.invalidLoadSubject : input.loadSubject);

    if (input.activeContactCount > 0) {
        add(resolution, Axis::Contact, State::Unavailable,
            QCoreApplication::translate("d26", "Contact Çözücü Desteği"),
            QCoreApplication::translate(
                "d26", "Active Contact tanımı mevcut; model-tabanlı solver consumer henüz etkin değil."),
            input.contactSubject);
        resolution.matrix = MatrixCapability{MatrixSymmetry::Unsymmetric, MatrixDefiniteness::Unknown};
    } else if (input.formulation == ResolvedFormulation::MixedUP) {
        add(resolution, Axis::Contact, State::Ready,
            QCoreApplication::translate("d26", "Contact"),
            QCoreApplication::translate("d26", "Aktif Contact yok."), input.contactSubject);
        resolution.matrix = MatrixCapability{MatrixSymmetry::Symmetric, MatrixDefiniteness::Indefinite};
    } else {
        add(resolution, Axis::Contact, State::Ready,
            QCoreApplication::translate("d26", "Contact"),
            QCoreApplication::translate("d26", "Aktif Contact yok."), input.contactSubject);
        resolution.matrix = MatrixCapability{MatrixSymmetry::Symmetric, MatrixDefiniteness::SpdExpected};
    }

    State backendState = State::Unavailable;
    QString backendDetail;
    switch (input.linearBackend) {
    case LinearBackendCapability::DenseReference:
        if (input.maximumDenseDofCount > 0 && input.dofCount > input.maximumDenseDofCount) {
            backendState = State::Invalid;
            backendDetail = QCoreApplication::translate(
                "d26", "%1 DOF, dense reference sınırı olan %2 DOF değerini aşıyor.")
                .arg(input.dofCount)
                .arg(input.maximumDenseDofCount);
        } else {
            backendState = State::Ready;
            backendDetail = QCoreApplication::translate("d26", "Dense reference • %1").arg(
                matrixCapabilityName(resolution.matrix));
        }
        break;
    case LinearBackendCapability::SparseCg:
        if (resolution.matrix.symmetry == MatrixSymmetry::Symmetric
            && resolution.matrix.definiteness == MatrixDefiniteness::SpdExpected) {
            backendState = State::Ready;
            backendDetail = QCoreApplication::translate("d26", "Sparse CG • SPD-compatible matrix metadata");
        } else {
            backendState = State::Unavailable;
            backendDetail = QCoreApplication::translate(
                "d26", "Sparse CG yalnız symmetric/SPD-compatible sistemlerde kullanılabilir.");
        }
        break;
    case LinearBackendCapability::AppleAccelerateSparseDirect:
        backendState = State::SetupOnly;
        backendDetail = QCoreApplication::translate(
            "d26", "Apple Accelerate sparse direct adapter mevcut; current product route'a bağlı değil.");
        break;
    case LinearBackendCapability::Unsupported:
        backendState = State::Unavailable;
        backendDetail = QCoreApplication::translate("d26", "Seçilen linear backend kullanılamaz.");
        break;
    }
    add(resolution, Axis::LinearBackend, backendState,
        QCoreApplication::translate("d26", "Lineer Çözücü"), backendDetail, input.meshSubject);

    if (input.analysisType == AnalysisType::NonlinearStatic) {
        State algorithmState = State::Ready;
        QString algorithmDetail;
        if (!input.nonlinearControlsValid) {
            algorithmState = State::Invalid;
            algorithmDetail = input.nonlinearControlsError;
        } else if (input.nonlinearAlgorithm != NonlinearAlgorithmCapability::FullNewton
                   && input.nonlinearAlgorithm != NonlinearAlgorithmCapability::ModifiedNewton) {
            algorithmState = State::Unavailable;
            algorithmDetail = QCoreApplication::translate("d26", "Nonlinear algorithm desteklenmiyor.");
        } else if (!input.nonlinearProductConsumerAvailable) {
            algorithmState = State::SetupOnly;
            algorithmDetail = QCoreApplication::translate(
                "d26", "Solver controls geçerli; general model nonlinear consumer henüz bağlı değil.");
        } else {
            algorithmDetail = input.nonlinearAlgorithm == NonlinearAlgorithmCapability::FullNewton
                ? QCoreApplication::translate("d26", "Full Newton • controls backend tarafından tüketilir")
                : QCoreApplication::translate("d26", "Modified Newton • controls backend tarafından tüketilir");
        }
        add(resolution, Axis::NonlinearAlgorithm, algorithmState,
            QCoreApplication::translate("d26", "Nonlinear Solver Controls"), algorithmDetail,
            input.settingsSubject);
    } else {
        add(resolution, Axis::NonlinearAlgorithm, State::Ready,
            QCoreApplication::translate("d26", "Nonlinear Algoritma"),
            QCoreApplication::translate("d26", "Static Structural için uygulanmaz."), input.settingsSubject);
    }

    if (input.requestedResults.isEmpty()) {
        add(resolution, Axis::ResultField, State::Ready,
            QCoreApplication::translate("d26", "Sonuç Alanları"),
            QCoreApplication::translate("d26", "Aktif result definition yok; solve izinli."),
            input.solutionSubject);
    } else {
        for (const RequestedResultCapability &result : input.requestedResults) {
            const bool supportedKind = result.kind == ResultDefinitionKind::TotalDeformation
                || result.kind == ResultDefinitionKind::EquivalentStress
                || result.kind == ResultDefinitionKind::ReactionForce;
            const bool nonlinearReady = input.analysisType != AnalysisType::NonlinearStatic
                || input.nonlinearFinalResultsAvailable;
            add(resolution, Axis::ResultField,
                supportedKind && nonlinearReady ? State::Ready : State::SetupOnly,
                QCoreApplication::translate("d26", "Sonuç Alanı"),
                supportedKind && nonlinearReady
                    ? QCoreApplication::translate("d26", "%1 current consumer tarafından üretilir.")
                          .arg(resultCapabilityName(result.kind))
                    : QCoreApplication::translate(
                          "d26", "%1 tanımı saklanabilir; current consumer field üretmiyor.")
                          .arg(resultCapabilityName(result.kind)),
                result.subject);
        }
    }

    return resolution;
}

} // namespace d26
