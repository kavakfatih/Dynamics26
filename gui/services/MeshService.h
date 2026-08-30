#pragma once

// FEM mesh servisi.
//
// SimulationMesh'in TEK sahibi burasıdır. Structured HEX8 baseline üreteci
// (StructuredHexMesher) doğrudan kullanılır; display tessellation'dan asla
// mesh türetilmez. CAD gövdesi eksen hizalı bir kutuysa yalnız SINIR KUTUSU
// ölçüsü ve gerçek CAD yüz kimlikleri devralınır — B-Rep üçgenleri değil.

#include "../core/ProjectTypes.h"
#include "GeometryService.h"

#include <QJsonObject>
#include <QtGlobal>
#include <QObject>
#include <QString>
#include <QVector>

#include <femcae/meshing/MeshTypes.h>
#include <femcae/meshing/StructuredHexMesher.h>

namespace d26 {

// Sınır koşullarının kapsamlandığı adlandırılmış kutu yüzü.
enum class BoxFace { XMin, XMax, YMin, YMax, ZMin, ZMax };

QString displayName(BoxFace face);

class MeshService final : public QObject
{
    Q_OBJECT
public:
    struct Definition {
        MeshSource source{MeshSource::ParametricBox};
        double lengthMm{100.0};
        double widthMm{20.0};
        double heightMm{20.0};
        int nx{20};
        int ny{4};
        int nz{4};

        [[nodiscard]] bool operator==(const Definition &other) const
        {
            return source == other.source && nx == other.nx && ny == other.ny && nz == other.nz
                && qFuzzyCompare(lengthMm, other.lengthMm) && qFuzzyCompare(widthMm, other.widthMm)
                && qFuzzyCompare(heightMm, other.heightMm);
        }
        [[nodiscard]] bool operator!=(const Definition &other) const { return !(*this == other); }
    };

    explicit MeshService(GeometryService *geometry, QObject *parent = nullptr);

    [[nodiscard]] const Definition &definition() const noexcept { return definition_; }
    void setDimensions(double lengthMm, double widthMm, double heightMm);
    void setDivisions(int nx, int ny, int nz);
    void setSource(MeshSource source);
    // CAD tarafından sürülüyorsa ölçüler kullanıcı tarafından düzenlenemez.
    [[nodiscard]] bool dimensionsAreDerived() const;

    bool generate();
    // Üretilmiş mesh verisini siler, TANIMI korur (Clear Generated Mesh).
    void clearGenerated();
    // Tanım dahil her şeyi sıfırlar (yeni proje / proje yükleme).
    void reset();

    [[nodiscard]] bool hasMesh() const noexcept { return !mesh_.elements.empty(); }
    [[nodiscard]] int nodeCount() const noexcept { return static_cast<int>(mesh_.nodes.size()); }
    [[nodiscard]] int elementCount() const noexcept { return static_cast<int>(mesh_.elements.size()); }
    [[nodiscard]] int boundaryFacetCount() const noexcept { return static_cast<int>(mesh_.boundaryFacets.size()); }
    [[nodiscard]] int dofCount() const noexcept { return 3 * nodeCount(); }
    // Üretilmeden önce beklenen boyut (Details'ta önizleme ve DOF uyarısı için).
    [[nodiscard]] int predictedNodeCount() const;
    [[nodiscard]] int predictedElementCount() const;
    [[nodiscard]] int predictedDofCount() const { return 3 * predictedNodeCount(); }

    [[nodiscard]] femcae::meshing::MeshQuality quality() const noexcept { return quality_; }
    [[nodiscard]] const femcae::meshing::SimulationMesh &mesh() const noexcept { return mesh_; }
    [[nodiscard]] const femcae::meshing::BoxBoundaryGeometry &boundaryGeometry() const noexcept { return boundary_; }
    [[nodiscard]] femcae::geometry::GeometryEntityId geometryIdFor(BoxFace face) const;
    [[nodiscard]] int facetCountFor(BoxFace face) const;
    [[nodiscard]] int nodeCountFor(BoxFace face) const;

    // Mesh, üretildiği geometri VE ayar revizyonuna göre güncel mi?
    [[nodiscard]] bool isUpToDate() const;
    [[nodiscard]] bool isOutOfDate() const { return hasMesh() && !isUpToDate(); }
    // Bağımlılık motoru için: ayar her değiştiğinde artar.
    [[nodiscard]] quint64 settingsRevision() const noexcept { return settingsRevision_; }
    // Her başarılı üretimde artar; çözüm bu değere bağlanır.
    [[nodiscard]] quint64 generation() const noexcept { return generation_; }

    [[nodiscard]] QJsonObject projectJson() const;
    void loadProjectJson(const QJsonObject &object);

signals:
    void changed();
    void message(const QString &text, d26::Severity severity);

private:
    void syncFromGeometry();

    GeometryService *geometry_;
    Definition definition_;
    femcae::meshing::SimulationMesh mesh_;
    femcae::meshing::MeshQuality quality_{};
    femcae::meshing::BoxBoundaryGeometry boundary_{};
    quint64 meshedGeometryRevision_{0};
    quint64 settingsRevision_{1};
    quint64 generation_{0};
    // Bayatlık İÇERİK karşılaştırmasıyla belirlenir, monoton sayaçla değil:
    // ayarları Undo ile üretim anındaki değerlere döndürmek mesh'i yeniden
    // GÜNCEL yapar (profesyonel CAE davranışı).
    Definition generatedDefinition_{};
    bool hasGeneratedDefinition_{false};
    bool geometryBoxAvailable_{false};
};

} // namespace d26
