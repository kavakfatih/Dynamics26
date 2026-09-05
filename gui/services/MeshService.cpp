#include "MeshService.h"

#include <femcae/meshing/AssignmentResolver.h>

#include <QCoreApplication>

#include <algorithm>
#include <exception>
#include <string>

using namespace femcae::meshing;
using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;

namespace d26 {
namespace {

// STEP/OCCT model birimi milimetredir; solver SI (m) bekler (ADR-0014:
// birim dönüşümü GUI'nin sorumluluğundadır).
constexpr double kMillimetreToMetre = 1.0e-3;

} // namespace

QString displayName(const BoxFace face)
{
    switch (face) {
    case BoxFace::XMin: return QCoreApplication::translate("d26", "X-Min Face");
    case BoxFace::XMax: return QCoreApplication::translate("d26", "X-Max Face");
    case BoxFace::YMin: return QCoreApplication::translate("d26", "Y-Min Face");
    case BoxFace::YMax: return QCoreApplication::translate("d26", "Y-Max Face");
    case BoxFace::ZMin: return QCoreApplication::translate("d26", "Z-Min Face");
    case BoxFace::ZMax: return QCoreApplication::translate("d26", "Z-Max Face");
    }
    return {};
}

MeshService::MeshService(GeometryService *geometry, QObject *parent)
    : QObject(parent), geometry_(geometry)
{
    connect(geometry_, &GeometryService::changed, this, [this] {
        syncFromGeometry();
        emit changed();
    });
    rebuildParametricTopology();
    syncFromGeometry();
}

void MeshService::rebuildParametricTopology()
{
    using femcae::geometry::GeometryEntityKind;
    parametricGeometry_.clear();
    const GeometryEntityId body = parametricGeometry_.addEntity(
        GeometryEntityKind::Body, InvalidGeometryId, "Parametric Box", "parametric-box/body");
    const auto face = [this, body](const char *name, const char *key) {
        return parametricGeometry_.addEntity(GeometryEntityKind::Face, body, name, key);
    };
    parametricBoundary_ = {
        body,
        face("X-Min Face", "parametric-box/face/x-min"),
        face("X-Max Face", "parametric-box/face/x-max"),
        face("Y-Min Face", "parametric-box/face/y-min"),
        face("Y-Max Face", "parametric-box/face/y-max"),
        face("Z-Min Face", "parametric-box/face/z-min"),
        face("Z-Max Face", "parametric-box/face/z-max")};

    for (std::size_t i = 0; i < parametricEdgeIds_.size(); ++i) {
        parametricEdgeIds_[i] = parametricGeometry_.addEntity(
            GeometryEntityKind::Edge, body, "Box Edge " + std::to_string(i + 1),
            "parametric-box/edge/" + std::to_string(i));
    }
    for (std::size_t i = 0; i < parametricVertexIds_.size(); ++i) {
        parametricVertexIds_[i] = parametricGeometry_.addEntity(
            GeometryEntityKind::Vertex, body, "Box Vertex " + std::to_string(i + 1),
            "parametric-box/vertex/" + std::to_string(i));
    }
}

void MeshService::syncFromGeometry()
{
    const auto bodies = geometry_->bodies();
    geometryBoxAvailable_ = false;
    if (bodies.isEmpty()) {
        if (definition_.source == MeshSource::GeometryBoundingBox) {
            definition_.source = MeshSource::ParametricBox;
        }
        return;
    }
    const auto descriptor = geometry_->boxDescriptor(bodies.front());
    if (!descriptor.has_value()) {
        // Kutu olmayan gövde: structured HEX8 baseline bu gövdeyi meshleyemez.
        // Kullanıcı parametrik kutu ile devam eder; sahte bir mesh üretilmez.
        if (definition_.source == MeshSource::GeometryBoundingBox) {
            definition_.source = MeshSource::ParametricBox;
        }
        return;
    }
    geometryBoxAvailable_ = true;
    definition_.source = MeshSource::GeometryBoundingBox;
    definition_.lengthMm = descriptor->max.x - descriptor->min.x;
    definition_.widthMm = descriptor->max.y - descriptor->min.y;
    definition_.heightMm = descriptor->max.z - descriptor->min.z;
}

void MeshService::setDimensions(const double lengthMm, const double widthMm, const double heightMm)
{
    if (dimensionsAreDerived()) {
        return;
    }
    const double nextLength = std::max(1.0e-3, lengthMm);
    const double nextWidth = std::max(1.0e-3, widthMm);
    const double nextHeight = std::max(1.0e-3, heightMm);
    if (qFuzzyCompare(definition_.lengthMm, nextLength)
        && qFuzzyCompare(definition_.widthMm, nextWidth)
        && qFuzzyCompare(definition_.heightMm, nextHeight)) {
        return;
    }
    definition_.lengthMm = nextLength;
    definition_.widthMm = nextWidth;
    definition_.heightMm = nextHeight;
    rebuildParametricTopology();
    ++settingsRevision_;
    emit changed();
}

void MeshService::setDivisions(const int nx, const int ny, const int nz)
{
    if (definition_.nx == std::clamp(nx, 1, 200) && definition_.ny == std::clamp(ny, 1, 200)
        && definition_.nz == std::clamp(nz, 1, 200)) {
        return;
    }
    definition_.nx = std::clamp(nx, 1, 200);
    definition_.ny = std::clamp(ny, 1, 200);
    definition_.nz = std::clamp(nz, 1, 200);
    ++settingsRevision_;
    emit changed();
}

void MeshService::setSource(const MeshSource source)
{
    if (source == MeshSource::GeometryBoundingBox && !geometryBoxAvailable_) {
        return;
    }
    definition_.source = source;
    if (source == MeshSource::GeometryBoundingBox) {
        syncFromGeometry();
    }
    ++settingsRevision_;
    emit changed();
}

bool MeshService::dimensionsAreDerived() const
{
    return definition_.source == MeshSource::GeometryBoundingBox && geometryBoxAvailable_;
}

bool MeshService::hasImportedGeometry() const
{
    return geometry_ != nullptr && geometry_->summary().hasGeometry;
}

const femcae::geometry::GeometryDocument &MeshService::selectionGeometryDocument() const
{
    return hasImportedGeometry() ? geometry_->document() : parametricGeometry_;
}

QVector<femcae::geometry::TopologyTessellation>
MeshService::displaySelectionTopologyScene(const double linearDeflection) const
{
    if (hasImportedGeometry()) {
        return geometry_->displayTopologyScene(linearDeflection);
    }

    femcae::geometry::TopologyTessellation result;
    result.display.sourceGeometryId = parametricBoundary_.body;
    result.display.sourceRevision = parametricGeometry_.revision();
    const double x = definition_.lengthMm * kMillimetreToMetre;
    const double y = definition_.widthMm * kMillimetreToMetre;
    const double z = definition_.heightMm * kMillimetreToMetre;
    result.display.points = {{0, 0, 0}, {x, 0, 0}, {x, y, 0}, {0, y, 0},
                             {0, 0, z}, {x, 0, z}, {x, y, z}, {0, y, z}};
    result.display.triangles = {{{0, 3, 2}}, {{0, 2, 1}}, {{4, 5, 6}}, {{4, 6, 7}},
                                {{0, 1, 5}}, {{0, 5, 4}}, {{1, 2, 6}}, {{1, 6, 5}},
                                {{2, 3, 7}}, {{2, 7, 6}}, {{3, 0, 4}}, {{3, 4, 7}}};
    result.triangleFaceIds = {parametricBoundary_.zMin, parametricBoundary_.zMin,
                              parametricBoundary_.zMax, parametricBoundary_.zMax,
                              parametricBoundary_.yMin, parametricBoundary_.yMin,
                              parametricBoundary_.xMax, parametricBoundary_.xMax,
                              parametricBoundary_.yMax, parametricBoundary_.yMax,
                              parametricBoundary_.xMin, parametricBoundary_.xMin};
    return {result};
}

QVector<femcae::geometry::EdgeDisplayTessellation>
MeshService::displaySelectionEdgeScene(const double linearDeflection) const
{
    if (hasImportedGeometry()) {
        return geometry_->displayEdgeScene(linearDeflection);
    }
    Q_UNUSED(linearDeflection)
    femcae::geometry::EdgeDisplayTessellation result;
    result.sourceGeometryId = parametricBoundary_.body;
    result.sourceRevision = parametricGeometry_.revision();
    const double x = definition_.lengthMm * kMillimetreToMetre;
    const double y = definition_.widthMm * kMillimetreToMetre;
    const double z = definition_.heightMm * kMillimetreToMetre;
    result.points = {{0, 0, 0}, {x, 0, 0}, {x, y, 0}, {0, y, 0},
                     {0, 0, z}, {x, 0, z}, {x, y, z}, {0, y, z}};
    result.lines = {{{0, 1}}, {{1, 2}}, {{2, 3}}, {{3, 0}},
                    {{4, 5}}, {{5, 6}}, {{6, 7}}, {{7, 4}},
                    {{0, 4}}, {{1, 5}}, {{2, 6}}, {{3, 7}}};
    result.lineEdgeIds.assign(parametricEdgeIds_.begin(), parametricEdgeIds_.end());
    return {result};
}

QVector<femcae::geometry::VertexDisplayPoints> MeshService::displaySelectionVertexScene() const
{
    if (hasImportedGeometry()) {
        return geometry_->displayVertexScene();
    }
    femcae::geometry::VertexDisplayPoints result;
    result.sourceGeometryId = parametricBoundary_.body;
    result.sourceRevision = parametricGeometry_.revision();
    const double x = definition_.lengthMm * kMillimetreToMetre;
    const double y = definition_.widthMm * kMillimetreToMetre;
    const double z = definition_.heightMm * kMillimetreToMetre;
    result.points = {{0, 0, 0}, {x, 0, 0}, {x, y, 0}, {0, y, 0},
                     {0, 0, z}, {x, 0, z}, {x, y, z}, {0, y, z}};
    result.pointVertexIds.assign(parametricVertexIds_.begin(), parametricVertexIds_.end());
    return {result};
}

int MeshService::predictedNodeCount() const
{
    return (definition_.nx + 1) * (definition_.ny + 1) * (definition_.nz + 1);
}

int MeshService::predictedElementCount() const
{
    return definition_.nx * definition_.ny * definition_.nz;
}

bool MeshService::generate()
{
    BoxBoundaryGeometry boundary = parametricBoundary_;
    AxisAlignedBox box{{0.0, 0.0, 0.0},
                       {definition_.lengthMm * kMillimetreToMetre,
                        definition_.widthMm * kMillimetreToMetre,
                        definition_.heightMm * kMillimetreToMetre}};
    quint64 revision = 0;

    if (dimensionsAreDerived()) {
        const auto bodies = geometry_->bodies();
        const auto descriptor = geometry_->boxDescriptor(bodies.front());
        if (descriptor.has_value()) {
            // Gerçek CAD yüz kimlikleri devralınır: sınır koşulları böylece
            // sentetik değil, gerçek geometri seçimi üzerinden kapsamlanır.
            boundary = BoxBoundaryGeometry{descriptor->bodyId,  descriptor->xMinFace, descriptor->xMaxFace,
                                           descriptor->yMinFace, descriptor->yMaxFace, descriptor->zMinFace,
                                           descriptor->zMaxFace};
            box = AxisAlignedBox{{descriptor->min.x * kMillimetreToMetre,
                                  descriptor->min.y * kMillimetreToMetre,
                                  descriptor->min.z * kMillimetreToMetre},
                                 {descriptor->max.x * kMillimetreToMetre,
                                  descriptor->max.y * kMillimetreToMetre,
                                  descriptor->max.z * kMillimetreToMetre}};
            revision = geometry_->summary().revision;
        }
    }

    StructuredHexMesherOptions options;
    options.nx = static_cast<std::size_t>(definition_.nx);
    options.ny = static_cast<std::size_t>(definition_.ny);
    options.nz = static_cast<std::size_t>(definition_.nz);

    try {
        const StructuredHexMesher mesher;
        mesh_ = mesher.meshBox(box, boundary, revision, options);
        quality_ = evaluateHexMeshQuality(mesh_);
    } catch (const std::exception &ex) {
        mesh_ = {};
        quality_ = {};
        emit message(tr("Mesh üretimi başarısız: %1").arg(QString::fromUtf8(ex.what())), Severity::Error);
        emit changed();
        return false;
    }

    boundary_ = boundary;
    meshedGeometryRevision_ = revision;
    generatedDefinition_ = definition_;
    hasGeneratedDefinition_ = true;
    ++generation_;
    emit message(tr("Mesh üretildi: %1 node, %2 HEX8, min scaled Jacobian %3")
                     .arg(mesh_.nodes.size())
                     .arg(mesh_.elements.size())
                     .arg(quality_.minimumScaledJacobian, 0, 'f', 3),
                 Severity::Success);
    emit changed();
    return true;
}

void MeshService::clearGenerated()
{
    // Üretilmiş düğüm/eleman/kalite verisi silinir; ölçüler, bölmeler ve
    // yöntem (yani PROJE TANIMI) korunur.
    if (!hasMesh()) {
        return;
    }
    mesh_ = {};
    quality_ = {};
    boundary_ = {};
    meshedGeometryRevision_ = 0;
    hasGeneratedDefinition_ = false;
    ++generation_;
    emit message(tr("Üretilmiş mesh temizlendi; mesh tanımı korundu."), Severity::Info);
    emit changed();
}

void MeshService::reset()
{
    mesh_ = {};
    quality_ = {};
    boundary_ = {};
    meshedGeometryRevision_ = 0;
    hasGeneratedDefinition_ = false;
    ++generation_;
    definition_ = Definition{};
    rebuildParametricTopology();
    ++settingsRevision_;
    syncFromGeometry();
    emit changed();
}

GeometryEntityId MeshService::geometryIdFor(const BoxFace face) const
{
    switch (face) {
    case BoxFace::XMin: return boundary_.xMin;
    case BoxFace::XMax: return boundary_.xMax;
    case BoxFace::YMin: return boundary_.yMin;
    case BoxFace::YMax: return boundary_.yMax;
    case BoxFace::ZMin: return boundary_.zMin;
    case BoxFace::ZMax: return boundary_.zMax;
    }
    return InvalidGeometryId;
}

std::optional<MeshService::FaceMeasurement> MeshService::selectionFaceMeasurement(
    const GeometryEntityId faceId) const
{
    const auto *entity = selectionGeometryDocument().find(faceId);
    if (!entity || entity->kind != femcae::geometry::GeometryEntityKind::Face) return std::nullopt;
    auto boundary = parametricBoundary_;
    std::array<double, 3> lengths{definition_.lengthMm, definition_.widthMm, definition_.heightMm};
    if (hasImportedGeometry()) {
        const auto box = geometry_->boxDescriptor(entity->parentId);
        if (!box) return std::nullopt;
        boundary = BoxBoundaryGeometry{box->bodyId, box->xMinFace, box->xMaxFace,
            box->yMinFace, box->yMaxFace, box->zMinFace, box->zMaxFace};
        lengths = {box->max.x - box->min.x, box->max.y - box->min.y, box->max.z - box->min.z};
    }
    const std::array<GeometryEntityId, 6> ids{boundary.xMin, boundary.xMax,
        boundary.yMin, boundary.yMax, boundary.zMin, boundary.zMax};
    for (int i = 0; i < 6; ++i) {
        if (ids[i] != faceId) continue;
        FaceMeasurement result;
        const int axis = i / 2;
        // CAD/authoring uzunlukları mm; alan SI m² olarak saklanır.
        result.areaM2 = lengths[(axis + 1) % 3] * lengths[(axis + 2) % 3] * 1.0e-6;
        result.outwardNormal[axis] = i % 2 == 0 ? -1.0 : 1.0;
        return result;
    }
    return std::nullopt;
}

int MeshService::facetCountFor(const BoxFace face) const
{
    const GeometryEntityId id = geometryIdFor(face);
    if (id == InvalidGeometryId) {
        return 0;
    }
    return static_cast<int>(mesh_.facetIdsForGeometry(id).size());
}

int MeshService::nodeCountFor(const BoxFace face) const
{
    const GeometryEntityId id = geometryIdFor(face);
    if (id == InvalidGeometryId) {
        return 0;
    }
    return static_cast<int>(boundaryNodeIdsForGeometry(mesh_, id).size());
}

bool MeshService::isUpToDate() const
{
    if (!hasMesh()) {
        return false;
    }
    // Mesh ayarları üretim anındaki değerlerden farklıysa mesh bayattır.
    if (!hasGeneratedDefinition_ || generatedDefinition_ != definition_) {
        return false;
    }
    if (!dimensionsAreDerived()) {
        return true;
    }
    // CAD tarafından sürülüyorsa geometri revizyonu da eşleşmelidir.
    return meshedGeometryRevision_ == geometry_->summary().revision;
}

QJsonObject MeshService::projectJson() const
{
    QJsonObject object;
    object[QStringLiteral("length_mm")] = definition_.lengthMm;
    object[QStringLiteral("width_mm")] = definition_.widthMm;
    object[QStringLiteral("height_mm")] = definition_.heightMm;
    object[QStringLiteral("nx")] = definition_.nx;
    object[QStringLiteral("ny")] = definition_.ny;
    object[QStringLiteral("nz")] = definition_.nz;
    object[QStringLiteral("source_is_geometry")] = definition_.source == MeshSource::GeometryBoundingBox;
    object[QStringLiteral("meshing_contract")] = QStringLiteral("geometry_provenance_not_display_tessellation");
    return object;
}

void MeshService::loadProjectJson(const QJsonObject &object)
{
    reset();
    definition_.lengthMm = object.value(QStringLiteral("length_mm")).toDouble(100.0);
    definition_.widthMm = object.value(QStringLiteral("width_mm")).toDouble(20.0);
    definition_.heightMm = object.value(QStringLiteral("height_mm")).toDouble(20.0);
    definition_.nx = object.value(QStringLiteral("nx")).toInt(20);
    definition_.ny = object.value(QStringLiteral("ny")).toInt(4);
    definition_.nz = object.value(QStringLiteral("nz")).toInt(4);
    definition_.source = object.value(QStringLiteral("source_is_geometry")).toBool(false)
        ? MeshSource::GeometryBoundingBox
        : MeshSource::ParametricBox;
    rebuildParametricTopology();
    ++settingsRevision_;
    syncFromGeometry();
    emit changed();
}

} // namespace d26
