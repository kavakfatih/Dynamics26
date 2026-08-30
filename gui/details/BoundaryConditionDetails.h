#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"

class QCheckBox;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;

namespace d26 {

// Fixed Support / Force Details.
//
// Kapsam (scope) gerçek bir geometri seçimidir: içe aktarılan CAD gövdesi
// eksen hizalı bir kutuysa gerçek STEP yüz kimlikleri, aksi halde parametrik
// kutunun provenance yüzleri kullanılır. Çözücüye giden düğüm listesi bu
// kapsamdan AssignmentResolver ile üretilir.
class BoundaryConditionDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit BoundaryConditionDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

signals:
    // Kapsanan yüzün viewport'ta vurgulanması için.
    void scopeHighlightRequested(quint64 geometryId);

private:
    void push();

    ServiceContext services_;
    bool updating_{false};
    bool isLoad_{false};

    QLineEdit *name_{nullptr};
    QLabel *scopingMethod_{nullptr};
    QComboBox *scope_{nullptr};
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
