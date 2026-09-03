#include "MeshService.h"

#include <femcae/meshing/AssignmentResolver.h>

#include <QCoreApplication>

#include <algorithm>
#include <exception>

using namespace femcae::meshing;
using femcae::geometry::GeometryEntityId;
using femcae::geometry::InvalidGeometryId;

namespace d26 {
namespace {

// Geometri içe aktarılmadığında kullanılan parametrik kutunun sentetik
// provenance kimlikleri. Gerçek CAD içe aktarıldığında bunların yerine
// STEP'ten gelen gerçek yüz kimlikleri geçer.
constexpr GeometryEntityId kSyntheticBody = 100;
constexpr GeometryEntityId kSyntheticXMin = 101;
constexpr GeometryEntityId kSyntheticXMax = 102;
constexpr GeometryEntityId kSyntheticYMin = 103;
constexpr GeometryEntityId kSyntheticYMax = 104;
constexpr GeometryEntityId kSyntheticZMin = 105;
constexpr GeometryEntityId kSyntheticZMax = 106;

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
    syncFromGeometry();
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
    definition_.lengthMm = std::max(1.0e-3, lengthMm);
    definition_.widthMm = std::max(1.0e-3, widthMm);
    definition_.heightMm = std::max(1.0e-3, heightMm);
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
    BoxBoundaryGeometry boundary{kSyntheticBody, kSyntheticXMin, kSyntheticXMax,
                                 kSyntheticYMin, kSyntheticYMax, kSyntheticZMin, kSyntheticZMax};
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
    ++settingsRevision_;
    syncFromGeometry();
    emit changed();
}

} // namespace d26
