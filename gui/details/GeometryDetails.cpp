#include "GeometryDetails.h"

#include "../services/GeometryService.h"
#include "../services/MeshService.h"

#include <QComboBox>
#include <QLabel>
#include <QPushButton>

namespace d26 {

GeometryDetails::GeometryDetails(const ServiceContext &services, QWidget *parent)
    : DetailsPage(parent), services_(services)
{
    auto *definition = addSection(tr("Definition"));
    source_ = definition->addValueRow(tr("Source"));
    lengthUnit_ = definition->addValueRow(tr("Length Unit"));
    bodies_ = definition->addValueRow(tr("Bodies"));
    faces_ = definition->addValueRow(tr("Faces"));
    edges_ = definition->addValueRow(tr("Edges"));
    status_ = definition->addValueRow(tr("Status"));

    auto *display = addSection(tr("Display"));
    representation_ = makeCombo({tr("Shaded + Edges"), tr("Shaded"), tr("Wireframe")});
    display->addRow(tr("Representation"), representation_);
    tessellation_ = makeCombo({tr("Automatic"), tr("Fine"), tr("Coarse")});
    display->addRow(tr("Tessellation"), tessellation_);

    auto *actions = addSection(tr("Actions"));
    auto *importButton = makeActionButton(tr("Import Geometry…"));
    auto *replaceButton = makeActionButton(tr("Replace Geometry…"));
    actions->addFullWidth(importButton);
    actions->addFullWidth(replaceButton);

    auto *advanced = addSection(tr("Advanced"), true, true);
    revision_ = advanced->addValueRow(tr("CAD Revision"));
    occtStatus_ = advanced->addValueRow(tr("CAD Adapter"));
    boxStatus_ = advanced->addValueRow(tr("Meshable Box"));
    advanced->addNote(tr("CAD B-Rep, görüntüleme üçgenlemesi ve FEM mesh birbirinin yerine kullanılmaz. "
                         "Üçgenleme yalnız ekran içindir; solver elemanları Mesh katmanında üretilir."));

    addStretch();

    connect(importButton, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("geometry.import")); });
    connect(replaceButton, &QPushButton::clicked, this, [this] { emit requestCommand(QStringLiteral("geometry.replace")); });
    connect(representation_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        emit representationChanged(index);
    });
    connect(tessellation_, &QComboBox::currentIndexChanged, this, [this](const int index) {
        const double deflection = index == 1 ? 0.05 : (index == 2 ? 0.45 : 0.15);
        emit tessellationQualityChanged(deflection);
    });
}

void GeometryDetails::refresh()
{
    const GeometrySummary summary = services_.geometry->summary();
    if (summary.hasGeometry) {
        source_->setText(summary.sourceFileName);
        lengthUnit_->setText(tr("mm (STEP model birimi)"));
        bodies_->setText(QString::number(summary.bodyCount));
        faces_->setText(QString::number(summary.faceCount));
        edges_->setText(QString::number(summary.edgeCount));
        status_->setText(tr("Up to date"));
    } else {
        // CAD içe aktarılmadıysa model gövdesi parametrik kutudur. Bu gerçek
        // bir tanımdır; "geometri yok" demek yanıltıcı olurdu.
        const MeshService::Definition &definition = services_.mesh->definition();
        source_->setText(tr("Parametric Box  (%1 × %2 × %3 mm)")
                             .arg(definition.lengthMm, 0, 'g', 5)
                             .arg(definition.widthMm, 0, 'g', 5)
                             .arg(definition.heightMm, 0, 'g', 5));
        lengthUnit_->setText(tr("mm"));
        bodies_->setText(QStringLiteral("1"));
        faces_->setText(QStringLiteral("6"));
        edges_->setText(QStringLiteral("12"));
        status_->setText(tr("Tanımlı — parametrik gövde"));
    }
    revision_->setText(QString::number(summary.revision));
    occtStatus_->setText(GeometryService::occtAvailable()
                             ? tr("OCCT / XDE — kullanılabilir")
                             : tr("OCCT bu derlemede yok"));

    const bool derived = services_.mesh->dimensionsAreDerived();
    if (!summary.hasGeometry) {
        boxStatus_->setText(tr("Evet — parametrik kutu doğrudan meshlenir"));
    } else {
        boxStatus_->setText(derived ? tr("Evet — sınır kutusu mesh'e devrediliyor")
                                    : tr("Hayır — structured HEX8 baseline yalnız eksen hizalı kutuyu mesher"));
    }
}

} // namespace d26
