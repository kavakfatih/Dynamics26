#include "ObjectDetails.h"

#include "../core/ProjectModel.h"
#include "../services/AnalysisService.h"
#include "../services/GeometryService.h"
#include "../services/MaterialService.h"
#include "../services/MeshService.h"
#include "../services/NamedSelectionService.h"

#include <femcae/femcae.h>

#include <QLabel>

namespace d26 {
namespace {

QString scopeDomainName(const SelectionDomain domain)
{
    switch (domain) {
    case SelectionDomain::Geometry: return QStringLiteral("Geometry");
    case SelectionDomain::Mesh: return QStringLiteral("Mesh");
    case SelectionDomain::ProjectObject: return QStringLiteral("Project Object");
    }
    return QStringLiteral("—");
}

QString scopeKindName(const SelectionKind kind)
{
    switch (kind) {
    case SelectionKind::Object: return QStringLiteral("Object");
    case SelectionKind::Body: return QStringLiteral("Body");
    case SelectionKind::Face: return QStringLiteral("Face");
    case SelectionKind::Edge: return QStringLiteral("Edge");
    case SelectionKind::Vertex: return QStringLiteral("Vertex");
    case SelectionKind::Node: return QStringLiteral("Node");
    case SelectionKind::Element: return QStringLiteral("Element");
    case SelectionKind::Facet: return QStringLiteral("Facet");
    }
    return QStringLiteral("—");
}

} // namespace

ObjectDetails::ObjectDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
}

void ObjectDetails::refresh()
{
    clearSections();
    switch (services_.project->typeOf(objectId_)) {
    case ObjectType::Project:               buildProject(); break;
    case ObjectType::Model:                 buildModel(); break;
    case ObjectType::Body:                  buildBody(); break;
    case ObjectType::MaterialsFolder:       buildMaterialsFolder(); break;
    case ObjectType::SectionsFolder:
    case ObjectType::Section:               buildSections(); break;
    case ObjectType::ConnectionsFolder:     buildConnections(); break;
    case ObjectType::NamedSelectionsFolder: buildNamedSelectionsFolder(); break;
    case ObjectType::NamedSelection:        buildNamedSelection(); break;
    case ObjectType::Solution:              buildSolution(); break;
    default:                                buildModel(); break;
    }
    addStretch();
}

void ObjectDetails::buildProject()
{
    auto *definition = addSection(tr("Definition"));
    const ProjectObject *object = services_.project->object(objectId_);
    definition->addValueRow(tr("Name"), object != nullptr ? object->name : tr("Project"));
    definition->addValueRow(tr("Analyses"), QString::number(services_.project->analyses().size()));

    auto *versions = addSection(tr("Versions"));
    versions->addValueRow(tr("Application"), QStringLiteral(FEMCAE_APP_VERSION));
    versions->addValueRow(tr("GUI Milestone"), QStringLiteral(DYNAMICS26_GUI_MILESTONE));
    versions->addValueRow(tr("Solver Engine"), QStringLiteral("%1.%2.%3")
                                                   .arg(fem_version_major())
                                                   .arg(fem_version_minor())
                                                   .arg(fem_version_patch()));
    versions->addValueRow(tr("C ABI"), QString::number(fem_api_version()));
    versions->addValueRow(tr("Project Schema"), QString::number(fem_project_schema_version()));
    versions->addValueRow(tr("Result Schema"), QString::number(fem_result_schema_version()));
}

void ObjectDetails::buildModel()
{
    const GeometrySummary geometry = services_.geometry->summary();
    auto *definition = addSection(tr("Definition"));
    definition->addValueRow(tr("Bodies"), QString::number(geometry.bodyCount));
    definition->addValueRow(tr("Materials"), QString::number(services_.materials->count()));
    definition->addValueRow(tr("Connections"), QString::number(0));

    auto *mesh = addSection(tr("Mesh"));
    if (services_.mesh->hasMesh()) {
        mesh->addValueRow(tr("Nodes"), QString::number(services_.mesh->nodeCount()));
        mesh->addValueRow(tr("Elements"), QString::number(services_.mesh->elementCount()));
        mesh->addValueRow(tr("Degrees of Freedom"), QString::number(services_.mesh->dofCount()));
    } else {
        mesh->addValueRow(tr("Status"), tr("Mesh üretilmedi"));
    }
}

