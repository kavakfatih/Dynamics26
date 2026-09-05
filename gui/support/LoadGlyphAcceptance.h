#pragma once

#include "BoundarySelectionAuthoringAcceptance.h"
#include "../commands/DomainCommands.h"
#include "../shell/GraphicsWorkspace.h"
#include <algorithm>
#include <cmath>
#ifdef FEMCAE_GUI_HAS_VTK
#include <QVTKOpenGLNativeWidget.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkDataArray.h>
#include <vtkFieldData.h>
#include <vtkMatrix4x4.h>
#include <vtkPolyDataMapper.h>
#include <vtkPolyData.h>
#include <vtkRenderer.h>
#include <vtkRendererCollection.h>
#include <vtkRenderWindow.h>
#endif

namespace d26 {
inline int runLoadGlyphAcceptance(QApplication &app, Dynamics26MainWindow &window)
{
#ifndef FEMCAE_GUI_HAS_VTK
    Q_UNUSED(app) Q_UNUSED(window)
    return 1;
#else
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        std::cout << (ok ? "PASS  " : "FAIL  ") << message << std::endl;
        if (!ok) ++failures;
    };
    const auto flush = [&] { app.processEvents(); app.processEvents(); };
    const auto services = window.services();
    auto *commands = window.documentCommands();
    auto *undo = commands->stack();
    auto *viewport = window.graphics()->viewport();
    auto *widget = viewport->findChild<QVTKOpenGLNativeWidget *>();
    if (!widget) return 1;
    QTemporaryDir temporary;
    const auto baseline = temporary.filePath(QStringLiteral("glyph-baseline.json"));
    if (!temporary.isValid() || !window.saveProjectToPath(baseline)) return 1;
    const auto analysisId = window.firstObjectOfType(ObjectType::Analysis);
    const auto *record = services.analysis->analysis(analysisId);
    if (!record || record->loads.isEmpty()) return 1;
    const auto loads = record->loads;
    const auto loadId = loads.first();
    for (const auto id : loads) window.setObjectSuppressed(id, id != loadId);
    auto *renderer = widget->renderWindow()->GetRenderers()->GetFirstRenderer();
    for (int divisions : {1, 4}) {
        const auto before = services.mesh->definition();
        auto after = before; after.nx = after.ny = divisions; after.nz = 1;
        commands->push(new commands::SetMeshDefinitionCommand(services, before, after, QStringLiteral("Glyph test mesh")));
        check(window.runCommand(QStringLiteral("mesh.generate")) && services.mesh->hasMesh(),
              "glyph acceptance uses actual application mesh generation");
        for (int axis = 0; axis < 3; ++axis) for (double sign : {-1.0, 1.0}) {
            const auto beforeLoad = *services.analysis->load(loadId);
            auto load = beforeLoad;
            load.scopingMethod = BoundaryScopingMethod::GeometrySelection;
            load.scope = BoxFace::ZMax; load.namedSelectionId = InvalidObjectId;
            load.fxN = axis == 0 ? 1000*sign : 0;
            load.fyN = axis == 1 ? 1000*sign : 0;
            load.fzN = axis == 2 ? 1000*sign : 0;
            commands->push(new commands::SetForceCommand(services, loadId, beforeLoad, load));
            window.selectObject(loadId); flush();
            const int beforeView = undo->index();
            int renderedGlyphs = 0;
            bool valid = true;
            auto *actors = renderer->GetActors(); actors->InitTraversal();
            while (auto *actor = actors->GetNextActor()) {
                auto *mapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
                auto *data = mapper ? mapper->GetInput() : nullptr;
                auto *origins = data ? data->GetFieldData()->GetArray("D26LoadGlyphOrigins") : nullptr;
                if (!origins) continue;
                auto *direction = data->GetFieldData()->GetArray("D26LoadGlyphDirection");
                auto *lengths = data->GetFieldData()->GetArray("D26LoadGlyphLengths");
                const auto n = origins->GetNumberOfTuples();
                valid &= n > 0 && direction && lengths && data->GetNumberOfPoints() % n == 0;
                if (!valid) break;
                renderedGlyphs += static_cast<int>(n);
                const auto pointsPerGlyph = data->GetNumberOfPoints()/n;
                valid &= pointsPerGlyph > 0 && direction->GetComponent(0,axis) == sign;
                for (vtkIdType i = 0; i < n; ++i) {
                    double origin[3]; origins->GetTuple(i, origin);
                    bool onFacet = false;
                    for (const auto &facet : services.mesh->mesh().boundaryFacets) {
                        if (facet.sourceGeometryId != services.mesh->geometryIdFor(BoxFace::ZMax)) continue;
                        double centre[3]{};
                        for (const auto id : facet.nodeIds) {
                            const auto *node = services.mesh->mesh().findNode(id);
                            if (!node) continue;
                            centre[0] += node->x.x/4; centre[1] += node->x.y/4; centre[2] += node->x.z/4;
                        }
                        onFacet |= std::hypot(origin[0]-centre[0],origin[1]-centre[1],origin[2]-centre[2]) < 1e-9;
                    }
                    valid &= onFacet;
                    const double length = lengths->GetComponent(i,0);
                    double lo = 1e30, hi = -1e30;
                    for (vtkIdType p = i*pointsPerGlyph; p < (i+1)*pointsPerGlyph; ++p) {
                        double local[4]{0,0,0,1}, world[4]; data->GetPoint(p,local);
                        // Gerçek aktör transform'u da dahil: eski world-origin
                        // rotation bug'ı yalnız seed niyetine bakarak geçemez.
                        actor->GetMatrix()->MultiplyPoint(local,world);
                        const double along = (world[axis]-origin[axis])*sign;
                        lo = std::min(lo,along); hi = std::max(hi,along);
                        double radialSquared = 0;
                        for (int a=0; a<3; ++a) if (a!=axis) radialSquared += std::pow(world[a]-origin[a],2);
                        valid &= std::isfinite(along) && std::sqrt(radialSquared) <= length*0.101 + 1e-8;
                    }
                    valid &= std::abs(lo) < 1e-8 && std::abs(hi-length) < 1e-8;
                }
            }
            check(valid && renderedGlyphs > 0 && renderedGlyphs <= 5
                      && renderedGlyphs == viewport->displayedLoadGlyphCount() && undo->index() == beforeView,
                  "actual signed XYZ arrow vertices stay at scoped top-face seeds with correct length/direction; no display Undo");
        }
    }
    auto load = *services.analysis->load(loadId);
    const auto nonzero = load;
    load.fxN = load.fyN = load.fzN = 0;
    commands->push(new commands::SetForceCommand(services, loadId, nonzero, load)); flush();
    check(viewport->displayedLoadGlyphCount() == 0, "zero product Force produces no fake normal arrow");
    undo->undo(); flush();
    check(viewport->displayedLoadGlyphCount() > 0, "Undo restores actual nonzero force arrows");
    window.setObjectSuppressed(analysisId, true); flush();
    check(viewport->displayedLoadGlyphCount() == 0, "inactive analysis contributes no load glyphs");
    undo->undo(); flush();
    check(viewport->displayedLoadGlyphCount() > 0, "Undo restores inherited load visibility");
    check(window.openProjectFromPath(baseline), "glyph acceptance restores original engineering model");
    flush();
    return failures;
#endif
}
} // namespace d26
