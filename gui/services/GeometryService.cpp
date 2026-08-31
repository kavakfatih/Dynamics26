#include "GeometryService.h"

#include <femcae/geometry/DxfSectionReader.h>

#include <QFileInfo>

#include <exception>

using namespace femcae::geometry;

namespace d26 {

GeometryService::GeometryService(QObject *parent) : QObject(parent) {}

bool GeometryService::occtAvailable() noexcept
{
    return OcctStepImporter::available();
}

bool GeometryService::importStep(const QString &path)
{
    StepImportResult result;
    try {
        result = importer_.importFile(path.toStdString(), document_);
    } catch (const std::exception &ex) {
        emit message(tr("STEP import istisnası: %1").arg(QString::fromUtf8(ex.what())), Severity::Error);
        return false;
    }
    if (!result.success) {
        emit message(tr("STEP import başarısız: %1").arg(QString::fromStdString(result.message)), Severity::Error);
        return false;
    }
    stepPath_ = path;
    lastEdgeCount_ = static_cast<int>(result.edgeCount);
    lastVertexCount_ = static_cast<int>(result.vertexCount);
    emit message(tr("Geometri içe aktarıldı: %1 body, %2 face, %3 edge (CAD revizyon %4)")
                     .arg(result.bodyCount)
                     .arg(result.faceCount)
                     .arg(result.edgeCount)
                     .arg(document_.revision()),
                 Severity::Success);
    emit changed();
    return true;
}

bool GeometryService::importDxfSection(const QString &path)
{
    DxfSectionReader reader;
    DxfSectionResult result;
    try {
        result = reader.readFile(path.toStdString());
    } catch (const std::exception &ex) {
        emit message(tr("DXF kesit istisnası: %1").arg(QString::fromUtf8(ex.what())), Severity::Error);
        return false;
    }
    if (!result.success) {
        emit message(tr("DXF kesit okunamadı: %1").arg(QString::fromStdString(result.message)), Severity::Error);
        return false;
    }
    try {
        section_.properties = result.profile.properties();
    } catch (const std::exception &ex) {
        emit message(tr("Kesit özelliği hesaplanamadı: %1").arg(QString::fromUtf8(ex.what())), Severity::Error);
        return false;
    }
    dxfPath_ = path;
    section_.hasSection = true;
    section_.sourceFileName = QFileInfo(path).fileName();
    section_.contourCount = static_cast<int>(result.profile.contours().size());
    emit message(tr("DXF kesit doğrulandı: %1 kontur, A = %2")
                     .arg(section_.contourCount)
                     .arg(section_.properties.area, 0, 'g', 6),
                 Severity::Success);
    emit changed();
    return true;
}

void GeometryService::clear()
{
    document_.clear();
    stepPath_.clear();
    dxfPath_.clear();
    section_ = {};
    lastEdgeCount_ = 0;
    lastVertexCount_ = 0;
    emit changed();
}

GeometrySummary GeometryService::summary() const
{
    GeometrySummary summary;
    summary.bodyCount = static_cast<int>(document_.entitiesOfKind(GeometryEntityKind::Body).size());
    summary.faceCount = static_cast<int>(document_.entitiesOfKind(GeometryEntityKind::Face).size());
    summary.edgeCount = lastEdgeCount_ > 0
        ? lastEdgeCount_
        : static_cast<int>(document_.entitiesOfKind(GeometryEntityKind::Edge).size());
    summary.vertexCount = lastVertexCount_ > 0
        ? lastVertexCount_
        : static_cast<int>(document_.entitiesOfKind(GeometryEntityKind::Vertex).size());
    summary.hasGeometry = summary.bodyCount > 0;
    summary.sourcePath = stepPath_;
    summary.sourceFileName = stepPath_.isEmpty() ? QString() : QFileInfo(stepPath_).fileName();
    summary.revision = document_.revision();
    return summary;
}

QVector<GeometryEntityId> GeometryService::bodies() const
{
    QVector<GeometryEntityId> result;
    for (const auto id : document_.entitiesOfKind(GeometryEntityKind::Body)) {
        result.push_back(id);
    }
    return result;
}

QString GeometryService::bodyName(const GeometryEntityId id) const
{
    const auto *entity = document_.find(id);
    if (entity == nullptr || entity->name.empty()) {
        return tr("Body");
    }
    return QString::fromStdString(entity->name);
}

std::optional<TopologyTessellation> GeometryService::displayTopologyTessellation(
    const GeometryEntityId bodyId, const double linearDeflection) const
{
    const auto *entity = document_.find(bodyId);
    if (entity == nullptr || entity->kind != GeometryEntityKind::Body) {
        return std::nullopt;
    }
    try {
        auto tessellation = importer_.tessellateWithTopology(bodyId, linearDeflection);
        if (tessellation.display.triangles.empty() || !tessellation.hasConsistentProvenance()) {
            return std::nullopt;
        }
        // Service, importer cache'inin import anindaki revision bilgisini guncel
        // GeometryDocument revision'iyle hizalar. Selection stale kontrolunun
        // tek dogruluk kaynagi GUI'nin sahip oldugu document revision'idir.
        tessellation.display.sourceRevision = document_.revision();
        return tessellation;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

QVector<TopologyTessellation> GeometryService::displayTopologyScene(const double linearDeflection) const
{
    QVector<TopologyTessellation> scene;
    const auto bodyIds = document_.entitiesOfKind(GeometryEntityKind::Body);
    scene.reserve(static_cast<qsizetype>(bodyIds.size()));
    for (const GeometryEntityId bodyId : bodyIds) {
        const auto tessellation = displayTopologyTessellation(bodyId, linearDeflection);
        if (!tessellation.has_value()) {
            // Eksik bir Body'yi sessizce atlamak Navigator/viewport modelini
            // farklılaştırır. Topology scene bu nedenle all-or-nothing'dir.
            return {};
        }
        scene.push_back(*tessellation);
    }
    return scene;
}

std::optional<EdgeDisplayTessellation> GeometryService::displayEdgeTessellation(
    const GeometryEntityId bodyId, const double linearDeflection) const
{
    const auto *entity = document_.find(bodyId);
    if (entity == nullptr || entity->kind != GeometryEntityKind::Body) {
        return std::nullopt;
    }
    try {
        auto display = importer_.tessellateEdges(bodyId, linearDeflection);
        if (display.lines.empty() || !display.hasConsistentProvenance()) {
            return std::nullopt;
        }
        display.sourceRevision = document_.revision();
        return display;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

std::optional<VertexDisplayPoints> GeometryService::displayVertexPoints(const GeometryEntityId bodyId) const
{
    const auto *entity = document_.find(bodyId);
    if (entity == nullptr || entity->kind != GeometryEntityKind::Body) {
        return std::nullopt;
    }
    try {
        auto display = importer_.displayVertices(bodyId);
        if (display.points.empty() || !display.hasConsistentProvenance()) {
            return std::nullopt;
        }
        display.sourceRevision = document_.revision();
        return display;
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

QVector<EdgeDisplayTessellation> GeometryService::displayEdgeScene(const double linearDeflection) const
{
    QVector<EdgeDisplayTessellation> scene;
    const auto bodyIds = document_.entitiesOfKind(GeometryEntityKind::Body);
    scene.reserve(static_cast<qsizetype>(bodyIds.size()));
    for (const GeometryEntityId bodyId : bodyIds) {
        const auto display = displayEdgeTessellation(bodyId, linearDeflection);
        if (!display.has_value()) {
            return {};
        }
        scene.push_back(*display);
    }
    return scene;
}

QVector<VertexDisplayPoints> GeometryService::displayVertexScene() const
{
    QVector<VertexDisplayPoints> scene;
    const auto bodyIds = document_.entitiesOfKind(GeometryEntityKind::Body);
    scene.reserve(static_cast<qsizetype>(bodyIds.size()));
    for (const GeometryEntityId bodyId : bodyIds) {
        const auto display = displayVertexPoints(bodyId);
        if (!display.has_value()) {
            return {};
        }
        scene.push_back(*display);
    }
    return scene;
}

std::optional<GeometryTessellation> GeometryService::displayTessellation(const GeometryEntityId bodyId,
                                                                        const double linearDeflection) const
{
    const auto topology = displayTopologyTessellation(bodyId, linearDeflection);
    if (!topology.has_value()) {
        return std::nullopt;
    }
    return topology->display;
}

std::optional<GeometryTessellation> GeometryService::firstBodyTessellation() const
{
    const auto ids = document_.entitiesOfKind(GeometryEntityKind::Body);
    if (ids.empty()) {
        return std::nullopt;
    }
    return displayTessellation(ids.front());
}

std::optional<StepAxisAlignedBoxDescriptor> GeometryService::boxDescriptor(const GeometryEntityId bodyId) const
{
    if (document_.find(bodyId) == nullptr) {
        return std::nullopt;
    }
    try {
        return importer_.axisAlignedBoxDescriptor(bodyId);
    } catch (const std::exception &) {
        return std::nullopt;
    }
}

QJsonObject GeometryService::projectJson() const
{
    // Mevcut proje şeması anahtarları korunur; eski dosyalar açılmaya devam eder.
    QJsonObject object;
    object[QStringLiteral("step_path")] = stepPath_;
    object[QStringLiteral("dxf_section_path")] = dxfPath_;
    object[QStringLiteral("cad_revision")] = static_cast<qint64>(document_.revision());
    object[QStringLiteral("contract")] = QStringLiteral("cad_geometry_not_display_tessellation_not_fem_mesh");
    return object;
}

void GeometryService::loadProjectJson(const QJsonObject &object)
{
    clear();
    const QString stepPath = object.value(QStringLiteral("step_path")).toString();
    const QString dxfPath = object.value(QStringLiteral("dxf_section_path")).toString();
    if (!stepPath.isEmpty()) {
        (void)importStep(stepPath);
    }
    if (!dxfPath.isEmpty()) {
        (void)importDxfSection(dxfPath);
    }
}

} // namespace d26
