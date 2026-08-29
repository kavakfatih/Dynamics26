#include "GeometryPanel.h"

#include <femcae/geometry/DxfSectionReader.h>

#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QJsonObject>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include <exception>
#include <utility>

using namespace femcae::geometry;

GeometryPanel::GeometryPanel(QWidget *parent) : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *cadGroup = new QGroupBox(tr("CAD Geometry — Solver Mesh'ten Ayrı"), this);
    auto *cadLayout = new QVBoxLayout(cadGroup);
    auto *importStepButton = new QPushButton(tr("STEP / STP İçe Aktar"), cadGroup);
    selectionFilter_ = new QComboBox(cadGroup);
    selectionFilter_->addItems({tr("Tümü"), tr("Body / Solid"), tr("Face / Surface"), tr("Edge / Curve"), tr("Vertex")});
    geometryTree_ = new QTreeWidget(cadGroup);
    geometryTree_->setHeaderLabels({tr("Geometri"), tr("Persistent ID")});
    geometryTree_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    stepStatus_ = new QLabel(cadGroup);
    stepStatus_->setWordWrap(true);
    stepStatus_->setText(OcctStepImporter::available()
        ? tr("OCCT STEP/XDE adapter hazır. CAD tessellation yalnız görüntüleme içindir.")
        : tr("OCCT bu build'de bulunamadı. Portable geometry/section core etkin; STEP import macOS OCCT build'inde açılır."));
    cadLayout->addWidget(importStepButton);
    cadLayout->addWidget(new QLabel(tr("Seçim Filtresi"), cadGroup));
    cadLayout->addWidget(selectionFilter_);
    cadLayout->addWidget(geometryTree_, 1);
    cadLayout->addWidget(stepStatus_);

    auto *sectionGroup = new QGroupBox(tr("Custom Section / DXF"), this);
    auto *sectionLayout = new QVBoxLayout(sectionGroup);
    auto *importDxfButton = new QPushButton(tr("DXF Kesit İçe Aktar"), sectionGroup);
    sectionSummary_ = new QLabel(tr("Henüz custom section yüklenmedi."), sectionGroup);
    sectionSummary_->setWordWrap(true);
    sectionLayout->addWidget(importDxfButton);
    sectionLayout->addWidget(sectionSummary_);

    auto *note = new QLabel(tr("CAD B-Rep, display tessellation ve FEM mesh birbirinin yerine kullanılamaz. "
                               "FEM mesh üretimi ayrı Mesh / Pre-Post katmanında yapılır."), this);
    note->setWordWrap(true);

    layout->addWidget(cadGroup, 2);
    layout->addWidget(sectionGroup);
    layout->addWidget(note);

    connect(importStepButton, &QPushButton::clicked, this, &GeometryPanel::importStep);
    connect(importDxfButton, &QPushButton::clicked, this, &GeometryPanel::importDxfSection);
    connect(selectionFilter_, qOverload<int>(&QComboBox::currentIndexChanged), this, &GeometryPanel::selectionFilterChanged);
}

void GeometryPanel::clearProject()
{
    document_.clear();
    currentStepPath_.clear();
    currentDxfPath_.clear();
    rebuildTree();
    sectionSummary_->setText(tr("Henüz custom section yüklenmedi."));
}

QJsonObject GeometryPanel::projectJson() const
{
    QJsonObject object;
    object["step_path"] = currentStepPath_;
    object["dxf_section_path"] = currentDxfPath_;
    object["cad_revision"] = static_cast<qint64>(document_.revision());
    object["contract"] = "cad_geometry_not_display_tessellation_not_fem_mesh";
    return object;
}

void GeometryPanel::loadProjectJson(const QJsonObject &object)
{
    clearProject();
    const QString stepPath = object.value("step_path").toString();
    const QString dxfPath = object.value("dxf_section_path").toString();
    if (!stepPath.isEmpty()) { (void)importStepPath(stepPath); }
    if (!dxfPath.isEmpty()) { (void)importDxfPath(dxfPath); }
}

void GeometryPanel::setTessellationConsumer(std::function<void(const GeometryTessellation &)> consumer)
{
    tessellationConsumer_ = std::move(consumer);
}

void GeometryPanel::importStep()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("STEP Geometri Aç"), QString(),
        tr("STEP Geometry (*.step *.stp *.STEP *.STP)"));
    if (!path.isEmpty()) { (void)importStepPath(path); }
}

bool GeometryPanel::importStepPath(const QString &path)
{
    const auto result = stepImporter_.importFile(path.toStdString(), document_);
    if (!result.success) {
        stepStatus_->setText(tr("STEP import başarısız: %1").arg(QString::fromStdString(result.message)));
        emit message(stepStatus_->text());
        return false;
    }
    currentStepPath_ = path;
    rebuildTree();
    stepStatus_->setText(tr("STEP/XDE: %1 body, %2 face, %3 edge, %4 vertex. CAD revision=%5")
        .arg(static_cast<qulonglong>(result.bodyCount)).arg(static_cast<qulonglong>(result.faceCount))
        .arg(static_cast<qulonglong>(result.edgeCount)).arg(static_cast<qulonglong>(result.vertexCount))
        .arg(static_cast<qulonglong>(document_.revision())));
    emit message(stepStatus_->text());

    const auto bodies = document_.entitiesOfKind(GeometryEntityKind::Body);
    if (!bodies.empty() && tessellationConsumer_) {
        try {
            auto tess = stepImporter_.tessellate(bodies.front(), 0.15);
            tess.sourceRevision = document_.revision();
            tessellationConsumer_(tess);
        } catch (const std::exception &ex) {
            emit message(tr("CAD display tessellation oluşturulamadı: %1").arg(ex.what()));
        }
    }
    return true;
}

