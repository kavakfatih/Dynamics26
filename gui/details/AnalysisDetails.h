#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"

class QComboBox;
class QLabel;
class QPushButton;
class QVBoxLayout;

namespace d26 {

// Analysis / Analysis Settings Details.
//
// §11: Normal kullanıcı yalnız NİYET görür (Incompressibility: Automatic).
// Bu niyetin çözüldüğü solver implementasyonu (mixed u-p, HEX8/P0, doğrudan
// çözücü) yalnız "Advanced Solver Settings" altında ve salt-okunur gösterilir.
class AnalysisDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit AnalysisDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

private:
    ServiceContext services_;
    bool updating_{false};

    QLabel *analysisType_{nullptr};
    QComboBox *largeDeflection_{nullptr};
    QComboBox *incompressibility_{nullptr};
    QLabel *solver_{nullptr};
    QLabel *status_{nullptr};
    QPushButton *solve_{nullptr};
    // Preflight sonucu büyük bir modal yerine Details içinde kompakt bir
    // doğrulama bölümünde gösterilir (§25).
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
