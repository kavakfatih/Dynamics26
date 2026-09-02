#pragma once

#include "DetailsPage.h"

#include "../core/ServiceContext.h"
#include "../services/MeshService.h"

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class QSpinBox;

namespace d26 {

// Mesh Details — doğrudan MeshService'e bağlıdır. Gösterilen tüm lifecycle,
// istatistik ve kalite değerleri authoritative MeshService/engineering scope
// servislerinden okunur; widget içinde ikinci mesh state'i tutulmaz.
class MeshDetails final : public DetailsPage
{
    Q_OBJECT
public:
    explicit MeshDetails(const ServiceContext &services, QWidget *parent = nullptr);
    void refresh() override;

private:
    void pushDefinition();
    void pushMeshCommand(const MeshService::Definition &after, const QString &text);
    [[nodiscard]] int staleFemScopeCount() const;

    ServiceContext services_;
    bool updating_{false};

    QLabel *method_{nullptr};
    QLabel *elementType_{nullptr};
    QComboBox *source_{nullptr};
    QDoubleSpinBox *length_{nullptr};
    QDoubleSpinBox *width_{nullptr};
    QDoubleSpinBox *height_{nullptr};
    QSpinBox *nx_{nullptr};
    QSpinBox *ny_{nullptr};
    QSpinBox *nz_{nullptr};

    QLabel *status_{nullptr};
    QLabel *generation_{nullptr};
    QLabel *settingsRevision_{nullptr};
    QLabel *sourceGeometryRevision_{nullptr};
    QLabel *meshedGeometryRevision_{nullptr};
    QLabel *staleScopes_{nullptr};

    QLabel *nodes_{nullptr};
    QLabel *elements_{nullptr};
    QLabel *facets_{nullptr};
    QLabel *dof_{nullptr};
    QLabel *scaledJacobian_{nullptr};
    QLabel *aspectRatio_{nullptr};
    QLabel *inverted_{nullptr};
    QLabel *predicted_{nullptr};
    QLabel *solverLimit_{nullptr};

    QPushButton *generate_{nullptr};
    QPushButton *clearGenerated_{nullptr};
};

} // namespace d26
