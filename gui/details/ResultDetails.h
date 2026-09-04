#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"
#include "../viewport/ViewportWidget.h"

class QLabel;

namespace d26 {

// Total Deformation / Equivalent Stress / Reaction Force Details.
//
// Tüm değerler gerçek çözüm çıktısındandır. Sonuç yokken bu sayfa gösterilmez;
// model ağacına "henüz sonuç yok" gibi sahte düğüm eklenmez (§30).
class ResultDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit ResultDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

    [[nodiscard]] ResultField field() const noexcept { return field_; }

    // Transient probe presentation; document/Undo/persistence state'i degildir.
    void clearProbe();
    void showDisplacementProbe(qint64 nodeId, double uxMm, double uyMm, double uzMm);
    void showEquivalentStressProbe(qint64 elementId, double vonMisesMPa);

private:
    ServiceContext services_;
    ResultField field_{ResultField::TotalDeformation};

    QLabel *type_{nullptr};
    QLabel *scope_{nullptr};
    QLabel *analysis_{nullptr};
    QLabel *maximum_{nullptr};
    QLabel *minimum_{nullptr};
    QLabel *unit_{nullptr};
    QLabel *deformationScale_{nullptr};
    QLabel *legend_{nullptr};
    QLabel *measure_{nullptr};
    QLabel *physicalQuantity_{nullptr};
    QLabel *association_{nullptr};
    QLabel *sourceLocation_{nullptr};
    QLabel *recoveryMethod_{nullptr};
    QLabel *storageUnit_{nullptr};
    QLabel *displayUnit_{nullptr};
    QLabel *configuration_{nullptr};
    QLabel *solveTime_{nullptr};
    QLabel *probeMethod_{nullptr};
    QLabel *probe_{nullptr};
    DetailsSection *reactionSection_{nullptr};
    DetailsSection *contourSection_{nullptr};
    QLabel *rx_{nullptr};
    QLabel *ry_{nullptr};
    QLabel *rz_{nullptr};
};

} // namespace d26
