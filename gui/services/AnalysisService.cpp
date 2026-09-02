#include "AnalysisService.h"
#include "ContactService.h"

#include <femcae/femcae.h>
#include <femcae/meshing/AssignmentResolver.h>

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonValue>

#include <algorithm>
#include <cmath>
#include <limits>

using namespace femcae::meshing;
using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;

namespace d26 {
namespace {

int faceToInt(const BoxFace face)
{
    return static_cast<int>(face);
}

BoxFace faceFromInt(const int value)
{
    return static_cast<BoxFace>(std::clamp(value, 0, 5));
}

BoundaryScopingMethod scopingMethodFromInt(const int value)
{
    return value == static_cast<int>(BoundaryScopingMethod::NamedSelection)
        ? BoundaryScopingMethod::NamedSelection
        : BoundaryScopingMethod::GeometrySelection;
}

bool parseExactObjectId(const QJsonValue &value, ObjectId *result)
{
    if (result == nullptr) {
        return false;
    }
    if (value.isString()) {
        bool ok = false;
        const qulonglong parsed = value.toString().toULongLong(&ok, 10);
        if (!ok) {
            return false;
        }
        *result = static_cast<ObjectId>(parsed);
        return true;
    }

    // Legacy analysis JSON ObjectId'leri number olarak yazabiliyordu. Double
    // temsilde güvenli integer aralığının dışına çıkmış değerleri sessizce
    // yuvarlamak yerine reddederiz. Yeni Named Selection referansları her zaman
    // decimal string olarak yazılır.
    if (value.isDouble()) {
        constexpr double kMaximumExactJsonInteger = 9007199254740991.0; // 2^53 - 1
        const double numeric = value.toDouble(-1.0);
        if (!std::isfinite(numeric) || numeric < 0.0 || numeric > kMaximumExactJsonInteger
            || std::floor(numeric) != numeric) {
            return false;
        }
        *result = static_cast<ObjectId>(static_cast<qulonglong>(numeric));
        return true;
    }
    return false;
}

ObjectId exactObjectIdOrInvalid(const QJsonValue &value)
{
    ObjectId result = InvalidObjectId;
    (void)parseExactObjectId(value, &result);
    return result;
}

} // namespace

// --- serileştirme ------------------------------------------------------------

QJsonObject SupportDefinition::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("name")] = name;
    object[QStringLiteral("scoping_method")] = static_cast<int>(scopingMethod);
    object[QStringLiteral("scope")] = faceToInt(scope);
    // Persistent engineering ObjectId JSON number değildir. >2^53 kimliklerde
    // IEEE-754 yuvarlama riski olmaması için decimal string saklanır.
    object[QStringLiteral("named_selection_id")] =
        QString::number(static_cast<qulonglong>(namedSelectionId));
    object[QStringLiteral("fix_x")] = fixX;
    object[QStringLiteral("fix_y")] = fixY;
    object[QStringLiteral("fix_z")] = fixZ;
    return object;
}

SupportDefinition SupportDefinition::fromJson(const QJsonObject &object)
{
    SupportDefinition definition;
    definition.name = object.value(QStringLiteral("name")).toString(QStringLiteral("Fixed Support"));
    definition.scopingMethod = scopingMethodFromInt(object.value(QStringLiteral("scoping_method")).toInt(0));
    definition.scope = faceFromInt(object.value(QStringLiteral("scope")).toInt(0));
    definition.namedSelectionId = exactObjectIdOrInvalid(object.value(QStringLiteral("named_selection_id")));
    definition.fixX = object.value(QStringLiteral("fix_x")).toBool(true);
    definition.fixY = object.value(QStringLiteral("fix_y")).toBool(true);
    definition.fixZ = object.value(QStringLiteral("fix_z")).toBool(true);
    return definition;
}

double LoadDefinition::magnitudeN() const
{
    return std::sqrt(fxN * fxN + fyN * fyN + fzN * fzN);
}

QJsonObject LoadDefinition::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("name")] = name;
    object[QStringLiteral("scoping_method")] = static_cast<int>(scopingMethod);
    object[QStringLiteral("scope")] = faceToInt(scope);
    object[QStringLiteral("named_selection_id")] =
        QString::number(static_cast<qulonglong>(namedSelectionId));
    object[QStringLiteral("fx_n")] = fxN;
    object[QStringLiteral("fy_n")] = fyN;
    object[QStringLiteral("fz_n")] = fzN;
    return object;
}

LoadDefinition LoadDefinition::fromJson(const QJsonObject &object)
{
    LoadDefinition definition;
    definition.name = object.value(QStringLiteral("name")).toString(QStringLiteral("Force"));
    definition.scopingMethod = scopingMethodFromInt(object.value(QStringLiteral("scoping_method")).toInt(0));
    definition.scope = faceFromInt(object.value(QStringLiteral("scope")).toInt(1));
    definition.namedSelectionId = exactObjectIdOrInvalid(object.value(QStringLiteral("named_selection_id")));
    definition.fxN = object.value(QStringLiteral("fx_n")).toDouble(0.0);
    definition.fyN = object.value(QStringLiteral("fy_n")).toDouble(0.0);
    definition.fzN = object.value(QStringLiteral("fz_n")).toDouble(0.0);
    return definition;
}

QJsonObject ResultDefinition::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("name")] = name;
    object[QStringLiteral("kind")] = static_cast<int>(kind);
    return object;
}

ResultDefinition ResultDefinition::fromJson(const QJsonObject &object)
{
    ResultDefinition definition;
    definition.kind = static_cast<ResultDefinitionKind>(
        std::clamp(object.value(QStringLiteral("kind")).toInt(0), 0, 2));
    definition.name = object.value(QStringLiteral("name")).toString(displayName(definition.kind));
    return definition;
}

QJsonObject NonlinearSolverControls::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("method")] = static_cast<int>(method);
    object[QStringLiteral("maximum_iterations")] = maximumIterations;
    object[QStringLiteral("adaptive_stepping")] = adaptiveStepping;
    object[QStringLiteral("initial_load_increment")] = initialLoadIncrement;
    object[QStringLiteral("minimum_load_increment")] = minimumLoadIncrement;
    object[QStringLiteral("maximum_load_increment")] = maximumLoadIncrement;
    object[QStringLiteral("line_search")] = lineSearch;
    object[QStringLiteral("residual_relative_tolerance")] = residualRelativeTolerance;
    object[QStringLiteral("displacement_relative_tolerance")] = displacementRelativeTolerance;
    return object;
}

NonlinearSolverControls NonlinearSolverControls::fromJson(const QJsonObject &object)
{
    NonlinearSolverControls controls;
    if (object.isEmpty()) {
        return controls;
    }
    controls.method = static_cast<NonlinearMethodIntent>(object.value(QStringLiteral("method")).toInt(0));
    controls.maximumIterations = object.value(QStringLiteral("maximum_iterations")).toInt(25);
    controls.adaptiveStepping = object.value(QStringLiteral("adaptive_stepping")).toBool(true);
    controls.initialLoadIncrement = object.value(QStringLiteral("initial_load_increment")).toDouble(0.25);
    controls.minimumLoadIncrement = object.value(QStringLiteral("minimum_load_increment")).toDouble(1.0e-4);
    controls.maximumLoadIncrement = object.value(QStringLiteral("maximum_load_increment")).toDouble(0.50);
    controls.lineSearch = object.value(QStringLiteral("line_search")).toBool(true);
    controls.residualRelativeTolerance =
        object.value(QStringLiteral("residual_relative_tolerance")).toDouble(1.0e-8);
    controls.displacementRelativeTolerance =
        object.value(QStringLiteral("displacement_relative_tolerance")).toDouble(1.0e-8);
    return controls;
}

