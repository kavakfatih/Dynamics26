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

    // Source identity lifecycle guard'lari birbirinden bağımsızdır. CAD belge
    // revision'i yalnız Geometry domainini, mesh generation yalnız Mesh domainini
    // geçersiz kılar; iki kimlik uzayı birbirini temizleyemez.
    bool invalidateGeometryRevision(quint64 currentRevision);
    bool invalidateMeshGeneration(quint64 currentGeneration);

signals:
    void selectionChanged();
    void preselectionChanged();
    void policyChanged();

private:
    [[nodiscard]] int indexOfIdentity(const SelectionItem &item) const noexcept;
    [[nodiscard]] bool containsIdentity(const SelectionItem &item) const noexcept;
    bool invalidateSourceRevision(SelectionDomain domain, quint64 currentRevision);
    void normalizePrimary();

    SelectionPolicy policy_{SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry)};
    QVector<SelectionItem> items_;
    std::optional<SelectionItem> primary_;
    std::optional<SelectionItem> preselection_;
};

} // namespace d26
