#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"

class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
class QVBoxLayout;

namespace d26 {

// Analysis / Analysis Settings Details.
//
// Beta.1 B1.4: Inspector authoritative AnalysisService/ProjectModel state'ini
// görünür ve düzenlenebilir yapar; ikinci bir preflight/lifecycle state tutmaz.
// §11 gereği kullanıcı niyeti (Incompressibility: Automatic) ile solver
// implementasyonu (mixed u-p / HEX8-P0) ayrılır.
class AnalysisDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit AnalysisDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

private:
    ServiceContext services_;
    bool updating_{false};

    QLineEdit *name_{nullptr};
    QLabel *analysisType_{nullptr};
    QComboBox *largeDeflection_{nullptr};
    QComboBox *incompressibility_{nullptr};
    QLabel *solver_{nullptr};

    QLabel *activeSupports_{nullptr};
    QLabel *activeLoads_{nullptr};
    QLabel *meshReadiness_{nullptr};
    QLabel *materialReadiness_{nullptr};

    QLabel *status_{nullptr};
    QLabel *resultAvailability_{nullptr};
    QPushButton *preflight_{nullptr};
    QPushButton *solve_{nullptr};

    // Preflight sonucu büyük bir modal yerine Details içinde kompakt bir
    // doğrulama bölümünde gösterilir (§25). İçerik her refresh'te doğrudan
    // AnalysisService::preflight() raporundan yeniden çizilir.
    DetailsSection *validation_{nullptr};
    QWidget *validationBody_{nullptr};
    QVBoxLayout *validationLayout_{nullptr};

    QLabel *resolvedFormulation_{nullptr};
    QLabel *elementTechnology_{nullptr};
    QLabel *linearSolver_{nullptr};
    QLabel *dofLimit_{nullptr};
    QLabel *newtonMethod_{nullptr};
};

} // namespace d26