bool NonlinearSolverControls::isValid(QString *error) const
{
    const auto fail = [error](const QString &text) {
        if (error != nullptr) {
            *error = text;
        }
        return false;
    };
    if (method != NonlinearMethodIntent::FullNewton && method != NonlinearMethodIntent::ModifiedNewton) {
        return fail(QStringLiteral("Nonlinear method intent geçersiz."));
    }
    if (maximumIterations < 1) {
        return fail(QStringLiteral("Maximum Newton iterations pozitif olmalı."));
    }
    if (!std::isfinite(initialLoadIncrement) || !std::isfinite(minimumLoadIncrement)
        || !std::isfinite(maximumLoadIncrement) || initialLoadIncrement <= 0.0 || initialLoadIncrement > 1.0) {
        return fail(QStringLiteral("Initial load increment için 0 < Δλ ≤ 1 olmalı."));
    }
    if (minimumLoadIncrement <= 0.0 || maximumLoadIncrement <= 0.0
        || minimumLoadIncrement > maximumLoadIncrement) {
        return fail(QStringLiteral("Minimum/maximum load increment aralığı geçersiz."));
    }
    if (initialLoadIncrement < minimumLoadIncrement || initialLoadIncrement > maximumLoadIncrement) {
        return fail(QStringLiteral("Initial load increment minimum/maximum sınırları içinde olmalı."));
    }
    if (!std::isfinite(residualRelativeTolerance) || !std::isfinite(displacementRelativeTolerance)
        || residualRelativeTolerance < 0.0 || displacementRelativeTolerance < 0.0) {
        return fail(QStringLiteral("Relative convergence toleransları negatif veya sonlu olmayan değer alamaz."));
    }
    if (error != nullptr) {
        error->clear();
    }
    return true;
}

bool PreflightReport::passed() const
{
    return std::none_of(checks.begin(), checks.end(),
                        [](const PreflightCheck &check) { return check.status == PreflightCheck::Status::Failed; });
}

bool PreflightReport::hasWarnings() const
{
    return std::any_of(checks.begin(), checks.end(),
                       [](const PreflightCheck &check) { return check.status == PreflightCheck::Status::Warning; });
}

QString PreflightReport::firstFailure() const
{
    for (const auto &check : checks) {
        if (check.status == PreflightCheck::Status::Failed) {
            return check.detail.isEmpty() ? check.label : check.detail;
        }
    }
    return {};
}

QStringList PreflightReport::failureMessages() const
{
    QStringList messages;
    for (const auto &check : checks) {
        if (check.status == PreflightCheck::Status::Failed) {
            messages << (check.detail.isEmpty() ? check.label : check.detail);
        }
    }
    return messages;
}

// --- yaşam döngüsü -----------------------------------------------------------

AnalysisService::AnalysisService(ProjectModel *project, MeshService *mesh, MaterialService *materials, QObject *parent)
    : QObject(parent), project_(project), mesh_(mesh), materials_(materials)
{
    // B2.4 solve-session telemetry AnalysisService'in derived state'idir.
    // Preflight yalnız bir gate'tir; gerçek solver session Solving ile başlar.
    // Böylece preflight failure için sahte Direct/ Newton session üretilmez.
    connect(this, &AnalysisService::solveStateChanged, this,
            [this](const ObjectId analysisId, const SolveState state) {
        auto it = analyses_.find(analysisId);
        if (it == analyses_.end()) {
            return;
        }

        SolverConvergenceSnapshot next = it->solverTelemetry;
        switch (state) {
        case SolveState::Idle:
        case SolveState::Preflight:
        case SolveState::Ready:
            next = {};
            break;
        case SolveState::Solving:
            next = {};
            next.summary.executionMode = SolverExecutionMode::DirectLinear;
            next.summary.state = SolverConvergenceState::Running;
            break;
        case SolveState::Completed:
            if (next.summary.executionMode != SolverExecutionMode::DirectLinear) {
                next = {};
                next.summary.executionMode = SolverExecutionMode::DirectLinear;
            }
            next.summary.state = SolverConvergenceState::Completed;
            next.entries.clear();
            break;
        case SolveState::Failed:
            // Failed sinyali Preflight failure sonrasında da gelir. Yalnız daha
            // önce Solving ile gerçek DirectLinear session başladıysa failure
            // telemetry'si yayınlanır; aksi halde unavailable kalır.
            if (next.summary.executionMode == SolverExecutionMode::DirectLinear) {
                next.summary.state = SolverConvergenceState::Failed;
                next.entries.clear();
            } else {
                next = {};
            }
            break;
        case SolveState::Cancelled:
            // Mevcut eşzamanlı solver cancellation üretmiyor. İleride gerçek
            // cancellation state contract'a eklendiğinde ayrı ifade edilecek.
            next = {};
            break;
        }

        it->solverTelemetry = next;
        emit solverTelemetryChanged(analysisId);
    });
}

bool AnalysisService::isActive(const ObjectId id) const
{
    return !project_->isEffectivelySuppressed(id);
}

QByteArray AnalysisService::solverInputSignature(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    if (record == nullptr) {
        return {};
    }
    QJsonObject signature;

    // 1) Mesh: hangi üretim ve hangi ölçü/bölme tanımı. Generation de 64-bit
    // engineering identity'dir; JSON number precision riski alınmaz.
    signature[QStringLiteral("mesh_generation")] =
        QString::number(static_cast<qulonglong>(mesh_->generation()));
    signature[QStringLiteral("mesh_up_to_date")] = mesh_->isUpToDate();
    const MeshService::Definition &meshDefinition = mesh_->definition();
    signature[QStringLiteral("mesh")] = QJsonObject{
        {QStringLiteral("l"), meshDefinition.lengthMm}, {QStringLiteral("w"), meshDefinition.widthMm},
        {QStringLiteral("h"), meshDefinition.heightMm}, {QStringLiteral("nx"), meshDefinition.nx},
        {QStringLiteral("ny"), meshDefinition.ny},      {QStringLiteral("nz"), meshDefinition.nz},
        {QStringLiteral("src"), static_cast<int>(meshDefinition.source)}};

    // 2) Malzeme: solver yalnız E ve ν kullanır
    if (const MaterialDefinition *material = materials_->assigned()) {
        signature[QStringLiteral("material")] = QJsonObject{
            {QStringLiteral("model"), static_cast<int>(material->model)},
            {QStringLiteral("e"), material->youngGPa},
            {QStringLiteral("nu"), material->poisson}};
    }

    // 3) Aktif sınır şartları ve yükler (bastırılmış olanlar imzada YOK).
    // Named Selection consumer entity ID kopyalamaz; imza referenced persistent
    // scope'un current engineering identity'sini içerir. Scope değişirse çözüm
    // otomatik olarak OutOfDate olur; sadece ad değişirse olmaz.
    QJsonArray boundary;
    for (const ObjectId id : project_->childrenOf(analysisId)) {
        if (project_->isEffectivelySuppressed(id)) {
            continue;
        }
        if (const SupportDefinition *support = this->support(id)) {
            boundary.append(QJsonObject{{QStringLiteral("k"), 0},
                                        {QStringLiteral("scope"),
                                         boundaryScopeSignature(support->scopingMethod,
                                                                support->scope,
                                                                support->namedSelectionId)},
                                        {QStringLiteral("x"), support->fixX},
                                        {QStringLiteral("y"), support->fixY},
                                        {QStringLiteral("z"), support->fixZ}});
        } else if (const LoadDefinition *load = this->load(id)) {
            boundary.append(QJsonObject{{QStringLiteral("k"), 1},
                                        {QStringLiteral("scope"),
                                         boundaryScopeSignature(load->scopingMethod,
                                                                load->scope,
                                                                load->namedSelectionId)},
                                        {QStringLiteral("fx"), load->fxN},
                                        {QStringLiteral("fy"), load->fyN},
                                        {QStringLiteral("fz"), load->fzN}});
        }
    }
    signature[QStringLiteral("boundary")] = boundary;

    // 4) Çözülen formülasyon ve solver'ın BUGÜN gerçekten tükettiği analiz ayarları.
    // B2.3 nonlinear controls persistent authoring state'tir; general nonlinear
    // model consumer bağlanana kadar bu lineer imzaya dahil edilmez.
    signature[QStringLiteral("formulation")] = static_cast<int>(resolvedFormulation(analysisId));
    signature[QStringLiteral("large_deflection")] = record->largeDeflection;
    signature[QStringLiteral("analysis_type")] = static_cast<int>(record->type);

    return QJsonDocument(signature).toJson(QJsonDocument::Compact);
}

void AnalysisService::touchDefinition(const ObjectId analysisId)
{
    Q_UNUSED(analysisId)
    // Bayatlık artık içerik imzasından türetilir; sayaç tutulmaz.
}