void ObjectDetails::buildBody()
{
    const ProjectObject *object = services_.project->object(objectId_);
    const GeometrySummary geometry = services_.geometry->summary();
    auto *definition = addSection(tr("Definition"));
    definition->addValueRow(tr("Name"), object != nullptr ? object->name : tr("Body"));
    definition->addValueRow(tr("Source"), geometry.hasGeometry ? geometry.sourceFileName : tr("Parametrik kutu"));
    definition->addValueRow(tr("Persistent ID"), object != nullptr ? QString::number(object->tag) : tr("—"));

    auto *material = addSection(tr("Material"));
    const MaterialDefinition *assigned = services_.materials->assigned();
    material->addValueRow(tr("Assignment"), assigned != nullptr ? assigned->name : tr("—"));

    auto *bounding = addSection(tr("Bounding Box"));
    const MeshService::Definition &meshDefinition = services_.mesh->definition();
    bounding->addValueRow(tr("Length"), QStringLiteral("%1 mm").arg(meshDefinition.lengthMm, 0, 'g', 6));
    bounding->addValueRow(tr("Width"), QStringLiteral("%1 mm").arg(meshDefinition.widthMm, 0, 'g', 6));
    bounding->addValueRow(tr("Height"), QStringLiteral("%1 mm").arg(meshDefinition.heightMm, 0, 'g', 6));
}

void ObjectDetails::buildMaterialsFolder()
{
    auto *definition = addSection(tr("Definition"));
    definition->addValueRow(tr("Materials"), QString::number(services_.materials->count()));
    const MaterialDefinition *assigned = services_.materials->assigned();
    definition->addValueRow(tr("Assigned"), assigned != nullptr ? assigned->name : tr("—"));
    definition->addNote(tr("Malzeme kütüphanesi Neo-Hookean, Mooney-Rivlin, Yeoh ve Ogden "
                           "hyperelastic modellerini taşıyacak şekilde kurgulanmıştır."));
}

void ObjectDetails::buildSections()
{
    const SectionSummary section = services_.geometry->sectionSummary();
    auto *definition = addSection(tr("Definition"));
    if (!section.hasSection) {
        definition->addValueRow(tr("Status"), tr("Kesit içe aktarılmadı"));
        definition->addNote(tr("DXF kesit profili içe aktarıldığında alan ve atalet momentleri "
                               "burada gösterilir."));
        return;
    }
    definition->addValueRow(tr("Source"), section.sourceFileName);
    definition->addValueRow(tr("Contours"), QString::number(section.contourCount));

    const auto &properties = section.properties;
    auto *values = addSection(tr("Section Properties"));
    values->addValueRow(QStringLiteral("A"), QString::number(properties.area, 'g', 8));
    values->addValueRow(QStringLiteral("Cx"), QString::number(properties.centroid.x, 'g', 8));
    values->addValueRow(QStringLiteral("Cy"), QString::number(properties.centroid.y, 'g', 8));
    values->addValueRow(QStringLiteral("Ixx"), QString::number(properties.ixx, 'g', 8));
    values->addValueRow(QStringLiteral("Iyy"), QString::number(properties.iyy, 'g', 8));
    values->addValueRow(QStringLiteral("Ixy"), QString::number(properties.ixy, 'g', 8));
    values->addValueRow(QStringLiteral("J"), QString::number(properties.polarJ, 'g', 8));

    auto *principal = addSection(tr("Principal"), true, true);
    principal->addValueRow(QStringLiteral("I1"), QString::number(properties.principalI1, 'g', 8));
    principal->addValueRow(QStringLiteral("I2"), QString::number(properties.principalI2, 'g', 8));
    principal->addValueRow(tr("Angle"), QStringLiteral("%1 rad").arg(properties.principalAngleRad, 0, 'g', 6));
}

void ObjectDetails::buildConnections()
{
    auto *definition = addSection(tr("Definition"));
    definition->addValueRow(tr("Contact Regions"), QString::number(0));
    definition->addValueRow(tr("Joints"), QString::number(0));
    definition->addNote(tr("Model ağacı contact region ve joint nesnelerini taşıyacak şekilde kurgulanmıştır. "
                           "Çekirdekteki penalty / augmented-Lagrangian temas çözücüsü şu an yalnız "
                           "doğrulama preset'i olarak erişilebilir; keyfi mesh üzerinde temas tanımı "
                           "bu sürümde eklenemez."));
}

void ObjectDetails::buildNamedSelectionsFolder()
{
    auto *definition = addSection(tr("Definition"));
    if (services_.namedSelections == nullptr) {
        definition->addValueRow(tr("Status"), tr("Persistent scope servisi kullanılamıyor"));
        return;
    }
    definition->addValueRow(tr("Named Selections"), QString::number(services_.namedSelections->count()));
    definition->addNote(tr("Named Selection; transient viewport seçimini CAD topology veya FEM mesh "
                           "kimlikleriyle kalıcı bir mühendislik kapsamına dönüştürür."));
}

