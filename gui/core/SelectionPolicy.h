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
    EdgeScope,
    VertexScope,
    ContactSide,
    BodyMeshControl,
    FaceMeshControl,
    MeshNodeScope,
    MeshElementScope,
    MeshFacetScope
};

struct SelectionPolicy {
    SelectionDomain domain{SelectionDomain::Geometry};
    QVector<SelectionKind> allowedKinds{SelectionKind::Body, SelectionKind::Face,
                                        SelectionKind::Edge, SelectionKind::Vertex};
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
            policy.allowedKinds = {SelectionKind::Body, SelectionKind::Face,
                                   SelectionKind::Edge, SelectionKind::Vertex};
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
        case SelectionPolicyPreset::EdgeScope:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Edge};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::VertexScope:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Vertex};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::BodyMeshControl:
            policy.domain = SelectionDomain::Geometry;
            policy.allowedKinds = {SelectionKind::Body};
            policy.allowMultiple = true;
            break;
        case SelectionPolicyPreset::MeshNodeScope:
            policy.domain = SelectionDomain::Mesh;
            policy.allowedKinds = {SelectionKind::Node};
            policy.allowMultiple = true;
            policy.visibleOnly = true;
            break;
        case SelectionPolicyPreset::MeshElementScope:
            policy.domain = SelectionDomain::Mesh;
            policy.allowedKinds = {SelectionKind::Element};
            policy.allowMultiple = true;
            policy.visibleOnly = true;
            break;
        case SelectionPolicyPreset::MeshFacetScope:
            policy.domain = SelectionDomain::Mesh;
            policy.allowedKinds = {SelectionKind::Facet};
            policy.allowMultiple = true;
            policy.visibleOnly = true;
            break;
        }
        return policy;
    }
};

} // namespace d26