QString AnalysisService::uniqueChildName(const ObjectId analysisId, const ObjectType type, const QString &base) const
{
    int index = 0;
    for (const ObjectId child : project_->childrenOf(analysisId)) {
        if (project_->typeOf(child) == type) {
            ++index;
        }
    }
    return QStringLiteral("%1 %2").arg(base).arg(index + 1);
}

ObjectId AnalysisService::createAnalysis(const AnalysisType type, const int row, const ObjectId requestedId,
                                         const QString &name, const bool withDefaults,
                                         const ObjectId requestedSettingsId, const ObjectId requestedSolutionId)
{
    QString displayed = name;
    if (displayed.isEmpty()) {
        ++analysisCounter_;
        displayed = QStringLiteral("%1 %2").arg(displayName(type)).arg(analysisCounter_);
    }
    const ObjectId analysisId =
        project_->addRootAt(row, ObjectType::Analysis, displayed, static_cast<qint64>(type), requestedId);
    if (analysisId == InvalidObjectId) {
        return InvalidObjectId;
    }

    AnalysisRecord record;
    record.type = type;
    // Alt yapı düğümlerinin kimlikleri de kalıcıdır: proje yeniden açıldığında
    // aynı ObjectId'ler geri gelir.
    record.settingsNode = project_->addObjectAt(analysisId, -1, ObjectType::AnalysisSettings,
                                                QStringLiteral("Analysis Settings"), 0, requestedSettingsId);
    record.solutionNode = project_->addObjectAt(analysisId, -1, ObjectType::Solution,
                                                QStringLiteral("Solution"), 0, requestedSolutionId);
    analyses_.insert(analysisId, record);

    if (withDefaults && type == AnalysisType::StaticStructural) {
        // Tipik başlangıç kurgusu: bir mesnet, bir yük ve üç sonuç TANIMI.
        // Sonuç tanımları çözümden önce de gerçek nesnelerdir (ANSYS'teki gibi).
        (void)insertFixedSupport(analysisId);
        (void)insertForce(analysisId);
        (void)insertResultDefinition(analysisId, ResultDefinitionKind::TotalDeformation);
        (void)insertResultDefinition(analysisId, ResultDefinitionKind::EquivalentStress);
        (void)insertResultDefinition(analysisId, ResultDefinitionKind::ReactionForce);
    }

    emit changed();
    return analysisId;
}

bool AnalysisService::removeAnalysis(const ObjectId analysisId)
{
    if (!analyses_.contains(analysisId)) {
        return false;
    }
    for (const ObjectId id : analyses_[analysisId].supports) {
        supports_.remove(id);
    }
    for (const ObjectId id : analyses_[analysisId].loads) {
        loads_.remove(id);
    }
    for (const ObjectId id : analyses_[analysisId].results) {
        results_.remove(id);
    }
    analyses_.remove(analysisId);
    project_->removeObject(analysisId);
    emit changed();
    return true;
}

void AnalysisService::clearAll()
{
    const QList<ObjectId> ids = analyses_.keys();
    for (const ObjectId id : ids) {
        project_->removeObject(id);
    }
    analyses_.clear();
    supports_.clear();
    loads_.clear();
    results_.clear();
    analysisCounter_ = 0;
    emit changed();
}

const AnalysisRecord *AnalysisService::analysis(const ObjectId analysisId) const
{
    const auto it = analyses_.constFind(analysisId);
    return it == analyses_.constEnd() ? nullptr : &it.value();
}

const SupportDefinition *AnalysisService::support(const ObjectId id) const
{
    const auto it = supports_.constFind(id);
    return it == supports_.constEnd() ? nullptr : &it.value();
}

const LoadDefinition *AnalysisService::load(const ObjectId id) const
{
    const auto it = loads_.constFind(id);
    return it == loads_.constEnd() ? nullptr : &it.value();
}

const ResultDefinition *AnalysisService::resultDefinition(const ObjectId id) const
{
    const auto it = results_.constFind(id);
    return it == results_.constEnd() ? nullptr : &it.value();
}

ObjectId AnalysisService::owningAnalysis(const ObjectId id) const
{
    return project_->ancestorOfType(id, ObjectType::Analysis);
}

void AnalysisService::setIncompressibility(const ObjectId analysisId, const IncompressibilityIntent intent)
{
    if (!analyses_.contains(analysisId) || analyses_[analysisId].incompressibility == intent) {
        return;
    }
    analyses_[analysisId].incompressibility = intent;
    touchDefinition(analysisId);
    emit changed();
}

void AnalysisService::setLargeDeflection(const ObjectId analysisId, const bool enabled)
{
    if (!analyses_.contains(analysisId) || analyses_[analysisId].largeDeflection == enabled) {
        return;
    }
    analyses_[analysisId].largeDeflection = enabled;
    touchDefinition(analysisId);
    emit changed();
}

void AnalysisService::setNonlinearSolverControls(const ObjectId analysisId,
                                                 const NonlinearSolverControls &controls)
{
    if (!analyses_.contains(analysisId) || analyses_[analysisId].nonlinearControls == controls) {
        return;
    }
    analyses_[analysisId].nonlinearControls = controls;
    // Persistent authoring mutation'dır; ancak B2.3'te general model solver bu
    // controls'u tüketmediği için mevcut lineer solution signature değişmez.
    emit changed();
}

void AnalysisService::renameObject(const ObjectId id, const QString &name)
{
    const QString trimmed = name.trimmed();
    if (trimmed.isEmpty()) {
        return;
    }
    if (supports_.contains(id)) {
        supports_[id].name = trimmed;
    } else if (loads_.contains(id)) {
        loads_[id].name = trimmed;
    } else if (results_.contains(id)) {
        results_[id].name = trimmed;
    } else if (!analyses_.contains(id)) {
        return;
    }
    project_->setName(id, trimmed);
    touchDefinition(owningAnalysis(id));
    emit changed();
}

void AnalysisService::refreshBoundaryNode(const ObjectId id)
{
    // Yalnız görünen ad senkronlanır. Nesne DURUMU (Ready / OutOfDate /
    // Suppressed ...) tek yetkili olan DependencyEngine tarafından yazılır.
    if (const SupportDefinition *definition = support(id)) {
        project_->setName(id, definition->name);
    } else if (const LoadDefinition *loadDefinition = load(id)) {
        project_->setName(id, loadDefinition->name);
    }
}

ObjectId AnalysisService::insertFixedSupport(const ObjectId analysisId, const SupportDefinition &definition,
                                             const int row, const ObjectId requestedId)
{
    if (!analyses_.contains(analysisId)) {
        return InvalidObjectId;
    }
    SupportDefinition copy = definition;
    if (copy.name.isEmpty()) {
        copy.name = uniqueChildName(analysisId, ObjectType::FixedSupport, QStringLiteral("Fixed Support"));
    }
    const ObjectId id =
        project_->addObjectAt(analysisId, row, ObjectType::FixedSupport, copy.name, 0, requestedId);
    if (id == InvalidObjectId) {
        return InvalidObjectId;
    }
    supports_.insert(id, copy);
    resyncChildLists(analysisId);
    refreshBoundaryNode(id);
    touchDefinition(analysisId);
    emit changed();
    return id;
}

ObjectId AnalysisService::insertForce(const ObjectId analysisId, const LoadDefinition &definition, const int row,
                                      const ObjectId requestedId)
{
    if (!analyses_.contains(analysisId)) {
        return InvalidObjectId;
    }
    LoadDefinition copy = definition;
    if (copy.name.isEmpty()) {
        copy.name = uniqueChildName(analysisId, ObjectType::Force, QStringLiteral("Force"));
    }
    const ObjectId id = project_->addObjectAt(analysisId, row, ObjectType::Force, copy.name, 0, requestedId);
    if (id == InvalidObjectId) {
        return InvalidObjectId;
    }
    loads_.insert(id, copy);
    resyncChildLists(analysisId);
    refreshBoundaryNode(id);
    touchDefinition(analysisId);
    emit changed();
    return id;
}

