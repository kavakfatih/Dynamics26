#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"
#include "../viewport/ViewportWidget.h"

class QComboBox;
class QLabel;

namespace d26 {

// Geometry / Body Details.
//
// Eski GeometryPanel'in tamamı buraya taşınmaz. Bu sayfa kompakt bir CAD
// müfettişidir: tanım, gösterim seçenekleri ve eylemler. Geometri ağacı,
// seçim filtresi ve serbest metin açıklamaları Details panelinden çıkarıldı.
class GeometryDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit GeometryDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

signals:
    void representationChanged(int representation);
    void tessellationQualityChanged(double linearDeflection);

private:
    ServiceContext services_;
    QLabel *source_{nullptr};
    QLabel *lengthUnit_{nullptr};
    QLabel *bodies_{nullptr};
    QLabel *faces_{nullptr};
    QLabel *edges_{nullptr};
    QLabel *status_{nullptr};
    QComboBox *representation_{nullptr};
    QComboBox *tessellation_{nullptr};
    QLabel *revision_{nullptr};
    QLabel *occtStatus_{nullptr};
    QLabel *boxStatus_{nullptr};
};

} // namespace d26
