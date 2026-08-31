#pragma once

// Dynamics26 Alpha.3.3 — application-shell CAD topology selection acceptance helper.
//
// --selection-selftest geliştirici bayrağında gerçek SelectionCoordinator signal
// zinciri üzerinden transient Body/Face/Edge/Vertex selection -> Navigator/
// Inspector senkronunu doğrular. Fiziksel mouse/trackpad kabulünün yerine geçmez.

#include "../core/DocumentCommandManager.h"
#include "../core/ProjectModel.h"
#include "../core/SelectionManager.h"
#include "../core/SelectionPolicy.h"
#include "../services/GeometryService.h"
#include "../shell/DetailsHost.h"
#include "../shell/Dynamics26MainWindow.h"
#include "../shell/ProjectNavigator.h"

#include <QApplication>
#include <QLabel>
#include <QUndoStack>

#include <iostream>
#include <string>

namespace d26 {

inline int runSelectionAcceptanceTest(QApplication &app, Dynamics26MainWindow &window)
{
    int failures = 0;
    int checks = 0;
    const auto check = [&failures, &checks](const bool condition, const std::string &message) {
        ++checks;
        std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
        failures += condition ? 0 : 1;
    };

    SelectionManager *selection = window.findChild<SelectionManager *>();
    const ServiceContext services = window.services();
    ProjectModel *project = services.project;
    ProjectNavigator *navigator = window.navigator();
    DetailsHost *details = window.detailsHost();
    QUndoStack *undo = window.documentCommands()->stack();

    check(selection != nullptr, "SelectionManager is owned by the application composition tree");
    check(project != nullptr && navigator != nullptr && details != nullptr && undo != nullptr,
          "selection acceptance has Project/Navigator/Details/Undo collaborators");
    if (selection == nullptr || project == nullptr || navigator == nullptr || details == nullptr || undo == nullptr) {
        return 1;
    }

    constexpr qint64 geometryBodyId = 910001;
    constexpr qint64 geometryFaceId = 910101;
    constexpr qint64 geometryEdgeId = 910201;
    constexpr qint64 geometryVertexId = 910301;
    const ObjectId restoreObject = navigator->selectedObject();
    const int undoIndexBefore = undo->index();
    const ObjectId projectBody = project->addObject(project->geometryNode(), ObjectType::Body,
                                                    QStringLiteral("Alpha33 Sync Body"), geometryBodyId);
    check(projectBody != InvalidObjectId && static_cast<qint64>(projectBody) != geometryBodyId,
          "Project Body identity stays distinct from CAD GeometryEntityId");

    const quint64 revision = services.geometry->summary().revision;
    const auto topologyItem = [revision](const SelectionKind kind,
                                         const qint64 geometryId,
                                         const qint64 parentId) {
        SelectionItem item;
        item.domain = SelectionDomain::Geometry;
        item.kind = kind;
        item.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryId);
        item.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(parentId);
        item.sourceRevision = revision;
        return item;
    };

    SelectionItem body;
    body.domain = SelectionDomain::Geometry;
    body.kind = SelectionKind::Body;
    body.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryBodyId);
    body.sourceRevision = revision;
    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry));
    check(selection->apply(body, SelectionOperation::Replace),
          "viewport-originated Body selection changes transient selection state");
    app.processEvents();
    check(navigator->selectedObject() == projectBody,
          "Body selection synchronizes Project Navigator current object");
    check(details->currentObject() == projectBody,
          "Body selection synchronizes Inspector project-object context");
    check(undo->index() == undoIndexBefore,
          "transient Body selection does not create a document Undo entry");

    const auto checkChildSelection = [&](const SelectionPolicyPreset preset,
                                         const SelectionKind kind,
                                         const qint64 geometryId,
                                         const QString &kindText) {
        selection->setPolicy(SelectionPolicy::preset(preset));
        const SelectionItem item = topologyItem(kind, geometryId, geometryBodyId);
        check(selection->apply(item, SelectionOperation::Replace),
              QStringLiteral("%1 selection is accepted under its topology policy").arg(kindText).toStdString());
        app.processEvents();
        check(navigator->selectedObject() == projectBody,
              QStringLiteral("%1 selection keeps parent Body as Navigator current object").arg(kindText).toStdString());
        check(details->currentObject() == projectBody,
              QStringLiteral("%1 selection keeps parent Body as Inspector project-object context").arg(kindText).toStdString());
        if (QLabel *summary = details->findChild<QLabel *>(QStringLiteral("Dynamics26SelectionSummary"))) {
            check(!summary->isHidden() && summary->text().contains(kindText),
                  QStringLiteral("Inspector exposes committed %1 selection summary").arg(kindText).toStdString());
        } else {
            check(false, "Inspector selection-summary widget is discoverable for acceptance testing");
        }
        check(undo->index() == undoIndexBefore,
              QStringLiteral("transient %1 selection does not create a document Undo entry").arg(kindText).toStdString());
    };

    checkChildSelection(SelectionPolicyPreset::SurfaceScope, SelectionKind::Face,
                        geometryFaceId, QStringLiteral("Face"));
    checkChildSelection(SelectionPolicyPreset::EdgeScope, SelectionKind::Edge,
                        geometryEdgeId, QStringLiteral("Edge"));
    checkChildSelection(SelectionPolicyPreset::VertexScope, SelectionKind::Vertex,
                        geometryVertexId, QStringLiteral("Vertex"));

    selection->clear();
    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry));
    project->removeObject(projectBody);
    if (restoreObject != InvalidObjectId && project->object(restoreObject) != nullptr) {
        window.selectObject(restoreObject);
    } else {
        window.selectObject(project->geometryNode());
    }
    app.processEvents();

    std::cout << (failures == 0 ? "selection shell acceptance PASS\n"
                                : "selection shell acceptance FAIL\n");
    std::cout << "checks=" << checks << " failures=" << failures << '\n';
    return failures == 0 ? 0 : 1;
}

} // namespace d26
