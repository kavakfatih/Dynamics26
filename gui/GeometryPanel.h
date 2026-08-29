#pragma once

#include <QWidget>
#include <QJsonObject>
#include <QString>

#include <functional>
#include <memory>

#include <femcae/geometry/GeometryDocument.h>
#include <femcae/geometry/OcctStepImporter.h>

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