ObjectId AnalysisService::insertResultDefinition(const ObjectId analysisId, const ResultDefinitionKind kind,
                                                 const int row, const ObjectId requestedId, const QString &name)
{
    if (!analyses_.contains(analysisId)) {
        return InvalidObjectId;
    }
    const ObjectId solutionNode = analyses_[analysisId].solutionNode;
    if (solutionNode == InvalidObjectId) {
        return InvalidObjectId;
    }
    ResultDefinition definition;
    definition.kind = kind;
    definition.name = name.isEmpty() ? displayName(kind) : name;
    const ObjectId id =
        project_->addObjectAt(solutionNode, row, objectTypeFor(kind), definition.name, 0, requestedId);
    if (id == InvalidObjectId) {
        return InvalidObjectId;
    }
    results_.insert(id, definition);
    resyncChildLists(analysisId);
    refreshResultNodes(analysisId);
    touchDefinition(analysisId);
    emit changed();
    return id;
}

bool AnalysisService::removeBoundaryCondition(const ObjectId id)
{
    const ObjectId analysisId = owningAnalysis(id);
    if (analysisId == InvalidObjectId || !analyses_.contains(analysisId)) {
        return false;
    }
    if (!supports_.contains(id) && !loads_.contains(id)) {
        return false;
    }
    supports_.remove(id);
    loads_.remove(id);
    project_->removeObject(id);
    resyncChildLists(analysisId);
    touchDefinition(analysisId);
    emit changed();
    return true;
}

bool AnalysisService::removeResultDefinition(const ObjectId id)
{
    const ObjectId analysisId = owningAnalysis(id);
    if (analysisId == InvalidObjectId || !analyses_.contains(analysisId) || !results_.contains(id)) {
        return false;
    }
    results_.remove(id);
    project_->removeObject(id);
    resyncChildLists(analysisId);
    touchDefinition(analysisId);
    emit changed();
    return true;
}

void AnalysisService::updateSupport(const ObjectId id, const SupportDefinition &definition)
{
    if (!supports_.contains(id)) {
        return;
    }
    supports_[id] = definition;
    refreshBoundaryNode(id);
    touchDefinition(owningAnalysis(id));
    emit changed();
}

void AnalysisService::updateLoad(const ObjectId id, const LoadDefinition &definition)
{
    if (!loads_.contains(id)) {
        return;
    }
    loads_[id] = definition;
    refreshBoundaryNode(id);
    touchDefinition(owningAnalysis(id));
    emit changed();
}

void AnalysisService::setSuppressed(const ObjectId id, const bool suppressed)
{
    if (project_->object(id) == nullptr || project_->isSuppressed(id) == suppressed) {
        return;
    }
    project_->setSuppressed(id, suppressed);
    refreshBoundaryNode(id);
    const ObjectId analysisId = owningAnalysis(id);
    if (analysisId != InvalidObjectId) {
        touchDefinition(analysisId);
        refreshResultNodes(analysisId);
    }
    emit changed();
}

// --- niyet -> solver implementasyonu -----------------------------------------

ResolvedFormulation AnalysisService::resolvedFormulation(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    if (record == nullptr) {
        return ResolvedFormulation::DisplacementBased;
    }
    const MaterialDefinition *material = materials_->assigned();
    const double poisson = material != nullptr ? material->poisson : 0.3;
    switch (record->incompressibility) {
    case IncompressibilityIntent::Compressible:
        return ResolvedFormulation::DisplacementBased;
    case IncompressibilityIntent::NearlyIncompressible:
        return ResolvedFormulation::MixedUP;
    case IncompressibilityIntent::Automatic:
        // Neredeyse sıkışmaz rejim: displacement-only formülasyon hacimsel
        // kilitlenmeye girer, bu yüzden mixed u-p'ye çözülür.
        return poisson >= 0.475 ? ResolvedFormulation::MixedUP : ResolvedFormulation::DisplacementBased;
    }
    return ResolvedFormulation::DisplacementBased;
}

QString AnalysisService::resolvedElementTechnology(const ObjectId analysisId) const
{
    return resolvedFormulation(analysisId) == ResolvedFormulation::MixedUP
        ? tr("HEX8/P0 — element-associated pressure DOF")
        : tr("HEX8 — full integration, displacement DOF");
}

QString AnalysisService::resolvedLinearSolver() const
{
    // fem_solve_linear_hex8_mesh çekirdekte LINEAR_SOLVER_DENSE_REFERENCE ile
    // çalışır. Bu bilgi kullanıcıya olduğu gibi değil, sınırı ile birlikte verilir.
    return tr("Direct — dense reference");
}

// --- preflight ---------------------------------------------------------------

