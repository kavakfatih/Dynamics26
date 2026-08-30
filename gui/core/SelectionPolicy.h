#pragma once

// SelectionPolicy transient viewport baglamidir; proje dosyasina yazilmaz.
// Aktif komut/Inspector hangi entity seviyelerini kabul ediyorsa picker ve
// SelectionManager ayni policy kontratini kullanir.

#include "SelectionTypes.h"

#include <QVector>

namespace d26 {

enum class SelectionPolicyPreset {
    NeutralGeometry,
    MaterialAssignment,
    SurfaceScope,
    ContactSide,
    BodyMeshControl,
    FaceMeshControl
};

struct SelectionPolicy {
    SelectionDomain domain{SelectionDomain::Geometry};
    QVector<SelectionKind> allowedKinds{SelectionKind::Body, SelectionKind::Face};
    bool allowMultiple{true};
    bool visibleOnly{true};

    [[nodiscard]] bool accepts(const SelectionItem &item) const noexcept
    {
        return item.isValid() && item.domain == domain && allowedKinds.contains(item.kind);
    }

    [[nodiscard]] bool operator==(const SelectionPolicy &other) const noexcept
    {
        return domain == other.domain && allowedKinds == other.allowedKinds
            && allowMultiple == other.allowMultiple && visibleOnly == other.visibleOnly;
    }
    [[nodiscard]] bool operator!=(const SelectionPolicy &other) const noexcept { return !(*this == other); }

    [[nodiscard]] static SelectionPolicy preset(const SelectionPolicyPreset preset)
    {
        SelectionPolicy policy;
        switch (preset) {
        case SelectionPolicyPreset::NeutralGeometry:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Body, SelectionKind::Face};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::MaterialAssignment:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Body};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::SurfaceScope:
        case SelectionPolicyPreset::ContactSide:
        case SelectionPolicyPreset::FaceMeshControl:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Face};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::BodyMeshControl:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Body};
            policy.allowMultiple = true;
            break;
        }
        return policy;
    }
};

} // namespace d26
