#include "ProjectTypes.h"

#include <QCoreApplication>

namespace d26 {

QString displayName(const ObjectType type)
{
    switch (type) {
    case ObjectType::Project:           return QStringLiteral("Project");
    case ObjectType::Model:             return QStringLiteral("Model");
    case ObjectType::GeometryFolder:    return QStringLiteral("Geometry");
    case ObjectType::Body:              return QStringLiteral("Body");
    case ObjectType::MaterialsFolder:   return QStringLiteral("Materials");
    case ObjectType::Material:          return QStringLiteral("Material");
    case ObjectType::SectionsFolder:    return QStringLiteral("Sections");
    case ObjectType::Section:           return QStringLiteral("Section");
    case ObjectType::ConnectionsFolder: return QStringLiteral("Connections");
    case ObjectType::ContactRegion:     return QStringLiteral("Contact Region");
    case ObjectType::Mesh:              return QStringLiteral("Mesh");
    case ObjectType::Analysis:          return QStringLiteral("Analysis");
    case ObjectType::AnalysisSettings:  return QStringLiteral("Analysis Settings");
    case ObjectType::FixedSupport:      return QStringLiteral("Fixed Support");
    case ObjectType::Force:             return QStringLiteral("Force");
    case ObjectType::Solution:          return QStringLiteral("Solution");
    case ObjectType::TotalDeformation:  return QStringLiteral("Total Deformation");
    case ObjectType::EquivalentStress:  return QStringLiteral("Equivalent Stress");
    case ObjectType::ReactionForce:     return QStringLiteral("Reaction Force");
    case ObjectType::ModeShape:         return QStringLiteral("Mode");
    }
    return QStringLiteral("Object");
}

QString displayName(const AnalysisType type)
{
    switch (type) {
    case AnalysisType::StaticStructural: return QStringLiteral("Static Structural");
    case AnalysisType::Modal:            return QStringLiteral("Modal");
    case AnalysisType::NonlinearStatic:  return QStringLiteral("Nonlinear Static");
    }
    return QStringLiteral("Analysis");
}

QString displayName(const MaterialModel model)
{
    switch (model) {
    case MaterialModel::LinearElastic: return QStringLiteral("Linear Elastic — Isotropic");
    case MaterialModel::NeoHookean:    return QStringLiteral("Neo-Hookean");
    case MaterialModel::MooneyRivlin:  return QStringLiteral("Mooney-Rivlin");
    case MaterialModel::Yeoh:          return QStringLiteral("Yeoh");
    case MaterialModel::Ogden:         return QStringLiteral("Ogden");
    }
    return QStringLiteral("Material");
}

QString displayName(const ObjectState state)
{
    switch (state) {
    case ObjectState::None:     return QString();
    case ObjectState::NotReady: return QCoreApplication::translate("d26", "Tanım eksik");
    case ObjectState::Ready:    return QCoreApplication::translate("d26", "Çözüme hazır");
    case ObjectState::UpToDate: return QCoreApplication::translate("d26", "Güncel");
    case ObjectState::OutOfDate: return QCoreApplication::translate("d26", "Güncel değil");
    case ObjectState::Warning:   return QCoreApplication::translate("d26", "Uyarı");
    case ObjectState::Error:     return QCoreApplication::translate("d26", "Hata");
    case ObjectState::Suppressed: return QCoreApplication::translate("d26", "Bastırıldı");
    case ObjectState::Solving:   return QCoreApplication::translate("d26", "Çözülüyor");
    }
    return QString();
}

QString displayName(const ResultDefinitionKind kind)
{
    switch (kind) {
    case ResultDefinitionKind::TotalDeformation: return QStringLiteral("Total Deformation");
    case ResultDefinitionKind::EquivalentStress: return QStringLiteral("Equivalent Stress");
    case ResultDefinitionKind::ReactionForce:    return QStringLiteral("Reaction Force");
    }
    return QStringLiteral("Result");
}

ObjectType objectTypeFor(const ResultDefinitionKind kind)
{
    switch (kind) {
    case ResultDefinitionKind::TotalDeformation: return ObjectType::TotalDeformation;
    case ResultDefinitionKind::EquivalentStress: return ObjectType::EquivalentStress;
    case ResultDefinitionKind::ReactionForce:    return ObjectType::ReactionForce;
    }
    return ObjectType::TotalDeformation;
}

bool isResultDefinition(const ObjectType type)
{
    return type == ObjectType::TotalDeformation || type == ObjectType::EquivalentStress
        || type == ObjectType::ReactionForce;
}

bool supportsSuppression(const ObjectType type)
{
    return type == ObjectType::Body || type == ObjectType::FixedSupport || type == ObjectType::Force
        || isResultDefinition(type);
}

bool supportsRename(const ObjectType type)
{
    return type == ObjectType::Body || type == ObjectType::Material || type == ObjectType::Analysis
        || type == ObjectType::FixedSupport || type == ObjectType::Force || isResultDefinition(type);
}

bool supportsDuplicate(const ObjectType type)
{
    return type == ObjectType::Material || type == ObjectType::FixedSupport || type == ObjectType::Force;
}

bool supportsDelete(const ObjectType type)
{
    return type == ObjectType::Material || type == ObjectType::Analysis || type == ObjectType::FixedSupport
        || type == ObjectType::Force || isResultDefinition(type);
}

ViewportContext viewportContextFor(const ObjectType type)
{
    switch (type) {
    case ObjectType::GeometryFolder:
    case ObjectType::Body:
    case ObjectType::Project:
    case ObjectType::Model:
        return ViewportContext::Geometry;
    case ObjectType::MaterialsFolder:
    case ObjectType::Material:
        return ViewportContext::Materials;
    case ObjectType::SectionsFolder:
    case ObjectType::Section:
        return ViewportContext::Materials;
    case ObjectType::ConnectionsFolder:
    case ObjectType::ContactRegion:
        return ViewportContext::Connections;
    case ObjectType::Mesh:
        return ViewportContext::Mesh;
    case ObjectType::FixedSupport:
    case ObjectType::Force:
        return ViewportContext::Loads;
    case ObjectType::Analysis:
    case ObjectType::AnalysisSettings:
        return ViewportContext::Analysis;
    case ObjectType::Solution:
    case ObjectType::TotalDeformation:
    case ObjectType::EquivalentStress:
    case ObjectType::ReactionForce:
        return ViewportContext::Results;
    case ObjectType::ModeShape:
        return ViewportContext::Modal;
    }
    return ViewportContext::Empty;
}

} // namespace d26