PreflightReport AnalysisService::preflight(const ObjectId analysisId) const
{
    PreflightReport report;
    const auto add = [&report](const PreflightCheck::Status status, const QString &label, const QString &detail,
                               const ObjectId subject = InvalidObjectId) {
        report.checks.push_back(PreflightCheck{status, label, detail, subject});
    };

    const AnalysisRecord *record = analysis(analysisId);
    if (record == nullptr) {
        add(PreflightCheck::Status::Failed, tr("Analiz"), tr("Analiz nesnesi bulunamadı."));
        return report;
    }

    // 1) Analiz türü
    if (record->type != AnalysisType::StaticStructural) {
        add(PreflightCheck::Status::Failed, tr("Analiz Türü"),
            tr("%1 için model tabanlı GUI çözüm akışı henüz etkin değil.").arg(displayName(record->type)),
            analysisId);
    } else {
        add(PreflightCheck::Status::Passed, tr("Analiz Türü"), displayName(record->type), analysisId);
    }

    // B2.3 nonlinear controls authoritative model state'tir. Geçersiz persisted
    // değerler solver'a ulaşmadan burada bloklanır. Geçerli control state ise
    // consumer desteği anlamına gelmez; Nonlinear Static Analysis Type check'i
    // general model solve'u ayrıca blocking tutar.
    if (record->type == AnalysisType::NonlinearStatic) {
        QString controlError;
        if (!record->nonlinearControls.isValid(&controlError)) {
            add(PreflightCheck::Status::Failed, tr("Nonlinear Solver Controls"), controlError,
                record->settingsNode);
        } else {
            const QString method = record->nonlinearControls.method == NonlinearMethodIntent::FullNewton
                ? tr("Full Newton") : tr("Modified Newton");
            add(PreflightCheck::Status::Passed, tr("Nonlinear Solver Controls"),
                tr("%1 • max %2 iteration • authoring state geçerli; model consumer henüz bağlı değil.")
                    .arg(method)
                    .arg(record->nonlinearControls.maximumIterations),
                record->settingsNode);
        }
    }

    // 2) Geometri
    add(PreflightCheck::Status::Passed, tr("Geometri"),
        mesh_->dimensionsAreDerived() ? tr("CAD gövdesi") : tr("Parametrik kutu gövdesi"));

    // 3) Malzeme ataması
    const MaterialDefinition *material = materials_->assigned();
    if (material == nullptr) {
        add(PreflightCheck::Status::Failed, tr("Malzeme"), tr("Modele malzeme atanmadı."));
    } else if (!material->supportsLinearStaticSolve()) {
        add(PreflightCheck::Status::Failed, tr("Malzeme"),
            tr("Atanan malzeme «%1». Static Structural çözüm yolu lineer izotropik malzeme gerektirir.")
                .arg(displayName(material->model)),
            materials_->assignedMaterialId());
    } else {
        add(PreflightCheck::Status::Passed, tr("Malzeme"), material->name, materials_->assignedMaterialId());
    }

    // 4) Mesh var mı / güncel mi / bozuk eleman
    if (!mesh_->hasMesh()) {
        add(PreflightCheck::Status::Failed, tr("Mesh"), tr("Mesh üretilmedi. Önce Generate Mesh çalıştırın."),
            project_->meshNode());
    } else if (mesh_->isOutOfDate()) {
        add(PreflightCheck::Status::Failed, tr("Mesh"), tr("Mesh güncel değil. Yeniden üretin."),
            project_->meshNode());
    } else {
        add(PreflightCheck::Status::Passed, tr("Mesh"),
            tr("%1 HEX8 • %2 DOF").arg(mesh_->elementCount()).arg(mesh_->dofCount()), project_->meshNode());
    }
    if (mesh_->hasMesh()) {
        const auto quality = mesh_->quality();
        if (quality.invertedElementCount > 0) {
            add(PreflightCheck::Status::Failed, tr("Mesh Kalitesi"),
                tr("%1 ters eleman bulundu.").arg(quality.invertedElementCount), project_->meshNode());
        } else if (quality.minimumScaledJacobian < 0.10) {
            add(PreflightCheck::Status::Warning, tr("Mesh Kalitesi"),
                tr("Minimum scaled Jacobian düşük: %1").arg(quality.minimumScaledJacobian, 0, 'f', 3),
                project_->meshNode());
        } else {
            add(PreflightCheck::Status::Passed, tr("Mesh Kalitesi"),
                tr("min scaled J = %1").arg(quality.minimumScaledJacobian, 0, 'f', 3), project_->meshNode());
        }
    }

    // 5) Aktif (bastırılmamış) sınır şartı ve yük. Consumer kapsamı da bu
    // aşamada doğrulanır: dangling/stale/wrong-domain Named Selection Solve'a
    // ulaşamaz. Mesh güncelse current CAD Face scope'un gerçek node union'ı da
    // boş olamaz.
    const bool meshReadyForScope = mesh_->hasMesh() && !mesh_->isOutOfDate();
    int activeSupports = 0;
    for (const ObjectId id : record->supports) {
        if (!isActive(id)) {
            continue;
        }
        ++activeSupports;
        const SupportDefinition *definition = support(id);
        if (definition == nullptr) {
            add(PreflightCheck::Status::Failed, tr("Sınır Şartı Kapsamı"),
                tr("Sınır şartı tanımı bulunamadı."), id);
            continue;
        }
        if (definition->scopingMethod == BoundaryScopingMethod::NamedSelection || meshReadyForScope) {
            const BoundaryScopeResolution scope = resolveBoundaryScope(*definition);
            if (!scope.valid) {
                add(PreflightCheck::Status::Failed, tr("Sınır Şartı Kapsamı"),
                    tr("%1 — %2").arg(definition->name, scope.error), id);
            } else if (meshReadyForScope && resolvedBoundaryNodeIds(scope).empty()) {
                add(PreflightCheck::Status::Failed, tr("Sınır Şartı Kapsamı"),
                    tr("%1 — scope current FEM Mesh üzerinde node üretmiyor.").arg(definition->name), id);
            } else {
                const int nodeCount = meshReadyForScope
                    ? static_cast<int>(resolvedBoundaryNodeIds(scope).size()) : 0;
                add(PreflightCheck::Status::Passed, tr("Sınır Şartı Kapsamı"),
                    meshReadyForScope
                        ? tr("%1 → %2 • %3 node").arg(definition->name, scope.label).arg(nodeCount)
                        : tr("%1 → %2").arg(definition->name, scope.label),
                    id);
            }
        }
    }

    int activeLoads = 0;
    double totalLoad = 0.0;
    for (const ObjectId id : record->loads) {
        if (!isActive(id)) {
            continue;
        }
        ++activeLoads;
        const LoadDefinition *definition = load(id);
        if (definition == nullptr) {
            add(PreflightCheck::Status::Failed, tr("Yük Kapsamı"), tr("Yük tanımı bulunamadı."), id);
            continue;
        }
        totalLoad += definition->magnitudeN();
        if (definition->scopingMethod == BoundaryScopingMethod::NamedSelection || meshReadyForScope) {
            const BoundaryScopeResolution scope = resolveBoundaryScope(*definition);
            if (!scope.valid) {
                add(PreflightCheck::Status::Failed, tr("Yük Kapsamı"),
                    tr("%1 — %2").arg(definition->name, scope.error), id);
            } else if (meshReadyForScope && resolvedBoundaryNodeIds(scope).empty()) {
                add(PreflightCheck::Status::Failed, tr("Yük Kapsamı"),
                    tr("%1 — scope current FEM Mesh üzerinde node üretmiyor.").arg(definition->name), id);
            } else {
                const int nodeCount = meshReadyForScope
                    ? static_cast<int>(resolvedBoundaryNodeIds(scope).size()) : 0;
                add(PreflightCheck::Status::Passed, tr("Yük Kapsamı"),
                    meshReadyForScope
                        ? tr("%1 → %2 • %3 node").arg(definition->name, scope.label).arg(nodeCount)
                        : tr("%1 → %2").arg(definition->name, scope.label),
                    id);
            }
        }
    }
    if (activeSupports == 0) {
        add(PreflightCheck::Status::Failed, tr("Sınır Şartı"),
            record->supports.isEmpty() ? tr("En az bir sınır şartı tanımlanmalı.")
                                       : tr("Tüm sınır şartları bastırılmış."),
            analysisId);
    } else {
        add(PreflightCheck::Status::Passed, tr("Sınır Şartı"), tr("%n aktif", "", activeSupports), analysisId);
    }
    if (activeLoads == 0) {
        add(PreflightCheck::Status::Failed, tr("Yük"),
            record->loads.isEmpty() ? tr("En az bir yük tanımlanmalı.") : tr("Tüm yükler bastırılmış."), analysisId);
    } else if (totalLoad <= 0.0) {
        add(PreflightCheck::Status::Warning, tr("Yük"), tr("Toplam yük büyüklüğü sıfır."), analysisId);
    } else {
        add(PreflightCheck::Status::Passed, tr("Yük"),
            tr("%1 aktif • Σ|F| = %2 N").arg(activeLoads).arg(totalLoad, 0, 'g', 6), analysisId);
    }

    // 6) Project-level Connections altındaki aktif ContactRegion nesneleri bu
    // Beta.1 aşamasında tüm model analizleri için engineering connection state'i
    // kabul edilir. Contact scope doğrulaması tek sahibi ContactService üzerinden
    // yapılır; AnalysisService entity ID kopyalamaz veya tree display metninden
    // scope türetmez.
    //
    // Kritik güvenlik kuralı: geçerli bir Contact tanımı bile model-tabanlı
    // Static Structural solver Contact consumer'a henüz bağlı değilse Solve'a
    // sessizce giremez. Aksi halde kullanıcı Contact tanımladığını düşünürken
    // solver teması tamamen yok sayarak fiziksel olarak yanlış sonuç üretebilir.
    for (const ObjectId contactId : project_->childrenOf(project_->connectionsNode())) {
        if (project_->typeOf(contactId) != ObjectType::ContactRegion || !isActive(contactId)) {
            continue;
        }
        const ProjectObject *contactNode = project_->object(contactId);
        const QString contactName = contactNode != nullptr && !contactNode->name.isEmpty()
            ? contactNode->name : tr("Contact Region");

        if (contacts_ == nullptr) {
            add(PreflightCheck::Status::Failed, tr("Contact Kapsamı"),
                tr("%1 — Contact engineering servisi AnalysisService'e bağlı değil.").arg(contactName),
                contactId);
            continue;
        }

        const ContactDefinition *definition = contacts_->byId(contactId);
        if (definition == nullptr) {
            add(PreflightCheck::Status::Failed, tr("Contact Kapsamı"),
                tr("%1 — Project tree kimliği var ancak Contact engineering tanımı bulunamadı.").arg(contactName),
                contactId);
            continue;
        }

        const ContactValidationResult validation = contacts_->validate(contactId);
        if (!validation.valid()) {
            const QString diagnostic = contactNode != nullptr && !contactNode->statusText.isEmpty()
                ? contactNode->statusText : tr("Contact source/target kapsamı geçersiz.");
            add(PreflightCheck::Status::Failed, tr("Contact Kapsamı"),
                tr("%1 — %2").arg(definition->name, diagnostic), contactId);
            continue;
        }

        add(PreflightCheck::Status::Passed, tr("Contact Kapsamı"),
            tr("%1 — Source/Target surface kapsamı geçerli.").arg(definition->name), contactId);
        add(PreflightCheck::Status::Failed, tr("Contact Çözücü Desteği"),
            tr("%1 — Bonded Contact tanımı geçerli; model-tabanlı Static Structural solver Contact consumer "
               "henüz etkin değil.")
                .arg(definition->name),
            contactId);
    }

    // 7) Çözülen formülasyon destekleniyor mu
    if (resolvedFormulation(analysisId) == ResolvedFormulation::MixedUP) {
        add(PreflightCheck::Status::Failed, tr("Formülasyon"),
            tr("Seçilen sıkışmazlık davranışı mixed u-p'ye çözülüyor; bu formülasyon keyfi mesh için "
               "henüz model çözümüne bağlı değildir."),
            record->settingsNode);
    } else {
        add(PreflightCheck::Status::Passed, tr("Formülasyon"), tr("Displacement-based (u)"), record->settingsNode);
    }
    if (record->largeDeflection) {
        add(PreflightCheck::Status::Failed, tr("Large Deflection"),
            tr("Geometrik nonlineer çözüm akışı GUI'de henüz etkin değil."), record->settingsNode);
    }

    // 8) Çözücü kapasitesi
    if (mesh_->hasMesh()) {
        const int dof = mesh_->dofCount();
        if (dof > maximumDofThreshold()) {
            add(PreflightCheck::Status::Failed, tr("Çözücü"),
                tr("%1 DOF, doğrudan yoğun referans çözücünün pratik sınırını (%2 DOF) aşıyor.")
                    .arg(dof)
                    .arg(maximumDofThreshold()),
                project_->meshNode());
        } else if (dof > warningDofThreshold()) {
            add(PreflightCheck::Status::Warning, tr("Çözücü"),
                tr("%1 DOF — yoğun çözücüde çözüm süresi uzayabilir.").arg(dof), project_->meshNode());
        } else {
            add(PreflightCheck::Status::Passed, tr("Çözücü"), resolvedLinearSolver());
        }
    }

    // 9) Sonuç tanımı
    int activeResults = 0;
    for (const ObjectId id : record->results) {
        if (isActive(id)) {
            ++activeResults;
        }
    }
    if (activeResults == 0) {
        add(PreflightCheck::Status::Warning, tr("Sonuç Tanımı"),
            tr("Aktif sonuç tanımı yok; çözüm çalışır fakat gösterilecek sonuç olmaz."),
            record->solutionNode);
    } else {
        add(PreflightCheck::Status::Passed, tr("Sonuç Tanımı"), tr("%n tanım", "", activeResults),
            record->solutionNode);
    }

    return report;
}

