#pragma once

// Details paneli konağı.
//
// Seçilen nesnenin TÜRÜNE göre doğru Details sayfasını gösterir. Eşleme nesne
// türü üzerinden yapılır; panel içeriği hiçbir zaman görünen metne bakılarak
// seçilmez. Eski büyük GeometryPanel / PrePostPanel widget'ları buraya
// gömülmez — her sayfa kendi kompakt müfettişidir.

#include "../core/ProjectTypes.h"
#include "../core/ServiceContext.h"

#include <QFrame>
#include <QHash>

class QLabel;
class QScrollArea;
class QStackedWidget;

namespace d26 {

class DetailsPage;
class GeometryDetails;
class MeshDetails;
class MaterialDetails;
class AnalysisDetails;
class BoundaryConditionDetails;
class ResultDetails;
class ObjectDetails;

class DetailsHost final : public QFrame
{
    Q_OBJECT
public:
    explicit DetailsHost(const ServiceContext &services, QWidget *parent = nullptr);

    void showObject(ObjectId id);
    // Yalnız değerleri tazeler: sayfa değiştirmez, kaydırma konumunu korur.
    void refresh();
    [[nodiscard]] ObjectId currentObject() const noexcept { return current_; }
    [[nodiscard]] GeometryDetails *geometryPage() const noexcept { return geometry_; }
    [[nodiscard]] BoundaryConditionDetails *boundaryConditionPage() const noexcept { return boundary_; }
    [[nodiscard]] ResultDetails *resultPage() const noexcept { return result_; }

signals:
    void modelEdited();
    void commandRequested(const QString &commandId);

private:
    [[nodiscard]] DetailsPage *pageFor(ObjectType type) const;
    void connectPage(DetailsPage *page);

    ServiceContext services_;
    ObjectId current_{InvalidObjectId};

    QLabel *title_{nullptr};
    QLabel *subtitle_{nullptr};
    QStackedWidget *stack_{nullptr};
    QScrollArea *scroll_{nullptr};
    QWidget *emptyState_{nullptr};

    GeometryDetails *geometry_{nullptr};
    MeshDetails *mesh_{nullptr};
    MaterialDetails *material_{nullptr};
    AnalysisDetails *analysis_{nullptr};
    BoundaryConditionDetails *boundary_{nullptr};
    ResultDetails *result_{nullptr};
    ObjectDetails *object_{nullptr};
};

} // namespace d26