void ObjectDetails::buildNamedSelection()
{
    auto *definitionSection = addSection(tr("Definition"));
    const ProjectObject *object = services_.project->object(objectId_);
    if (services_.namedSelections == nullptr) {
        definitionSection->addValueRow(tr("Status"), tr("Persistent scope servisi kullanılamıyor"));
        return;
    }

    const NamedSelectionDefinition *definition = services_.namedSelections->byId(objectId_);
    if (definition == nullptr || definition->scope.entities.isEmpty()) {
        definitionSection->addValueRow(tr("Status"), tr("Named Selection tanımı bulunamadı"));
        return;
    }

    const ScopeReference &scope = definition->scope;
    const ScopeEntityReference &first = scope.entities.front();
    definitionSection->addValueRow(tr("Name"), definition->name);
    definitionSection->addValueRow(tr("Domain"), scopeDomainName(first.domain));
    definitionSection->addValueRow(tr("Entity Type"), scopeKindName(first.kind));
    definitionSection->addValueRow(tr("Entities"), QString::number(scope.entities.size()));

    auto *lifecycle = addSection(tr("Lifecycle"));
    lifecycle->addValueRow(tr("Status"), object != nullptr && !object->statusText.isEmpty()
                                            ? object->statusText
                                            : tr("—"));
    if (first.domain == SelectionDomain::Geometry) {
        lifecycle->addValueRow(tr("CAD Revision"), QString::number(scope.sourceRevision));
    } else if (first.domain == SelectionDomain::Mesh) {
        lifecycle->addValueRow(tr("Mesh Generation"), QString::number(scope.sourceRevision));
    }

    auto *identity = addSection(tr("Engineering Identity"), true, true);
    if (first.domain == SelectionDomain::Geometry) {
        identity->addValueRow(tr("Geometry Entity ID"),
                              QString::number(static_cast<qulonglong>(first.geometryEntityId)));
        if (geometrySelectionKindHasBodyParent(first.kind)) {
            identity->addValueRow(tr("Parent Body ID"),
                                  QString::number(static_cast<qulonglong>(first.parentGeometryId)));
        }
        identity->addValueRow(tr("Persistent Key"), first.persistentKey);
        if (scope.entities.size() > 1) {
            identity->addNote(tr("Kimlik bölümü ilk entity'yi gösterir; toplam kapsam %1 entity içeriyor.")
                                  .arg(scope.entities.size()));
        }
    } else if (first.domain == SelectionDomain::Mesh) {
        identity->addValueRow(tr("Mesh Entity ID"),
                              QString::number(static_cast<qulonglong>(first.meshEntityId)));
        if (scope.entities.size() > 1) {
            identity->addNote(tr("Kimlik bölümü ilk entity'yi gösterir; toplam kapsam %1 entity içeriyor.")
                                  .arg(scope.entities.size()));
        }
    }
}

void ObjectDetails::buildSolution()
{
    const ObjectId analysisId = services_.analysis->owningAnalysis(objectId_);
    const AnalysisRecord *record = services_.analysis->analysis(analysisId);
    auto *definition = addSection(tr("Definition"));
    if (record == nullptr || !record->solved) {
        definition->addValueRow(tr("Status"), tr("Çözüm çalıştırılmadı"));
        definition->addNote(tr("Çözüm başarıyla tamamlandığında sonuç nesneleri bu düğümün altında oluşur."));
        return;
    }
    const SolveResults &results = record->solveResults;
    definition->addValueRow(tr("Status"), tr("Solved"));
    definition->addValueRow(tr("Wall Clock"), QStringLiteral("%1 s").arg(results.wallClockSeconds, 0, 'f', 3));

    auto *summary = addSection(tr("Summary"));
    summary->addValueRow(tr("Nodes"), QString::number(results.nodeCount));
    summary->addValueRow(tr("Elements"), QString::number(results.elementCount));
    summary->addValueRow(tr("Degrees of Freedom"), QString::number(results.dofCount()));
    summary->addValueRow(tr("Max Total Deformation"), QStringLiteral("%1 mm").arg(results.maxDisplacementMm, 0, 'g', 6));
    summary->addValueRow(tr("Max Equivalent Stress"), QStringLiteral("%1 MPa").arg(results.maxVonMisesMPa, 0, 'g', 6));
}

} // namespace d26
