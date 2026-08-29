#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QString>

#include <exception>
#include <functional>
#include <memory>

#include <femcae/geometry/GeometryDocument.h>
#include <femcae/geometry/OcctStepImporter.h>
#include <femcae/geometry/SectionProfile.h>

class QComboBox;
class QLabel;
class QPushButton;
class QTreeWidget;

class GeometryPanel final : public QWidget
{
    Q_OBJECT
public:
    explicit GeometryPanel(QWidget *parent = nullptr);
    void clearProject();
    void setTessellationConsumer(std::function<void(const femcae::geometry::GeometryTessellation &)> consumer);
    [[nodiscard]] QJsonObject projectJson() const;
    void loadProjectJson(const QJsonObject &object);

    // Alpha.1 CAE workbench yeni Details yüzeyini legacy GeometryPanel widget'ını
    // doğrudan göstermeden besler. CAD dokümanı yine bu sınıfta tek veri kaynağıdır;
    // yeni shell yalnız salt-okunur özet alır ve mevcut import slotlarını çağırır.
    [[nodiscard]] QString currentStepPath() const { return currentStepPath_; }
    [[nodiscard]] QString currentDxfPath() const { return currentDxfPath_; }
    [[nodiscard]] int bodyCount() const
    {
        return static_cast<int>(document_.entitiesOfKind(femcae::geometry::GeometryEntityKind::Body).size());
    }
    [[nodiscard]] int faceCount() const
    {
        return static_cast<int>(document_.entitiesOfKind(femcae::geometry::GeometryEntityKind::Face).size());
    }
    [[nodiscard]] bool hasCadGeometry() const { return bodyCount() > 0; }

    // Alpha.1 viewport bağlamı: Results ekranından Geometri'ye dönüldüğünde
    // son CAD gövdesinin display tessellation'ını yeniden gösterebilmek için
    // panel kendi CAD belgesinden güvenli bir preview üretir. CAD B-Rep ile FEM
    // mesh birbirine dönüştürülmez; yalnız mevcut tessellation consumer çağrılır.
    [[nodiscard]] bool showCurrentGeometry()
    {
        const auto bodies = document_.entitiesOfKind(femcae::geometry::GeometryEntityKind::Body);
        if (bodies.empty() || !tessellationConsumer_) {
            return false;
        }
        try {
            auto tessellation = stepImporter_.tessellate(bodies.front(), 0.15);
            tessellation.sourceRevision = document_.revision();
            tessellationConsumer_(tessellation);
            return true;
        } catch (const std::exception &ex) {
            emit message(tr("CAD display tessellation yeniden oluşturulamadı: %1").arg(ex.what()));
            return false;
        }
    }

signals:
    void message(const QString &text);

private slots:
    void importStep();
    void importDxfSection();
    void selectionFilterChanged(int index);

private:
    bool importStepPath(const QString &path);
    bool importDxfPath(const QString &path);
    void rebuildTree();
    void updateSectionSummary(const femcae::geometry::SectionProperties &properties, const QString &sourceName);
    static QString kindName(femcae::geometry::GeometryEntityKind kind);

    femcae::geometry::GeometryDocument document_{"femcae-gui-geometry"};
    femcae::geometry::OcctStepImporter stepImporter_;
    std::function<void(const femcae::geometry::GeometryTessellation &)> tessellationConsumer_;

    QComboBox *selectionFilter_ = nullptr;
    QTreeWidget *geometryTree_ = nullptr;
    QLabel *stepStatus_ = nullptr;
    QLabel *sectionSummary_ = nullptr;
    QString currentStepPath_;
    QString currentDxfPath_;
};
