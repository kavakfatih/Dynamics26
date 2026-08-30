#include "MaterialService.h"

#include <femcae/femcae.h>

#include <QJsonArray>

#include <algorithm>

namespace d26 {
namespace {

const char *kModelKey = "model_index";

} // namespace

QJsonObject MaterialDefinition::toJson() const
{
    QJsonObject object;
    object[QStringLiteral("name")] = name;
    object[QLatin1String(kModelKey)] = static_cast<int>(model);
    object[QStringLiteral("young_gpa")] = youngGPa;
    object[QStringLiteral("poisson")] = poisson;
    object[QStringLiteral("density_kg_m3")] = densityKgM3;
    object[QStringLiteral("bulk_mpa")] = bulkMPa;
    object[QStringLiteral("c10_mpa")] = c10MPa;
    object[QStringLiteral("c01_mpa")] = c01MPa;
    object[QStringLiteral("c20_mpa")] = c20MPa;
    object[QStringLiteral("c30_mpa")] = c30MPa;
    object[QStringLiteral("ogden_terms")] = ogdenTerms;
    QJsonArray mu;
    QJsonArray alpha;
    for (int i = 0; i < 3; ++i) {
        mu.append(ogdenMuMPa[static_cast<std::size_t>(i)]);
        alpha.append(ogdenAlpha[static_cast<std::size_t>(i)]);
    }
    object[QStringLiteral("ogden_mu_mpa")] = mu;
    object[QStringLiteral("ogden_alpha")] = alpha;
    return object;
}

MaterialDefinition MaterialDefinition::fromJson(const QJsonObject &object)
{
    MaterialDefinition definition;
    definition.name = object.value(QStringLiteral("name")).toString(QStringLiteral("Material"));
    definition.model = static_cast<MaterialModel>(
        std::clamp(object.value(QLatin1String(kModelKey)).toInt(0), 0, 4));
    definition.youngGPa = object.value(QStringLiteral("young_gpa")).toDouble(210.0);
    definition.poisson = object.value(QStringLiteral("poisson")).toDouble(0.30);
    definition.densityKgM3 = object.value(QStringLiteral("density_kg_m3")).toDouble(7850.0);
    definition.bulkMPa = object.value(QStringLiteral("bulk_mpa")).toDouble(2000.0);
    definition.c10MPa = object.value(QStringLiteral("c10_mpa")).toDouble(1.0);
    definition.c01MPa = object.value(QStringLiteral("c01_mpa")).toDouble(0.25);
    definition.c20MPa = object.value(QStringLiteral("c20_mpa")).toDouble(0.10);
    definition.c30MPa = object.value(QStringLiteral("c30_mpa")).toDouble(0.01);
    definition.ogdenTerms = std::clamp(object.value(QStringLiteral("ogden_terms")).toInt(2), 1, 3);
    const QJsonArray mu = object.value(QStringLiteral("ogden_mu_mpa")).toArray();
    const QJsonArray alpha = object.value(QStringLiteral("ogden_alpha")).toArray();
    for (int i = 0; i < 3 && i < mu.size(); ++i) {
        definition.ogdenMuMPa[static_cast<std::size_t>(i)] = mu.at(i).toDouble();
    }
    for (int i = 0; i < 3 && i < alpha.size(); ++i) {
        definition.ogdenAlpha[static_cast<std::size_t>(i)] = alpha.at(i).toDouble();
    }
    return definition;
}

// ---------------------------------------------------------------------------

MaterialService::MaterialService(ProjectModel *project, QObject *parent) : QObject(parent), project_(project) {}

void MaterialService::touch()
{
    ++revision_;
}

const MaterialDefinition *MaterialService::byId(const ObjectId id) const
{
    const auto it = materials_.constFind(id);
    return it == materials_.constEnd() ? nullptr : &it.value();
}

const MaterialDefinition *MaterialService::at(const int index) const
{
    if (index < 0 || index >= order_.size()) {
        return nullptr;
    }
    return byId(order_.at(index));
}

int MaterialService::rowOf(const ObjectId id) const
{
    return static_cast<int>(order_.indexOf(id));
}

QString MaterialService::uniqueName(const QString &base) const
{
    QString candidate = base;
    int suffix = 2;
    const auto taken = [this](const QString &name) {
        for (const ObjectId id : order_) {
            if (materials_.value(id).name.compare(name, Qt::CaseInsensitive) == 0) {
                return true;
            }
        }
        return false;
    };
    while (taken(candidate)) {
        candidate = QStringLiteral("%1 %2").arg(base).arg(suffix++);
    }
    return candidate;
}

void MaterialService::refreshNode(const ObjectId id)
{
    // Yalnız ad senkronlanır; nesne durumu DependencyEngine tarafından yazılır.
    if (const MaterialDefinition *definition = byId(id)) {
        project_->setName(id, definition->name);
    }
}

ObjectId MaterialService::createMaterial(const MaterialDefinition &definition, const int row,
                                         const ObjectId requestedId)
{
    MaterialDefinition copy = definition;
    if (requestedId == InvalidObjectId) {
        copy.name = uniqueName(definition.name);
    }
    const ObjectId id = project_->addObjectAt(project_->materialsNode(), row, ObjectType::Material, copy.name,
                                              0, requestedId);
    if (id == InvalidObjectId) {
        return InvalidObjectId;
    }
    materials_.insert(id, copy);
    const int index = (row < 0 || row > order_.size()) ? order_.size() : row;
    order_.insert(index, id);
    if (assigned_ == InvalidObjectId) {
        assigned_ = id;
    }
    for (const ObjectId other : order_) {
        refreshNode(other);
    }
    touch();
    emit changed();
    return id;
}

bool MaterialService::removeMaterial(const ObjectId id)
{
    if (!materials_.contains(id)) {
        return false;
    }
    if (order_.size() <= 1) {
        emit message(tr("Modelde en az bir malzeme bulunmalıdır."), Severity::Warning);
        return false;
    }
    const bool wasAssigned = assigned_ == id;
    materials_.remove(id);
    order_.removeAll(id);
    project_->removeObject(id);
    if (wasAssigned) {
        assigned_ = order_.isEmpty() ? InvalidObjectId : order_.first();
    }
    for (const ObjectId other : order_) {
        refreshNode(other);
    }
    touch();
    emit changed();
    return true;
}

void MaterialService::updateMaterial(const ObjectId id, const MaterialDefinition &definition)
{
    if (!materials_.contains(id)) {
        return;
    }
    materials_[id] = definition;
    refreshNode(id);
    touch();
    emit changed();
}

void MaterialService::renameMaterial(const ObjectId id, const QString &name)
{
    if (!materials_.contains(id) || name.trimmed().isEmpty()) {
        return;
    }
    materials_[id].name = name.trimmed();
    refreshNode(id);
    touch();
    emit changed();
}

void MaterialService::setAssignedMaterial(const ObjectId id)
{
    if (!materials_.contains(id) || assigned_ == id) {
        return;
    }
    assigned_ = id;
    for (const ObjectId other : order_) {
        refreshNode(other);
    }
    touch();
    emit changed();
}

void MaterialService::clear()
{
    for (const ObjectId id : order_) {
        project_->removeObject(id);
    }
    materials_.clear();
    order_.clear();
    assigned_ = InvalidObjectId;
    touch();
}

void MaterialService::resetToDefault()
{
    clear();
    (void)createMaterial(MaterialDefinition{});
    emit changed();
}

HyperelasticPreview MaterialService::preview(const ObjectId id) const
{
    HyperelasticPreview preview;
    const MaterialDefinition *material = byId(id);
    if (material == nullptr) {
        preview.message = tr("Malzeme bulunamadı.");
        return preview;
    }
    if (material->model == MaterialModel::LinearElastic) {
        preview.message = tr("Lineer izotropik malzeme için hyperelastic eğri önizlemesi tanımlı değildir.");
        return preview;
    }

    std::array<double, 6> parameters{};
    int parameterCount = 0;
    switch (material->model) {
    case MaterialModel::NeoHookean:
        parameters[0] = material->c10MPa * 1.0e6;
        parameterCount = 1;
        break;
    case MaterialModel::MooneyRivlin:
        parameters[0] = material->c10MPa * 1.0e6;
        parameters[1] = material->c01MPa * 1.0e6;
        parameterCount = 2;
        break;
    case MaterialModel::Yeoh:
        parameters[0] = material->c10MPa * 1.0e6;
        parameters[1] = material->c20MPa * 1.0e6;
        parameters[2] = material->c30MPa * 1.0e6;
        parameterCount = 3;
        break;
    case MaterialModel::Ogden: {
        const int terms = std::clamp(material->ogdenTerms, 1, 3);
        parameterCount = 2 * terms;
        for (int i = 0; i < terms; ++i) {
            parameters[static_cast<std::size_t>(2 * i)] = material->ogdenMuMPa[static_cast<std::size_t>(i)] * 1.0e6;
            parameters[static_cast<std::size_t>(2 * i + 1)] = material->ogdenAlpha[static_cast<std::size_t>(i)];
        }
        break;
    }
    case MaterialModel::LinearElastic:
        break;
    }

    const double bulk = material->bulkMPa * 1.0e6;
    double initialShear = 0.0;
    const int validation = fem_hyperelastic_validate(static_cast<int>(material->model), bulk, parameterCount,
                                                     parameters.data(), &initialShear);
    if (validation != 0) {
        preview.message = tr("Engine parametre doğrulaması başarısız (status %1).").arg(validation);
        return preview;
    }

    preview.curve.reserve(41);
    for (int i = 0; i <= 40; ++i) {
        const double stretch = 1.0 + 0.025 * i;
        double nominalStress = 0.0;
        double energy = 0.0;
        const int rc = fem_hyperelastic_isochoric_uniaxial_preview(static_cast<int>(material->model), bulk,
                                                                   parameterCount, parameters.data(), stretch,
                                                                   &nominalStress, &energy);
        if (rc != 0) {
            preview.curve.clear();
            preview.message = tr("Önizleme hesabı başarısız (status %1).").arg(rc);
            return preview;
        }
        preview.curve.push_back(QPointF(stretch, nominalStress / 1.0e6));
    }

    preview.ok = true;
    preview.initialShearModulusMPa = initialShear / 1.0e6;
    preview.message = tr("Engine doğrulaması geçti — G₀ = %1 MPa").arg(preview.initialShearModulusMPa, 0, 'g', 6);
    return preview;
}

QJsonObject MaterialService::toJson() const
{
    QJsonObject root;
    QJsonArray array;
    for (const ObjectId id : order_) {
        QJsonObject entry = materials_.value(id).toJson();
        entry[QStringLiteral("object_id")] = static_cast<qint64>(id);
        array.append(entry);
    }
    root[QStringLiteral("materials")] = array;
    root[QStringLiteral("assigned_object_id")] = static_cast<qint64>(assigned_);
    return root;
}

void MaterialService::fromJson(const QJsonObject &object)
{
    clear();
    const QJsonArray array = object.value(QStringLiteral("materials")).toArray();
    for (const auto &value : array) {
        const QJsonObject entry = value.toObject();
        const auto id = static_cast<ObjectId>(entry.value(QStringLiteral("object_id")).toInteger(0));
        (void)createMaterial(MaterialDefinition::fromJson(entry), -1, id);
    }
    if (order_.isEmpty()) {
        (void)createMaterial(MaterialDefinition{});
    }
    const auto assigned = static_cast<ObjectId>(object.value(QStringLiteral("assigned_object_id")).toInteger(0));
    assigned_ = materials_.contains(assigned) ? assigned : order_.first();
    for (const ObjectId id : order_) {
        refreshNode(id);
    }
    touch();
    emit changed();
}

QJsonObject MaterialService::toLegacyJson() const
{
    // V1.0 şeması tek malzeme taşır: atanmış malzeme yazılır.
    QJsonObject object;
    const MaterialDefinition *material = assigned();
    if (material == nullptr) {
        return object;
    }
    object[QStringLiteral("model")] = QStringLiteral("linear_isotropic");
    object[QStringLiteral("name")] = material->name;
    object[QStringLiteral("young_gpa")] = material->youngGPa;
    object[QStringLiteral("poisson")] = material->poisson;
    object[QStringLiteral("density_kg_m3")] = material->densityKgM3;
    object[QStringLiteral("studio_model_index")] = static_cast<int>(material->model);
    object[QStringLiteral("bulk_mpa")] = material->bulkMPa;
    object[QStringLiteral("c10_mpa")] = material->c10MPa;
    object[QStringLiteral("c01_mpa")] = material->c01MPa;
    object[QStringLiteral("c20_mpa")] = material->c20MPa;
    object[QStringLiteral("c30_mpa")] = material->c30MPa;
    object[QStringLiteral("ogden_terms")] = material->ogdenTerms;
    for (int i = 0; i < 3; ++i) {
        object[QStringLiteral("ogden_mu%1_mpa").arg(i + 1)] = material->ogdenMuMPa[static_cast<std::size_t>(i)];
        object[QStringLiteral("ogden_alpha%1").arg(i + 1)] = material->ogdenAlpha[static_cast<std::size_t>(i)];
    }
    return object;
}

void MaterialService::fromLegacyJson(const QJsonObject &object)
{
    clear();
    MaterialDefinition material;
    material.name = object.value(QStringLiteral("name")).toString(QStringLiteral("Structural Steel"));
    material.youngGPa = object.value(QStringLiteral("young_gpa")).toDouble(210.0);
    material.poisson = object.value(QStringLiteral("poisson")).toDouble(0.30);
    material.densityKgM3 = object.value(QStringLiteral("density_kg_m3")).toDouble(7850.0);
    material.model = static_cast<MaterialModel>(
        std::clamp(object.value(QStringLiteral("studio_model_index")).toInt(0), 0, 4));
    material.bulkMPa = object.value(QStringLiteral("bulk_mpa")).toDouble(2000.0);
    material.c10MPa = object.value(QStringLiteral("c10_mpa")).toDouble(1.0);
    material.c01MPa = object.value(QStringLiteral("c01_mpa")).toDouble(0.25);
    material.c20MPa = object.value(QStringLiteral("c20_mpa")).toDouble(0.10);
    material.c30MPa = object.value(QStringLiteral("c30_mpa")).toDouble(0.01);
    material.ogdenTerms = std::clamp(object.value(QStringLiteral("ogden_terms")).toInt(2), 1, 3);
    const double defaultMu[3] = {1.5, 0.5, 0.1};
    const double defaultAlpha[3] = {2.0, -2.0, 4.0};
    for (int i = 0; i < 3; ++i) {
        material.ogdenMuMPa[static_cast<std::size_t>(i)] =
            object.value(QStringLiteral("ogden_mu%1_mpa").arg(i + 1)).toDouble(defaultMu[i]);
        material.ogdenAlpha[static_cast<std::size_t>(i)] =
            object.value(QStringLiteral("ogden_alpha%1").arg(i + 1)).toDouble(defaultAlpha[i]);
    }
    (void)createMaterial(material);
    touch();
    emit changed();
}

} // namespace d26
