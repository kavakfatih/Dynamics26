#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"

#include <array>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QSpinBox;
class MaterialCurveWidget;

namespace d26 {

// Material Details.
//
// Malzeme modeli seçimi hyperelastic ailesini (Neo-Hookean / Mooney-Rivlin /
// Yeoh / Ogden) destekleyecek şekilde kurgulanmıştır. Eğri önizlemesi gerçek
// Fortran çekirdeğindeki doğrulama + izokorik uniaxial hesabını çağırır.
class MaterialDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit MaterialDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

private:
    void pushDefinition();
    void updateVisibility();
    void evaluateCurve();

    ServiceContext services_;
    bool updating_{false};

    QLineEdit *name_{nullptr};
    QLabel *type_{nullptr};
    QComboBox *model_{nullptr};
    QDoubleSpinBox *density_{nullptr};
    QDoubleSpinBox *young_{nullptr};
    QDoubleSpinBox *poisson_{nullptr};
    QDoubleSpinBox *bulk_{nullptr};
    QDoubleSpinBox *c10_{nullptr};
    QDoubleSpinBox *c01_{nullptr};
    QDoubleSpinBox *c20_{nullptr};
    QDoubleSpinBox *c30_{nullptr};
    QSpinBox *ogdenTerms_{nullptr};
    std::array<QDoubleSpinBox *, 3> ogdenMu_{nullptr, nullptr, nullptr};
    std::array<QDoubleSpinBox *, 3> ogdenAlpha_{nullptr, nullptr, nullptr};

    DetailsRow *youngRow_{nullptr};
    DetailsRow *poissonRow_{nullptr};
    DetailsRow *bulkRow_{nullptr};
    DetailsRow *c10Row_{nullptr};
    DetailsRow *c01Row_{nullptr};
    DetailsRow *c20Row_{nullptr};
    DetailsRow *c30Row_{nullptr};
    DetailsRow *ogdenTermsRow_{nullptr};
    std::array<DetailsRow *, 3> ogdenMuRow_{nullptr, nullptr, nullptr};
    std::array<DetailsRow *, 3> ogdenAlphaRow_{nullptr, nullptr, nullptr};

    DetailsSection *elasticSection_{nullptr};
    DetailsSection *hyperelasticSection_{nullptr};
    DetailsSection *testDataSection_{nullptr};
    MaterialCurveWidget *curve_{nullptr};
    QLabel *curveStatus_{nullptr};
    QLabel *solveNote_{nullptr};
    QLabel *assignment_{nullptr};
    QPushButton *assignButton_{nullptr};
};

} // namespace d26
