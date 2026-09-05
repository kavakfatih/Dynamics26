#pragma once

#include "DetailsPage.h"
#include "../core/SelectionTypes.h"
#include "../core/ServiceContext.h"
#include "../services/MeshService.h"
#include "../shell/CommandRegistry.h"
#include <QLabel>
#include <QToolButton>

namespace d26 {
// Transient seçim müfettişi; persistent Body/Named Selection sayfasının veya
// document Undo state'inin sahibi değildir. Hover sayfayı değiştirmez.
class SelectionDetails final : public DetailsPage {
public:
    explicit SelectionDetails(const ServiceContext &services, QWidget *parent)
        : DetailsPage(parent), services_(services) { setObjectName(QStringLiteral("Dynamics26SelectionDetails")); }
    void showSelection(const QVector<SelectionItem> &items, CommandRegistry *commands) {
        items_ = items; commands_ = commands; refresh();
    }
    void refresh() override {
        clearSections();
        if (items_.isEmpty()) return;
        const auto &first = items_.front();
        auto *selection = addSection(tr("Selection"));
        const QString kind = first.kind == SelectionKind::Face ? tr("Face")
            : first.kind == SelectionKind::Edge ? tr("Edge") : tr("Vertex");
        selection->addValueRow(tr("Entity Type"), kind);
        selection->addValueRow(tr("Entities"), QString::number(items_.size()))->setObjectName(QStringLiteral("SelectionEntityCount"));
        const auto &document = services_.mesh->selectionGeometryDocument();
        const auto *entity = document.find(first.geometryEntityId);
        const auto *body = document.find(first.parentGeometryId);
        selection->addValueRow(tr("Parent Body"), body ? QString::fromStdString(body->name) : tr("—"));
        selection->addValueRow(tr("Geometry ID"), QString::number(first.geometryEntityId));
        selection->addValueRow(tr("Persistent Key"), entity ? QString::fromStdString(entity->persistentKey) : tr("—"));
        selection->addValueRow(tr("Status"), entity && first.sourceRevision == document.revision() ? tr("Up to Date") : tr("Out of Date"));
        if (items_.size() > 1) selection->addNote(tr("Kimlik alanları ilk entity'yi gösterir; komutlar seçili kapsamın tamamını kullanır."));
        if (first.kind == SelectionKind::Face) {
            double area = 0.0;
            bool available = true;
            std::optional<std::array<double, 3>> normal;
            bool sameNormal = true;
            for (const auto &item : items_) {
                const auto measurement = services_.mesh->selectionFaceMeasurement(item.geometryEntityId);
                if (!measurement || item.sourceRevision != document.revision()) { available = false; break; }
                area += measurement->areaM2;
                if (normal && *normal != measurement->outwardNormal) sameNormal = false;
                normal = measurement->outwardNormal;
            }
            selection->addValueRow(tr("Area"), available ? tr("%1 mm²").arg(area * 1.0e6, 0, 'g', 8) : tr("Unavailable"));
            selection->addValueRow(tr("Outward Normal"), !available || !normal ? tr("Unavailable")
                : !sameNormal ? tr("Multiple directions")
                : QStringLiteral("(%1, %2, %3)").arg((*normal)[0]).arg((*normal)[1]).arg((*normal)[2]));
            if (!available) selection->addNote(tr("Bu CAD yüzü için doğrulanmış ölçüm desteği bulunmuyor."));
        }
        auto *actions = addSection(tr("Actions"));
        for (const char *id : {"selection.createNamed", "selection.createSupport", "selection.createForce"}) {
            if (first.kind != SelectionKind::Face && QString::fromLatin1(id) != QStringLiteral("selection.createNamed")) continue;
            if (!commands_ || !commands_->action(QLatin1String(id))) continue;
            auto *button = new QToolButton(actions);
            button->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
            button->setDefaultAction(commands_->action(QLatin1String(id)));
            actions->addFullWidth(button);
        }
        addStretch();
    }
private:
    ServiceContext services_;
    QVector<SelectionItem> items_;
    CommandRegistry *commands_{nullptr};
};
} // namespace d26
