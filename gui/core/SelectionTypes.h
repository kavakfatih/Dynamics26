#pragma once

// Dynamics26 Alpha.3.4 — transient selection ve persistent scope kontratlari.
//
// Project object, CAD topology ve FEM entity kimlikleri ayni uzay DEGILDIR.
// Bu tipler kimlik alanlarini acik tutarak selection state'in ProjectModel,
// GeometryDocument veya SimulationMesh kimliklerini birbirine karistirmasini
// engeller.

#include "ProjectTypes.h"

#include <femcae/geometry/GeometryTypes.h>
#include <femcae/meshing/MeshTypes.h>

#include <QString>
#include <QVector>
#include <QtGlobal>

namespace d26 {

enum class SelectionDomain {
    ProjectObject,
    Geometry,
    Mesh
};

enum class SelectionKind {
    Object,
    Body,
    Face,
    Edge,
    Vertex,
    Node,
    Element,
    Facet
};

enum class SelectionOperation {
    Replace,
    Add,
    Remove,
    Toggle,
    Clear
};

struct SelectionItem {
    SelectionDomain domain{SelectionDomain::Geometry};
    SelectionKind kind{SelectionKind::Body};
    ObjectId projectObjectId{InvalidObjectId};
    femcae::geometry::GeometryEntityId geometryEntityId{femcae::geometry::InvalidGeometryId};
    femcae::geometry::GeometryEntityId parentGeometryId{femcae::geometry::InvalidGeometryId};
    femcae::meshing::MeshEntityId meshEntityId{femcae::meshing::InvalidMeshId};
    quint64 sourceRevision{0};

    [[nodiscard]] bool isValid() const noexcept
    {
        switch (domain) {
        case SelectionDomain::ProjectObject:
            return kind == SelectionKind::Object && projectObjectId != InvalidObjectId;
        case SelectionDomain::Geometry:
            if (geometryEntityId == femcae::geometry::InvalidGeometryId) {
                return false;
            }
            if (kind == SelectionKind::Body) {
                return true;
            }
            if (kind == SelectionKind::Face || kind == SelectionKind::Edge || kind == SelectionKind::Vertex) {
                return parentGeometryId != femcae::geometry::InvalidGeometryId;
            }
            return false;
        case SelectionDomain::Mesh:
            return meshEntityId != femcae::meshing::InvalidMeshId
                && (kind == SelectionKind::Node || kind == SelectionKind::Element
                    || kind == SelectionKind::Facet);
        }
        return false;
    }

    [[nodiscard]] bool sameIdentity(const SelectionItem &other) const noexcept
    {
        if (domain != other.domain || kind != other.kind) {
            return false;
        }
        switch (domain) {
        case SelectionDomain::ProjectObject:
            return projectObjectId == other.projectObjectId;
        case SelectionDomain::Geometry:
            return geometryEntityId == other.geometryEntityId;
        case SelectionDomain::Mesh:
            return meshEntityId == other.meshEntityId;
        }
        return false;
    }

    [[nodiscard]] bool operator==(const SelectionItem &other) const noexcept
    {
        return domain == other.domain && kind == other.kind
            && projectObjectId == other.projectObjectId
            && geometryEntityId == other.geometryEntityId
            && parentGeometryId == other.parentGeometryId
            && meshEntityId == other.meshEntityId
            && sourceRevision == other.sourceRevision;
    }
    [[nodiscard]] bool operator!=(const SelectionItem &other) const noexcept { return !(*this == other); }
};

// Persistent engineering scope icin DATA-ONLY temel kontrat.
// Geometry scope stable CAD identity + persistentKey tasir. Mesh scope ise
// SimulationMesh icindeki MeshEntityId'yi ve ScopeReference sourceRevision
// alaninda mesh generation'i tasir. Mesh yeniden uretildiginde eski FEM scope
// otomatik rebind edilmez; acikca stale kabul edilir.
struct ScopeEntityReference {
    SelectionDomain domain{SelectionDomain::Geometry};
    SelectionKind kind{SelectionKind::Face};
    femcae::geometry::GeometryEntityId geometryEntityId{femcae::geometry::InvalidGeometryId};
    femcae::geometry::GeometryEntityId parentGeometryId{femcae::geometry::InvalidGeometryId};
    femcae::meshing::MeshEntityId meshEntityId{femcae::meshing::InvalidMeshId};
    QString persistentKey;
};

struct ScopeReference {
    // Domain'e gore source guard:
    // - Geometry scope: GeometryDocument revision
    // - Mesh scope: MeshService generation
    // Tek basina kalici kimlik degildir; stale scope'un sessizce kullanilmasini
    // engelleyen lifecycle kontratidir.
    quint64 sourceRevision{0};
    QVector<ScopeEntityReference> entities;
    [[nodiscard]] bool isEmpty() const noexcept { return entities.isEmpty(); }
};

} // namespace d26
