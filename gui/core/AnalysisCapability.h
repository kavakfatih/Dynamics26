#pragma once

// Dynamics26 Beta.3 — typed analysis capability contract.
//
// Bu katman engineering doğrulamasının ikinci bir sahibi değildir.
// AnalysisService current document state'ini typed input'a dönüştürür;
// AnalysisCapabilityResolver yalnız feature/applicability kombinasyonunu çözer.
// Authoritative kullanıcı raporu AnalysisService::preflight() olarak kalır ve
// aynı resolved capability nesnesi Solve girişinde de tüketilir.

#include "ProjectTypes.h"

#include <QString>
#include <QVector>

namespace d26 {

enum class CapabilityState {
    Ready = 0,
    SetupOnly,
    Unavailable,
    Stale,
    Invalid
};

enum class CapabilityAxis {
    AnalysisType = 0,
    GeometrySource,
    MeshTopology,
    ElementFormulation,
    MaterialModel,
    Kinematics,
    IncompressibilityFormulation,
    BoundaryCondition,
    LoadType,
    Contact,
    LinearBackend,
    NonlinearAlgorithm,
    ResultField
};

enum class GeometryCapability {
    ParametricBox = 0,
    BoxCompatibleCad,
    UnsupportedCad
};

enum class ElementFormulationCapability {
    LinearHex8 = 0,
    TotalLagrangianHex8,
    Unsupported
};

enum class NonlinearAlgorithmCapability {
    NotApplicable = 0,
    FullNewton,
    ModifiedNewton,
    Unsupported
};

enum class LinearBackendCapability {
    DenseReference = 0,
    SparseCg,
    AppleAccelerateSparseDirect,
    Unsupported
};

enum class MatrixSymmetry {
    Symmetric = 0,
    Unsymmetric,
    Unknown
};

enum class MatrixDefiniteness {
    SpdExpected = 0,
    Indefinite,
    Unknown
};

struct MatrixCapability {
    MatrixSymmetry symmetry{MatrixSymmetry::Unknown};
    MatrixDefiniteness definiteness{MatrixDefiniteness::Unknown};
};

struct RequestedResultCapability {
    ResultDefinitionKind kind{ResultDefinitionKind::TotalDeformation};
    ObjectId subject{InvalidObjectId};
};

struct AnalysisCapabilityInput {
    bool analysisPresent{false};
    AnalysisType analysisType{AnalysisType::StaticStructural};
    ObjectId analysisSubject{InvalidObjectId};
    ObjectId settingsSubject{InvalidObjectId};

    GeometryCapability geometry{GeometryCapability::ParametricBox};
    ObjectId geometrySubject{InvalidObjectId};

    bool meshPresent{false};
    bool meshStale{false};
    bool allElementsHex8{false};
    ObjectId meshSubject{InvalidObjectId};

    bool materialAssigned{false};
    MaterialModel materialModel{MaterialModel::LinearElastic};
    ObjectId materialSubject{InvalidObjectId};

    bool largeDeformation{false};
    ResolvedFormulation formulation{ResolvedFormulation::DisplacementBased};

    int activeFixedSupportCount{0};
    int invalidFixedSupportCount{0};
    ObjectId invalidBoundarySubject{InvalidObjectId};
    ObjectId boundarySubject{InvalidObjectId};
    int activeTotalForceCount{0};
    int invalidTotalForceCount{0};
    ObjectId invalidLoadSubject{InvalidObjectId};
    ObjectId loadSubject{InvalidObjectId};
    bool totalForceConsumerAvailable{true};

    int activeContactCount{0};
    ObjectId contactSubject{InvalidObjectId};

    LinearBackendCapability linearBackend{LinearBackendCapability::DenseReference};
    int dofCount{0};
    int maximumDenseDofCount{0};

    NonlinearAlgorithmCapability nonlinearAlgorithm{NonlinearAlgorithmCapability::NotApplicable};
    bool nonlinearControlsValid{true};
    QString nonlinearControlsError;
    bool nonlinearProductConsumerAvailable{false};

    QVector<RequestedResultCapability> requestedResults;
    ObjectId solutionSubject{InvalidObjectId};
    bool nonlinearFinalResultsAvailable{false};
};

struct CapabilityDecision {
    CapabilityAxis axis{CapabilityAxis::AnalysisType};
    CapabilityState state{CapabilityState::Invalid};
    QString label;
    QString detail;
    ObjectId subject{InvalidObjectId};

    [[nodiscard]] bool ready() const noexcept { return state == CapabilityState::Ready; }
};

struct AnalysisCapabilityResolution {
    QVector<CapabilityDecision> decisions;
    MatrixCapability matrix;

    [[nodiscard]] bool solveReady() const noexcept;
    [[nodiscard]] const CapabilityDecision *firstBlocking() const noexcept;
    [[nodiscard]] const CapabilityDecision *decision(CapabilityAxis axis) const noexcept;
};

class AnalysisCapabilityResolver final
{
public:
    [[nodiscard]] static AnalysisCapabilityResolution resolve(const AnalysisCapabilityInput &input);
};

[[nodiscard]] QString capabilityStateName(CapabilityState state);
[[nodiscard]] QString matrixCapabilityName(const MatrixCapability &matrix);

} // namespace d26
