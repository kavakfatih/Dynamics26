#pragma once

// CAD geometri servisi.
//
// CAD B-Rep belgesinin TEK sahibi burasıdır. Eski GeometryPanel widget'ında
// duran import/tessellation mantığı buraya taşındı; böylece mühendislik durumu
// bir widget'ın ömrüne bağlı olmaktan çıktı.
//
// KRİTİK MİMARİ KURAL (ADR-0013):
//   CAD Geometry  !=  Display Tessellation  !=  FEM Mesh
// Bu servis yalnız ilk ikisini üretir. Üçgenler/çizgiler/noktalar asla solver
// entity'si değildir; FEM mesh üretimi MeshService'in işidir.

#include "../core/ProjectTypes.h"

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QVector>

#include <optional>

#include <femcae/geometry/GeometryDocument.h>
#include <femcae/geometry/OcctStepImporter.h>
#include <femcae/geometry/SectionProfile.h>

namespace d26 {

struct GeometrySummary {
    bool hasGeometry{false};
    int bodyCount{0};
    int faceCount{0};
    int edgeCount{0};
    int vertexCount{0};
    QString sourceFileName;
    QString sourcePath;
    quint64 revision{0};
};

struct SectionSummary {
    bool hasSection{false};
    QString sourceFileName;
    int contourCount{0};
    femcae::geometry::SectionProperties properties;
};

class GeometryService final : public QObject
{
    Q_OBJECT
public:
    explicit GeometryService(QObject *parent = nullptr);

    [[nodiscard]] static bool occtAvailable() noexcept;

    bool importStep(const QString &path);
    bool importDxfSection(const QString &path);
    void clear();

    [[nodiscard]] GeometrySummary summary() const;
    [[nodiscard]] SectionSummary sectionSummary() const { return section_; }
    [[nodiscard]] const femcae::geometry::GeometryDocument &document() const noexcept { return document_; }
    [[nodiscard]] QVector<femcae::geometry::GeometryEntityId> bodies() const;
    [[nodiscard]] QString bodyName(femcae::geometry::GeometryEntityId id) const;

    // Yalnız görüntüleme için üçgenleme. Çağıran taraf bunu FEM mesh yerine
    // kullanamaz; ViewportWidget bunu GeometrySurface rolüyle çizer.
    [[nodiscard]] std::optional<femcae::geometry::GeometryTessellation>
        displayTessellation(femcae::geometry::GeometryEntityId bodyId, double linearDeflection = 0.15) const;

    // Alpha.3.2 topology-aware display yolu. display verisi ile her triangle'in
    // gercek CAD Face kimligi birlikte gelir; Face ID display triangle ID'si
    // veya FEM facet ID'si olarak yorumlanamaz.
    [[nodiscard]] std::optional<femcae::geometry::TopologyTessellation>
        displayTopologyTessellation(femcae::geometry::GeometryEntityId bodyId,
                                    double linearDeflection = 0.15) const;

    // Tüm imported Body'leri tek viewport sahnesine hazırlayan all-or-nothing
    // contract. Herhangi bir Body topology provenance üretemezse eksik/geçici
    // bir sahne döndürülmez; Navigator ile viewport birbirinden ayrışmaz.
    [[nodiscard]] QVector<femcae::geometry::TopologyTessellation>
        displayTopologyScene(double linearDeflection = 0.15) const;

    // Alpha.3.3 CAD Edge/Vertex display provenance. Surface triangle edge veya
    // display point index'i topology kimligi DEGILDIR. Service importer'in
    // import-revision bilgisini current GeometryDocument revision'ina hizalar.
    [[nodiscard]] std::optional<femcae::geometry::EdgeDisplayTessellation>
        displayEdgeTessellation(femcae::geometry::GeometryEntityId bodyId,
                                double linearDeflection = 0.15) const;
    [[nodiscard]] std::optional<femcae::geometry::VertexDisplayPoints>
        displayVertexPoints(femcae::geometry::GeometryEntityId bodyId) const;

    // Multi-body CAD sahnesi all-or-nothing kalir. Bir Body Edge/Vertex display
    // provenance uretemiyorsa kismi bir topology sahnesi kullaniciya sunulmaz.
    [[nodiscard]] QVector<femcae::geometry::EdgeDisplayTessellation>
        displayEdgeScene(double linearDeflection = 0.15) const;
    [[nodiscard]] QVector<femcae::geometry::VertexDisplayPoints>
        displayVertexScene() const;

    [[nodiscard]] std::optional<femcae::geometry::GeometryTessellation> firstBodyTessellation() const;

    // Eksen hizalı sınır kutusu tanımı (yalnız OCCT + kutu benzeri gövde için).
    // Mesh servisi bunu structured HEX8 kutusunun ölçüsü olarak devralabilir.
    [[nodiscard]] std::optional<femcae::geometry::StepAxisAlignedBoxDescriptor>
        boxDescriptor(femcae::geometry::GeometryEntityId bodyId) const;

    // Proje kalıcılığı — mevcut şema anahtarları korunur.
    [[nodiscard]] QJsonObject projectJson() const;
    void loadProjectJson(const QJsonObject &object);

signals:
    void changed();
    void message(const QString &text, d26::Severity severity);

private:
    femcae::geometry::GeometryDocument document_{"dynamics26-geometry"};
    femcae::geometry::OcctStepImporter importer_;
    QString stepPath_;
    QString dxfPath_;
    SectionSummary section_;
    int lastEdgeCount_{0};
    int lastVertexCount_{0};
};

} // namespace d26