// --- sonuç düğümleri ---------------------------------------------------------

void AnalysisService::resyncChildLists(const ObjectId analysisId)
{
    if (!analyses_.contains(analysisId)) {
        return;
    }
    AnalysisRecord &record = analyses_[analysisId];
    record.supports.clear();
    record.loads.clear();
    for (const ObjectId child : project_->childrenOf(analysisId)) {
        if (supports_.contains(child)) {
            record.supports.push_back(child);
        } else if (loads_.contains(child)) {
            record.loads.push_back(child);
        }
    }
    record.results.clear();
    if (record.solutionNode != InvalidObjectId) {
        for (const ObjectId child : project_->childrenOf(record.solutionNode)) {
            if (results_.contains(child)) {
                record.results.push_back(child);
            }
        }
    }
}

void AnalysisService::refreshResultNodes(const ObjectId analysisId)
{
    // Yalnız ad senkronlanır; durum DependencyEngine'in işidir.
    const AnalysisRecord *record = analysis(analysisId);
    if (record == nullptr) {
        return;
    }
    for (const ObjectId id : record->results) {
        if (const ResultDefinition *definition = resultDefinition(id)) {
            project_->setName(id, definition->name);
        }
    }
}

bool AnalysisService::solutionIsOutOfDate(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    if (record == nullptr || !record->solved) {
        return false;
    }
    // Sonuç, üretildiği solver girdisine bağlıdır. Girdi imzası değiştiyse
    // (mesh, malzeme, aktif BC/yük, formülasyon) sonuç bayattır; imza aynıysa
    // — örneğin bir değişiklik Undo ile geri alındıysa — yeniden geçerlidir.
    return !mesh_->hasMesh() || record->solvedSignature != solverInputSignature(analysisId);
}

SolveState AnalysisService::solveState(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    return record != nullptr ? record->solveState : SolveState::Idle;
}

void AnalysisService::clearSolution(const ObjectId analysisId)
{
    if (!analyses_.contains(analysisId)) {
        return;
    }
    AnalysisRecord &record = analyses_[analysisId];
    if (!record.solved && record.solveState == SolveState::Idle) {
        return;
    }
    // Analiz, ayarlar, sınır şartları, yükler ve SONUÇ TANIMLARI korunur;
    // yalnız hesaplanmış alan değerleri silinir. Idle transition B2.4 derived
    // solve-session telemetry'sini de kendi signal adapter'ı üzerinden temizler.
    record.solved = false;
    record.solveResults = {};
    record.resultDatabase.clear();
    record.solveState = SolveState::Idle;
    record.solvedSignature.clear();
    refreshResultNodes(analysisId);
    emit message(tr("Çözüm temizlendi; analiz tanımı korundu."), Severity::Info);
    emit solveStateChanged(analysisId, SolveState::Idle);
    emit resultsChanged(analysisId);
    emit changed();
}

// --- çözüm -------------------------------------------------------------------

