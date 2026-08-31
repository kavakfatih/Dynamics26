#pragma once

// Dynamics26 Alpha.3.2 — application-shell selection acceptance helper.
//
// Bu helper yeni bir ürün servisi veya test-only public API oluşturmaz. Yalnız
// --selection-selftest geliştirici bayrağında, gerçek SelectionCoordinator
// signal zinciri üzerinden transient geometry selection -> Navigator/Inspector
// senkronunu doğrular. Fiziksel mouse/trackpad kabulünün yerine geçmez.

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

    // Bu project Body yalnız acceptance test boyunca yaşar. CAD kimliği bilinçli
    // olarak Project ObjectId'den farklıdır; iki kimlik uzayını birbirine
    // eşitleyen bir regresyonu testin kendisi de gizlememelidir.
    constexpr qint64 geometryBodyId = 910001;
    constexpr qint64 geometryFaceId = 910101;
    const ObjectId restoreObject = navigator->selectedObject();
    const int undoIndexBefore = undo->index();
    const ObjectId projectBody = project->addObject(project->geometryNode(), ObjectType::Body,
                                                    QStringLiteral("Alpha32 Sync Body"), geometryBodyId);
    check(projectBody != InvalidObjectId && static_cast<qint64>(projectBody) != geometryBodyId,
          "Project Body identity stays distinct from CAD GeometryEntityId");

    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::NeutralGeometry));
    SelectionItem body;
    body.domain = SelectionDomain::Geometry;
    body.kind = SelectionKind::Body;
    body.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryBodyId);
    body.sourceRevision = services.geometry->summary().revision;
    check(selection->apply(body, SelectionOperation::Replace),
          "viewport-originated Body selection changes transient selection state");
    app.processEvents();

    check(navigator->selectedObject() == projectBody,
          "Body selection synchronizes Project Navigator current object");
    check(details->currentObject() == projectBody,
          "Body selection synchronizes Inspector project-object context");
    check(undo->index() == undoIndexBefore,
          "transient Body selection does not create a document Undo entry");

    selection->setPolicy(SelectionPolicy::preset(SelectionPolicyPreset::SurfaceScope));
    SelectionItem face;
    face.domain = SelectionDomain::Geometry;
    face.kind = SelectionKind::Face;
    face.geometryEntityId = static_cast<femcae::geometry::GeometryEntityId>(geometryFaceId);
    face.parentGeometryId = static_cast<femcae::geometry::GeometryEntityId>(geometryBodyId);
    face.sourceRevision = body.sourceRevision;
    check(selection->apply(face, SelectionOperation::Replace),
          "Face selection is accepted under a surface-scope policy");
    app.processEvents();

    check(navigator->selectedObject() == projectBody,
          "Face selection keeps parent Body as Navigator current object");
    check(details->currentObject() == projectBody,
          "Face selection keeps parent Body as Inspector project-object context");
    if (QLabel *summary = details->findChild<QLabel *>(QStringLiteral("Dynamics26SelectionSummary"))) {
        check(!summary->isHidden() && summary->text().contains(QStringLiteral("Face")),
              "Inspector exposes committed Face selection as a secondary selection summary");
    } else {
        check(false, "Inspector selection-summary widget is discoverable for acceptance testing");
    }
    check(undo->index() == undoIndexBefore,
          "transient Face selection does not create a document Undo entry");

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
