#pragma once

#include "SelectionPolicy.h"

#include <QObject>
#include <QVector>

#include <optional>

namespace d26 {

class SelectionManager final : public QObject
{
    Q_OBJECT
public:
    explicit SelectionManager(QObject *parent = nullptr);

    [[nodiscard]] const SelectionPolicy &policy() const noexcept { return policy_; }
    void setPolicy(const SelectionPolicy &policy);

    [[nodiscard]] const QVector<SelectionItem> &items() const noexcept { return items_; }
    [[nodiscard]] std::optional<SelectionItem> primary() const { return primary_; }
    [[nodiscard]] std::optional<SelectionItem> preselection() const { return preselection_; }

    bool setPreselection(std::optional<SelectionItem> item);
    void clearPreselection();

    // Returns true only when committed selection state changes.
    bool apply(const SelectionItem &item, SelectionOperation operation);
    bool clear();

    // CAD belgesi degistiginde eski revision'a ait transient geometry state
    // sessizce saklanmaz. ProjectObject/Mesh domainleri bu fonksiyondan etkilenmez.
    bool invalidateGeometryRevision(quint64 currentRevision);

signals:
    void selectionChanged();
    void preselectionChanged();
    void policyChanged();

private:
    [[nodiscard]] int indexOfIdentity(const SelectionItem &item) const noexcept;
    [[nodiscard]] bool containsIdentity(const SelectionItem &item) const noexcept;
    void normalizePrimary();

    SelectionPolicy policy_{SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry)};
    QVector<SelectionItem> items_;
    std::optional<SelectionItem> primary_;
    std::optional<SelectionItem> preselection_;
};

} // namespace d26