bool AnalysisService::solve(const ObjectId analysisId)
{
    if (!analyses_.contains(analysisId)) {
        return false;
    }
    AnalysisRecord &record = analyses_[analysisId];

    // Solve doğrudan solver'ı çağırmaz: önce preflight (§24).
    record.solveState = SolveState::Preflight;
    emit solveStateChanged(analysisId, SolveState::Preflight);
    const PreflightReport report = preflight(analysisId);
    emit solverOutput(tr("──────────────────────────────────────────────"));
    emit solverOutput(tr("PRE-FLIGHT"));
    for (const auto &check : report.checks) {
        const QString mark = check.status == PreflightCheck::Status::Passed
            ? QStringLiteral("  ✓ ")
            : (check.status == PreflightCheck::Status::Warning ? QStringLiteral("  ! ") : QStringLiteral("  ✕ "));
        emit solverOutput(mark + check.label + (check.detail.isEmpty() ? QString() : QStringLiteral(" — ") + check.detail));
    }
    if (!report.passed()) {
        emit solverOutput(tr("Çözüm başlatılmadı — preflight başarısız."));
        emit message(report.firstFailure(), Severity::Warning);
        record.solveState = SolveState::Failed;
        emit solveStateChanged(analysisId, SolveState::Failed);
        emit changed();
        return false;
    }
    emit solverOutput(tr("Ready to Solve"));

    record.solveState = SolveState::Solving;
    emit solveStateChanged(analysisId, SolveState::Solving);

    const MaterialDefinition *material = materials_->assigned();
    const SimulationMesh &mesh = mesh_->mesh();

    emit solverOutput(tr("Static Structural çözümü başlatıldı"));
    emit solverOutput(tr("  Mesh          : %1 node, %2 HEX8, %3 DOF")
                          .arg(mesh.nodes.size())
                          .arg(mesh.elements.size())
                          .arg(mesh_->dofCount()));
    emit solverOutput(tr("  Malzeme       : %1  E=%2 GPa  ν=%3")
                          .arg(material->name)
                          .arg(material->youngGPa)
                          .arg(material->poisson));
    emit solverOutput(tr("  Formülasyon   : %1").arg(resolvedElementTechnology(analysisId)));
    emit solverOutput(tr("  Lineer çözücü : %1").arg(resolvedLinearSolver()));

    std::vector<long long> nodeIds;
    std::vector<double> coordinates;
    nodeIds.reserve(mesh.nodes.size());
    coordinates.reserve(3 * mesh.nodes.size());
    for (const auto &node : mesh.nodes) {
        nodeIds.push_back(node.id);
        coordinates.push_back(node.x.x);
        coordinates.push_back(node.x.y);
        coordinates.push_back(node.x.z);
    }
    std::vector<long long> elementIds;
    std::vector<long long> connectivity;
    elementIds.reserve(mesh.elements.size());
    connectivity.reserve(8 * mesh.elements.size());
    for (const auto &element : mesh.elements) {
        elementIds.push_back(element.id);
        for (const auto id : element.nodeIds) {
            connectivity.push_back(id);
        }
    }

    std::vector<long long> constraintNodes;
    std::vector<int> constraintComponents;
    std::vector<double> constraintValues;
    std::vector<long long> loadNodes;
    std::vector<int> loadComponents;
    std::vector<double> loadValues;

    const auto fail = [&](const QString &text) {
        emit solverOutput(QStringLiteral("  ") + text);
        emit message(text, Severity::Error);
        record.solveState = SolveState::Failed;
        emit solveStateChanged(analysisId, SolveState::Failed);
        emit changed();
    };

    // Sınır şartları persistent engineering scope resolver üzerinden current
    // FEM node union'ına çözülür. Display tessellation ID'leri bu zincire girmez.
    for (const ObjectId supportId : record.supports) {
        if (!isActive(supportId)) {
            emit solverOutput(tr("  Sınır şartı   : %1 — BASTIRILDI, atlandı")
                                  .arg(support(supportId) != nullptr ? support(supportId)->name : QString()));
            continue;
        }
        const SupportDefinition *definition = support(supportId);
        if (definition == nullptr) {
            continue;
        }
        const BoundaryScopeResolution scope = resolveBoundaryScope(*definition);
        const auto nodes = resolvedBoundaryNodeIds(scope);
        if (!scope.valid || nodes.empty()) {
            fail(tr("Sınır şartı kapsamı solver node'larına çözülemedi: %1")
                     .arg(scope.error.isEmpty() ? definition->name : scope.error));
            return false;
        }
        for (const auto nodeId : nodes) {
            for (int component = 0; component < 3; ++component) {
                const bool constrained = component == 0 ? definition->fixX
                    : (component == 1 ? definition->fixY : definition->fixZ);
                if (!constrained) {
                    continue;
                }
                constraintNodes.push_back(static_cast<long long>(nodeId));
                constraintComponents.push_back(component + 1);
                constraintValues.push_back(0.0);
            }
        }
        emit solverOutput(tr("  Sınır şartı   : %1 → %2 • %3 node")
                              .arg(definition->name, scope.label)
                              .arg(nodes.size()));
    }

    // Her Force nesnesinin vectorValue değeri TOPLAM kuvvettir. Named Selection
    // birden çok Face içerirse node'lar önce tek union'a indirgenir; total force
    // yüz başına tekrarlanmaz, union node sayısına yalnız BİR KEZ bölünür.
    for (const ObjectId loadId : record.loads) {
        if (!isActive(loadId)) {
            emit solverOutput(tr("  Yük           : %1 — BASTIRILDI, atlandı")
                                  .arg(load(loadId) != nullptr ? load(loadId)->name : QString()));
            continue;
        }
        const LoadDefinition *definition = load(loadId);
        if (definition == nullptr) {
            continue;
        }
        const BoundaryScopeResolution scope = resolveBoundaryScope(*definition);
        const auto nodes = resolvedBoundaryNodeIds(scope);
        if (!scope.valid || nodes.empty()) {
            fail(tr("Yük kapsamı solver node'larına çözülemedi: %1")
                     .arg(scope.error.isEmpty() ? definition->name : scope.error));
            return false;
        }
        const double share = 1.0 / static_cast<double>(nodes.size());
        for (const auto nodeId : nodes) {
            for (int component = 0; component < 3; ++component) {
                const double totalComponent = component == 0 ? definition->fxN
                    : (component == 1 ? definition->fyN : definition->fzN);
                const double value = totalComponent * share;
                if (std::abs(value) < 1.0e-30) {
                    continue;
                }
                loadNodes.push_back(static_cast<long long>(nodeId));
                loadComponents.push_back(component + 1);
                loadValues.push_back(value);
            }
        }
        emit solverOutput(tr("  Yük           : %1 → %2 • %3 node, |F| = %4 N")
                              .arg(definition->name, scope.label)
                              .arg(nodes.size())
                              .arg(definition->magnitudeN(), 0, 'g', 6));
    }

    if (constraintNodes.empty()) {
        fail(tr("Sınır şartı mesh düğümlerine çözülemedi; model serbest cisim durumunda."));
        return false;
    }
    if (loadNodes.empty()) {
        fail(tr("Yük mesh düğümlerine çözülemedi."));
        return false;
    }

    std::vector<double> displacements(3 * mesh.nodes.size(), 0.0);
    std::vector<double> reactions(3 * mesh.nodes.size(), 0.0);
    std::vector<double> vonMises(mesh.elements.size(), 0.0);

    QElapsedTimer timer;
    timer.start();
    const int status = fem_solve_linear_hex8_mesh(
        static_cast<int>(mesh.nodes.size()), nodeIds.data(), coordinates.data(),
        static_cast<int>(mesh.elements.size()), elementIds.data(), connectivity.data(),
        material->youngGPa * 1.0e9, material->poisson,
        static_cast<int>(constraintNodes.size()), constraintNodes.data(), constraintComponents.data(),
        constraintValues.data(),
        static_cast<int>(loadNodes.size()), loadNodes.data(), loadComponents.data(), loadValues.data(),
        displacements.data(), reactions.data(), vonMises.data());
    const double elapsed = static_cast<double>(timer.nsecsElapsed()) * 1.0e-9;

    if (status != 0) {
        record.solved = false;
        record.solveResults = {};
        record.resultDatabase.clear();
        refreshResultNodes(analysisId);
        fail(tr("Fortran lineer çözüm başarısız (status %1).").arg(status));
        return false;
    }

    NodeVectorField displacementField;
    displacementField.name = "displacement";
    ElementScalarField stressField;
    stressField.name = "von_mises";

    SolveResults results;
    results.valid = true;
    results.nodeCount = static_cast<int>(mesh.nodes.size());
    results.elementCount = static_cast<int>(mesh.elements.size());
    results.dofCount = 3 * results.nodeCount;
    results.wallClockSeconds = elapsed;
    results.minVonMisesMPa = std::numeric_limits<double>::max();

    for (std::size_t i = 0; i < mesh.nodes.size(); ++i) {
        const femcae::geometry::Vec3 u{displacements[3 * i], displacements[3 * i + 1], displacements[3 * i + 2]};
        displacementField.values[mesh.nodes[i].id] = u;
        results.maxDisplacementMm = std::max(results.maxDisplacementMm,
                                             std::sqrt(u.x * u.x + u.y * u.y + u.z * u.z) * 1.0e3);
        results.reactionXN += reactions[3 * i];
        results.reactionYN += reactions[3 * i + 1];
        results.reactionZN += reactions[3 * i + 2];
    }
    for (std::size_t i = 0; i < mesh.elements.size(); ++i) {
        stressField.values[mesh.elements[i].id] = vonMises[i];
        results.maxVonMisesMPa = std::max(results.maxVonMisesMPa, vonMises[i] / 1.0e6);
        results.minVonMisesMPa = std::min(results.minVonMisesMPa, vonMises[i] / 1.0e6);
    }
    if (mesh.elements.empty()) {
        results.minVonMisesMPa = 0.0;
    }

    record.resultDatabase.clear();
    record.resultDatabase.setDisplacement(std::move(displacementField));
    record.resultDatabase.setElementScalar(std::move(stressField));

    const auto probe = record.resultDatabase.probeNearestNode(
        mesh, {mesh_->definition().lengthMm * 1.0e-3, mesh_->definition().widthMm * 1.0e-3,
               mesh_->definition().heightMm * 1.0e-3});
    if (probe.has_value()) {
        results.probeNodeId = static_cast<qint64>(probe->nodeId);
        results.probeUxMm = probe->vectorValue.x * 1.0e3;
    }

    record.solveResults = results;
    record.solved = true;
    record.solveState = SolveState::Completed;
    // Bayatlık imzası: bu girdilerden herhangi biri değişirse sonuç OutOfDate olur.
    record.solvedSignature = solverInputSignature(analysisId);

    emit solverOutput(tr("  Çözüm tamamlandı — %1 s").arg(elapsed, 0, 'f', 3));
    emit solverOutput(tr("  max |u|        = %1 mm").arg(results.maxDisplacementMm, 0, 'g', 8));
    emit solverOutput(tr("  max von Mises  = %1 MPa").arg(results.maxVonMisesMPa, 0, 'g', 8));
    emit solverOutput(tr("  ΣR             = (%1, %2, %3) N")
                          .arg(results.reactionXN, 0, 'g', 6)
                          .arg(results.reactionYN, 0, 'g', 6)
                          .arg(results.reactionZN, 0, 'g', 6));
    emit message(tr("Çözüm tamamlandı: max |u| = %1 mm, max von Mises = %2 MPa")
                     .arg(results.maxDisplacementMm, 0, 'g', 6)
                     .arg(results.maxVonMisesMPa, 0, 'g', 6),
                 Severity::Success);

    refreshResultNodes(analysisId);
    emit solveStateChanged(analysisId, SolveState::Completed);
    emit resultsChanged(analysisId);
    emit changed();
    return true;
}

