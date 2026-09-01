#pragma once

#include "DetailsPage.h"

#include "../core/ProjectTypes.h"
#include "../core/ServiceContext.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QToolButton;

namespace d26 {

// Fixed Support / Force Details.
//
// Consumer iki scoping method taşır:
//   Geometry Selection -> legacy/current tek BoxFace kapsamı
//   Named Selection     -> persistent NamedSelection ObjectId referansı
//
// Named Selection consumer scope entity'lerini kopyalamaz. Solver/preflight
// current persistent scope'u AnalysisService resolver üzerinden okur.
class BoundaryConditionDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit BoundaryConditionDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

signals:
    // Mevcut viewport highlight API tek GeometryEntityId kabul eder. Çoklu Face
    // Named Selection için yanıltıcı tek-yüz highlight üretmek yerine 0 ile
    // temizlenir; gerçek multi-face overlay ayrı viewport capability'sidir.
    void scopeHighlightRequested(quint64 geometryId);

private:
    void push();
    void populateNamedSelections(ObjectId currentId);
    [[nodiscard]] ObjectId selectedNamedSelectionId() const;

    ServiceContext services_;
    bool updating_{false};
    bool isLoad_{false};

    QLineEdit *name_{nullptr};
    QComboBox *scopingMethod_{nullptr};
    QComboBox *scope_{nullptr};
    QComboBox *namedSelection_{nullptr};
    QToolButton *showNamedSelection_{nullptr};
    DetailsRow *geometryScopeRow_{nullptr};
    DetailsRow *namedSelectionRow_{nullptr};
    QLabel *scopeStatistics_{nullptr};

    DetailsSection *supportSection_{nullptr};
    QCheckBox *fixX_{nullptr};
    QCheckBox *fixY_{nullptr};
    QCheckBox *fixZ_{nullptr};
    QLabel *behavior_{nullptr};

    DetailsSection *loadSection_{nullptr};
    QLabel *defineBy_{nullptr};
    QDoubleSpinBox *fx_{nullptr};
    QDoubleSpinBox *fy_{nullptr};
    QDoubleSpinBox *fz_{nullptr};
    QLabel *magnitude_{nullptr};

    QLabel *coordinateSystem_{nullptr};
};

} // namespace d26
