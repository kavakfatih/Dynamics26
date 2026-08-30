#pragma once

// Malzeme servisi.
//
// Malzemeler modelin gerçek nesneleridir. Bu servis hem malzeme verisinin hem
// de model ağacındaki Material düğümlerinin sahibidir; böylece bir malzemenin
// kimliği (ObjectId) tek yerde üretilir ve Undo/persistence aynı kimliği
// yeniden kurabilir.
//
// Lineer izotropik model Static Structural çözümünü besler; hyperelastic
// modeller Fortran çekirdeğindeki doğrulama/önizleme yolunu kullanır ve bu
// sınır kullanıcıya açıkça bildirilir.

#include "../core/ProjectModel.h"
#include "../core/ProjectTypes.h"

#include <QJsonObject>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>

#include <array>

namespace d26 {

struct MaterialDefinition {
    QString name{QStringLiteral("Structural Steel")};
    MaterialModel model{MaterialModel::LinearElastic};
    double youngGPa{210.0};
    double poisson{0.30};
    double densityKgM3{7850.0};
    // Hyperelastic parametreleri (MPa cinsinden gösterilir, Pa'ya çevrilir).
    double bulkMPa{2000.0};
    double c10MPa{1.0};
    double c01MPa{0.25};
    double c20MPa{0.10};
    double c30MPa{0.01};
    int ogdenTerms{2};
    std::array<double, 3> ogdenMuMPa{1.5, 0.5, 0.1};
    std::array<double, 3> ogdenAlpha{2.0, -2.0, 4.0};

    // Static Structural çözüm yolu yalnız lineer izotropik malzemeyi destekler.
    [[nodiscard]] bool supportsLinearStaticSolve() const { return model == MaterialModel::LinearElastic; }
    [[nodiscard]] QJsonObject toJson() const;
    static MaterialDefinition fromJson(const QJsonObject &object);
};

struct HyperelasticPreview {
    bool ok{false};
    QString message;
    double initialShearModulusMPa{0.0};
    QVector<QPointF> curve;
};

class MaterialService final : public QObject
{
    Q_OBJECT
public:
    explicit MaterialService(ProjectModel *project, QObject *parent = nullptr);

    [[nodiscard]] int count() const { return static_cast<int>(order_.size()); }
    [[nodiscard]] const QVector<ObjectId> &order() const noexcept { return order_; }
    [[nodiscard]] const MaterialDefinition *byId(ObjectId id) const;
    [[nodiscard]] const MaterialDefinition *at(int index) const;
    [[nodiscard]] int rowOf(ObjectId id) const;

    // Undo/persistence için: kimlik ve satır konumu dışarıdan verilebilir.
    ObjectId createMaterial(const MaterialDefinition &definition, int row = -1,
                            ObjectId requestedId = InvalidObjectId);
    bool removeMaterial(ObjectId id);
    void updateMaterial(ObjectId id, const MaterialDefinition &definition);
    void renameMaterial(ObjectId id, const QString &name);

    // Gövdeye atanmış malzeme (Alpha.2: model başına tek atama).
    [[nodiscard]] ObjectId assignedMaterialId() const noexcept { return assigned_; }
    [[nodiscard]] const MaterialDefinition *assigned() const { return byId(assigned_); }
    void setAssignedMaterial(ObjectId id);

    void resetToDefault();
    void clear();

    // Fortran çekirdeğindeki gerçek doğrulama + izokorik uniaxial önizleme.
    [[nodiscard]] HyperelasticPreview preview(ObjectId id) const;

    // Bağımlılık motoru için: malzeme verisi her değiştiğinde artar.
    [[nodiscard]] quint64 revision() const noexcept { return revision_; }

    [[nodiscard]] QJsonObject toJson() const;
    void fromJson(const QJsonObject &object);
    // Eski (V1.0) tek malzemeli şemadan yükleme.
    void fromLegacyJson(const QJsonObject &object);
    [[nodiscard]] QJsonObject toLegacyJson() const;

signals:
    void changed();
    void message(const QString &text, d26::Severity severity);

private:
    [[nodiscard]] QString uniqueName(const QString &base) const;
    void touch();
    void refreshNode(ObjectId id);

    ProjectModel *project_;
    QHash<ObjectId, MaterialDefinition> materials_;
    QVector<ObjectId> order_;
    ObjectId assigned_{InvalidObjectId};
    quint64 revision_{1};
};

} // namespace d26