bool AnalysisService::hasResults(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    return record != nullptr && record->solved;
}

const ResultDatabase *AnalysisService::resultDatabase(const ObjectId analysisId) const
{
    const AnalysisRecord *record = analysis(analysisId);
    return record != nullptr && record->solved ? &record->resultDatabase : nullptr;
}

// --- kalıcılık ---------------------------------------------------------------

QJsonObject AnalysisService::analysisToJson(const ObjectId analysisId) const
{
    QJsonObject entry;
    const AnalysisRecord *record = analysis(analysisId);
    const ProjectObject *node = project_->object(analysisId);
    if (record == nullptr || node == nullptr) {
        return entry;
    }
    entry[QStringLiteral("object_id")] = static_cast<qint64>(analysisId);
    entry[QStringLiteral("name")] = node->name;
    entry[QStringLiteral("analysis_type")] = static_cast<int>(record->type);
    entry[QStringLiteral("large_deflection")] = record->largeDeflection;
    entry[QStringLiteral("incompressibility")] = static_cast<int>(record->incompressibility);
    entry[QStringLiteral("nonlinear_solver_controls")] = record->nonlinearControls.toJson();
    entry[QStringLiteral("suppressed")] = project_->isSuppressed(analysisId);
    entry[QStringLiteral("settings_object_id")] = static_cast<qint64>(record->settingsNode);
    entry[QStringLiteral("solution_object_id")] = static_cast<qint64>(record->solutionNode);

    // Sınır şartları ve yükler TEK ve SIRALI bir listede saklanır. İki ayrı
    // dizi kullanmak, ağaçtaki gerçek sıralamayı (ör. Support, Force, Support)
    // kaybederdi; ordering round-trip'i bu yüzden tek liste üzerinden yapılır.
    QJsonArray boundaryArray;
    for (const ObjectId id : project_->childrenOf(analysisId)) {
        QJsonObject child;
        if (supports_.contains(id)) {
            child = supports_.value(id).toJson();
            child[QStringLiteral("kind")] = QStringLiteral("fixed_support");
        } else if (loads_.contains(id)) {
            child = loads_.value(id).toJson();
            child[QStringLiteral("kind")] = QStringLiteral("force");
        } else {
            continue;
        }
        child[QStringLiteral("object_id")] = static_cast<qint64>(id);
        child[QStringLiteral("suppressed")] = project_->isSuppressed(id);
        boundaryArray.append(child);
    }
    entry[QStringLiteral("boundary_conditions")] = boundaryArray;

    QJsonArray resultArray;
    for (const ObjectId id : record->results) {
        QJsonObject child = results_.value(id).toJson();
        child[QStringLiteral("object_id")] = static_cast<qint64>(id);
        child[QStringLiteral("suppressed")] = project_->isSuppressed(id);
        resultArray.append(child);
    }
    entry[QStringLiteral("result_definitions")] = resultArray;
    return entry;
}

ObjectId AnalysisService::restoreAnalysis(const QJsonObject &entry, const int row)
{
    const auto analysisId = static_cast<ObjectId>(entry.value(QStringLiteral("object_id")).toInteger(0));
    const auto type =
        static_cast<AnalysisType>(std::clamp(entry.value(QStringLiteral("analysis_type")).toInt(0), 0, 2));
    const QString name = entry.value(QStringLiteral("name")).toString();
    const auto settingsId = static_cast<ObjectId>(entry.value(QStringLiteral("settings_object_id")).toInteger(0));
    const auto solutionId = static_cast<ObjectId>(entry.value(QStringLiteral("solution_object_id")).toInteger(0));
    const ObjectId created = createAnalysis(type, row, analysisId, name, false, settingsId, solutionId);
    if (created == InvalidObjectId) {
        return InvalidObjectId;
    }
    AnalysisRecord &record = analyses_[created];
    record.largeDeflection = entry.value(QStringLiteral("large_deflection")).toBool(false);
    record.incompressibility = static_cast<IncompressibilityIntent>(
        std::clamp(entry.value(QStringLiteral("incompressibility")).toInt(0), 0, 2));
    record.nonlinearControls = NonlinearSolverControls::fromJson(
        entry.value(QStringLiteral("nonlinear_solver_controls")).toObject());
    project_->setSuppressed(created, entry.value(QStringLiteral("suppressed")).toBool(false));

    // Sıralı tek liste: kaydedilen ağaç sırası birebir yeniden kurulur.
    for (const auto &child : entry.value(QStringLiteral("boundary_conditions")).toArray()) {
        const QJsonObject childObject = child.toObject();
        const auto id = static_cast<ObjectId>(childObject.value(QStringLiteral("object_id")).toInteger(0));
        const bool isLoad = childObject.value(QStringLiteral("kind")).toString() == QStringLiteral("force");
        const ObjectId inserted = isLoad
            ? insertForce(created, LoadDefinition::fromJson(childObject), -1, id)
            : insertFixedSupport(created, SupportDefinition::fromJson(childObject), -1, id);
        if (inserted != InvalidObjectId && childObject.value(QStringLiteral("suppressed")).toBool(false)) {
            project_->setSuppressed(inserted, true);
        }
    }
    for (const auto &child : entry.value(QStringLiteral("result_definitions")).toArray()) {
        const QJsonObject childObject = child.toObject();
        const auto id = static_cast<ObjectId>(childObject.value(QStringLiteral("object_id")).toInteger(0));
        const ResultDefinition definition = ResultDefinition::fromJson(childObject);
        const ObjectId inserted = insertResultDefinition(created, definition.kind, -1, id, definition.name);
        if (inserted != InvalidObjectId && childObject.value(QStringLiteral("suppressed")).toBool(false)) {
            project_->setSuppressed(inserted, true);
        }
    }
    emit changed();
    return created;
}

int AnalysisService::rowOfAnalysis(const ObjectId analysisId) const
{
    return project_->rowOf(analysisId);
}

QJsonObject AnalysisService::toJson() const
{
    QJsonObject root;
    QJsonArray analysisArray;
    // Üst düzey sıralama ProjectModel köklerinden gelir; ordering korunur.
    for (const ObjectId analysisId : project_->analyses()) {
        const QJsonObject entry = analysisToJson(analysisId);
        if (!entry.isEmpty()) {
            analysisArray.append(entry);
        }
    }
    root[QStringLiteral("analyses")] = analysisArray;
    root[QStringLiteral("analysis_counter")] = analysisCounter_;
    return root;
}

void AnalysisService::fromJson(const QJsonObject &object)
{
    clearAll();
    const QJsonArray analysisArray = object.value(QStringLiteral("analyses")).toArray();
    for (const auto &value : analysisArray) {
        (void)restoreAnalysis(value.toObject(), -1);
    }
    analysisCounter_ = object.value(QStringLiteral("analysis_counter")).toInt(analysisArray.size());
    emit changed();
}

QJsonObject AnalysisService::toLegacyLoadJson() const
{
    // V1.0 şeması tek skaler yük taşır. Yeni object model buna SIKIŞTIRILMAZ;
    // bu yalnız eski sürümlerin dosyayı açabilmesi için yazılan bir özettir.
    QJsonObject object;
    double force = 1000.0;
    for (const ObjectId analysisId : project_->analyses()) {
        const AnalysisRecord *record = analysis(analysisId);
        if (record == nullptr || record->type != AnalysisType::StaticStructural || record->loads.isEmpty()) {
            continue;
        }
        if (const LoadDefinition *definition = load(record->loads.first())) {
            force = definition->fxN;
        }
        break;
    }
    object[QStringLiteral("force_n")] = force;
    return object;
}

void AnalysisService::applyLegacyLoadJson(const QJsonObject &object)
{
    const double force = object.value(QStringLiteral("force_n")).toDouble(1000.0);
    for (const ObjectId analysisId : project_->analyses()) {
        const AnalysisRecord *record = analysis(analysisId);
        if (record == nullptr || record->type != AnalysisType::StaticStructural || record->loads.isEmpty()) {
            continue;
        }
        const ObjectId loadId = record->loads.first();
        if (const LoadDefinition *definition = load(loadId)) {
            LoadDefinition updated = *definition;
            updated.fxN = force;
            updateLoad(loadId, updated);
        }
        break;
    }
}

} // namespace d26
