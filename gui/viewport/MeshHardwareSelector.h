#pragma once

// Dynamics26 Alpha.3.5 — visible-only FEM window selection.
//
// vtkHardwareSelector render-window rectangle icindeki GORUNUR point/cell
// primitive'lerini dondurur. VTK primitive ID engineering identity DEGILDIR;
// her sonuc mutlaka MeshSelectionScene provenance tablosundan MeshEntityId'ye
// cevrilir.
//
// Bu helper bilerek SURFACE/visible-only semantigindedir. Interior/pick-through
// secimi ayri bir frustum policy olarak ele alinacaktir.

#include "MeshSelectionScene.h"

#include <QVector>

#include <vtkDataObject.h>
#include <vtkHardwareSelector.h>
#include <vtkIdTypeArray.h>
#include <vtkInformation.h>
#include <vtkNew.h>
#include <vtkProp.h>
#include <vtkRenderer.h>
#include <vtkSelection.h>
#include <vtkSelectionNode.h>
#include <vtkSmartPointer.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>

namespace d26 {

[[nodiscard]] inline QVector<SelectionItem>
selectVisibleMeshArea(vtkRenderer *renderer,
                      vtkProp *target,
                      const MeshSelectionScene &scene,
                      const SelectionKind kind,
                      const std::array<unsigned int, 4> &area)
{
    QVector<SelectionItem> items;
    if (renderer == nullptr || target == nullptr || !scene.complete()
        || (kind != SelectionKind::Node && kind != SelectionKind::Element
            && kind != SelectionKind::Facet)) {
        return items;
    }

    const unsigned int xmin = std::min(area[0], area[2]);
    const unsigned int ymin = std::min(area[1], area[3]);
    const unsigned int xmax = std::max(area[0], area[2]);
    const unsigned int ymax = std::max(area[1], area[3]);

    vtkNew<vtkHardwareSelector> selector;
    selector->SetRenderer(renderer);
    selector->SetArea(xmin, ymin, xmax, ymax);
    selector->SetFieldAssociation(kind == SelectionKind::Node
                                      ? vtkDataObject::FIELD_ASSOCIATION_POINTS
                                      : vtkDataObject::FIELD_ASSOCIATION_CELLS);

    vtkSmartPointer<vtkSelection> selection = selector->Select();
    if (selection == nullptr) {
        return items;
    }

    const auto appendUnique = [&items](const std::optional<SelectionItem> &candidate) {
        if (!candidate.has_value()) {
            return;
        }
        for (const SelectionItem &existing : items) {
            if (existing.sameIdentity(*candidate)) {
                return;
            }
        }
        items.push_back(*candidate);
    };

    for (unsigned int nodeIndex = 0; nodeIndex < selection->GetNumberOfNodes(); ++nodeIndex) {
        vtkSelectionNode *node = selection->GetNode(nodeIndex);
        if (node == nullptr || node->GetProperties() == nullptr
            || node->GetProperties()->Get(vtkSelectionNode::PROP()) != target) {
            continue;
        }

        vtkIdTypeArray *ids = vtkIdTypeArray::SafeDownCast(node->GetSelectionList());
        if (ids == nullptr) {
            continue;
        }

        for (vtkIdType i = 0; i < ids->GetNumberOfValues(); ++i) {
            const vtkIdType displayId = ids->GetValue(i);
            if (displayId < 0) {
                continue;
            }
            const std::size_t provenanceIndex = static_cast<std::size_t>(displayId);
            if (kind == SelectionKind::Node) {
                appendUnique(scene.selectionItemForVisibleNode(provenanceIndex));
            } else {
                appendUnique(scene.selectionItemForBoundaryCell(provenanceIndex, kind));
            }
        }
    }
    return items;
}

} // namespace d26