void GeometryPanel::importDxfSection()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("DXF Kesit Aç"), QString(), tr("DXF (*.dxf *.DXF)"));
    if (!path.isEmpty()) { (void)importDxfPath(path); }
}

bool GeometryPanel::importDxfPath(const QString &path)
{
    DxfSectionReader reader;
    const auto result = reader.readFile(path.toStdString());
    if (!result.success) {
        sectionSummary_->setText(tr("DXF import başarısız: %1").arg(QString::fromStdString(result.message)));
        emit message(sectionSummary_->text());
        return false;
    }
    try {
        const auto p = result.profile.properties();
        currentDxfPath_ = path;
        updateSectionSummary(p, path);
        emit message(tr("DXF custom section doğrulandı: %1 contour, %2 entity")
            .arg(static_cast<qulonglong>(result.profile.contours().size())).arg(static_cast<qulonglong>(result.entityCount)));
        return true;
    } catch (const std::exception &ex) {
        sectionSummary_->setText(tr("Section property hesabı başarısız: %1").arg(ex.what()));
        return false;
    }
}

void GeometryPanel::selectionFilterChanged(int)
{
    rebuildTree();
}

void GeometryPanel::rebuildTree()
{
    geometryTree_->clear();
    const int filter = selectionFilter_->currentIndex();
    auto visible = [filter](GeometryEntityKind kind) {
        if (filter == 0) return true;
        if (filter == 1) return kind == GeometryEntityKind::Body || kind == GeometryEntityKind::Assembly;
        if (filter == 2) return kind == GeometryEntityKind::Face;
        if (filter == 3) return kind == GeometryEntityKind::Edge;
        if (filter == 4) return kind == GeometryEntityKind::Vertex;
        return true;
    };

    const auto assemblies = document_.entitiesOfKind(GeometryEntityKind::Assembly);
    for (const auto assemblyId : assemblies) {
        const auto *assembly = document_.find(assemblyId);
        if (!assembly) continue;
        auto *root = new QTreeWidgetItem(geometryTree_, {QString::fromStdString(assembly->name), QString::number(static_cast<qulonglong>(assembly->id))});
        for (const auto bodyId : document_.childrenOf(assemblyId)) {
            const auto *body = document_.find(bodyId);
            if (!body || body->kind != GeometryEntityKind::Body) continue;
            QTreeWidgetItem *bodyItem = root;
            if (visible(GeometryEntityKind::Body)) {
                bodyItem = new QTreeWidgetItem(root, {QString::fromStdString(body->name), QString::number(static_cast<qulonglong>(body->id))});
            }
            for (const auto childId : document_.childrenOf(bodyId)) {
                const auto *child = document_.find(childId);
                if (!child || !visible(child->kind)) continue;
                new QTreeWidgetItem(bodyItem, {kindName(child->kind) + "  " + QString::fromStdString(child->name), QString::number(static_cast<qulonglong>(child->id))});
            }
            bodyItem->setExpanded(true);
        }
        root->setExpanded(true);
    }
    geometryTree_->resizeColumnToContents(0);
}

void GeometryPanel::updateSectionSummary(const SectionProperties &p, const QString &sourceName)
{
    sectionSummary_->setText(tr("%1\nA = %2\nCx = %3   Cy = %4\nIxx = %5\nIyy = %6\nIxy = %7\nJp = %8\nPrincipal I1/I2 = %9 / %10")
        .arg(sourceName)
        .arg(p.area, 0, 'g', 10).arg(p.centroid.x, 0, 'g', 10).arg(p.centroid.y, 0, 'g', 10)
        .arg(p.ixx, 0, 'g', 10).arg(p.iyy, 0, 'g', 10).arg(p.ixy, 0, 'g', 10).arg(p.polarJ, 0, 'g', 10)
        .arg(p.principalI1, 0, 'g', 10).arg(p.principalI2, 0, 'g', 10));
}

QString GeometryPanel::kindName(const GeometryEntityKind kind)
{
    switch (kind) {
    case GeometryEntityKind::Assembly: return tr("Assembly");
    case GeometryEntityKind::Body: return tr("Body");
    case GeometryEntityKind::Face: return tr("Face");
    case GeometryEntityKind::Edge: return tr("Edge");
    case GeometryEntityKind::Vertex: return tr("Vertex");
    case GeometryEntityKind::Surface: return tr("Surface");
    case GeometryEntityKind::Curve: return tr("Curve");
    }
    return tr("Geometry");
}
