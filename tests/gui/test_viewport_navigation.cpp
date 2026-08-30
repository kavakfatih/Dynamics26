#include "viewport/Dynamics26InteractorStyle.h"
#include "viewport/ViewportCameraController.h"
#include "viewport/ViewportInputRouter.h"

#include <QApplication>
#include <QKeyEvent>
#include <QLineEdit>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QWheelEvent>

#include <vtkActor.h>
#include <vtkAxesActor.h>
#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
#include <vtkCameraOrientationWidget.h>
#endif
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCubeSource.h>
#include <vtkNew.h>
#include <vtkOrientationMarkerWidget.h>
#include <vtkPolyDataMapper.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>

#include <array>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

bool close(const double a, const double b, const double tolerance = 1.0e-8)
{
    return std::abs(a - b) <= tolerance;
}

QMouseEvent mouseEvent(const QEvent::Type type, const QPointF position, const Qt::MouseButton button,
                       const Qt::MouseButtons buttons, const Qt::KeyboardModifiers modifiers)
{
    return QMouseEvent(type, position, position, button, buttons, modifiers);
}

vtkSmartPointer<vtkRenderer> rendererWithModel()
{
    vtkNew<vtkCubeSource> cube;
    cube->SetBounds(2.0, 6.0, -4.0, 2.0, 10.0, 12.0);
    vtkNew<vtkPolyDataMapper> mapper;
    mapper->SetInputConnection(cube->GetOutputPort());
    vtkNew<vtkActor> actor;
    actor->SetMapper(mapper);
    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    renderer->AddActor(actor);
    return renderer;
}

void inputRouterTests()
{
    std::vector<d26::NavigationAction> actions;
    d26::ViewportInputRouter router([&actions](const d26::NavigationAction &action) { actions.push_back(action); });

    auto leftPress = mouseEvent(QEvent::MouseButtonPress, {10.0, 10.0}, Qt::LeftButton, Qt::LeftButton,
                                Qt::NoModifier);
    auto leftMove = mouseEvent(QEvent::MouseMove, {30.0, 25.0}, Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
    auto leftRelease = mouseEvent(QEvent::MouseButtonRelease, {30.0, 25.0}, Qt::LeftButton, Qt::NoButton,
                                  Qt::NoModifier);
    check(!router.routeEvent(&leftPress) && !router.routeEvent(&leftMove) && !router.routeEvent(&leftRelease)
              && actions.empty(),
          "plain left drag is reserved and emits no camera action");

    auto middlePress = mouseEvent(QEvent::MouseButtonPress, {10.0, 10.0}, Qt::MiddleButton, Qt::MiddleButton,
                                  Qt::NoModifier);
    auto middleMove = mouseEvent(QEvent::MouseMove, {18.0, 14.0}, Qt::NoButton, Qt::MiddleButton, Qt::NoModifier);
    auto middleRelease = mouseEvent(QEvent::MouseButtonRelease, {18.0, 14.0}, Qt::MiddleButton, Qt::NoButton,
                                    Qt::NoModifier);
    (void)router.routeEvent(&middlePress);
    (void)router.routeEvent(&middleMove);
    (void)router.routeEvent(&middleRelease);
    check(actions.size() == 3 && actions[0].type == d26::NavigationActionType::Orbit
              && actions[1].phase == d26::NavigationPhase::Update && actions[1].delta == QPointF(8.0, 4.0),
          "middle drag emits the orbit lifecycle");

    actions.clear();
    auto panPress = mouseEvent(QEvent::MouseButtonPress, {4.0, 5.0}, Qt::MiddleButton, Qt::MiddleButton,
                               Qt::ShiftModifier);
    auto panMove = mouseEvent(QEvent::MouseMove, {9.0, 12.0}, Qt::NoButton, Qt::MiddleButton,
                              Qt::ShiftModifier);
    auto panRelease = mouseEvent(QEvent::MouseButtonRelease, {9.0, 12.0}, Qt::MiddleButton, Qt::NoButton,
                                 Qt::ShiftModifier);
    (void)router.routeEvent(&panPress);
    (void)router.routeEvent(&panMove);
    (void)router.routeEvent(&panRelease);
    check(actions.size() == 3 && actions[1].type == d26::NavigationActionType::Pan
              && actions[1].delta == QPointF(5.0, 7.0),
          "Shift+middle drag emits the pan lifecycle");

    d26::WheelInputDescriptor mouseWheel;
    mouseWheel.angleDelta = QPoint(0, 120);
    mouseWheel.deviceType = QInputDevice::DeviceType::Mouse;
    mouseWheel.deviceName = QStringLiteral("USB Mouse");
    const d26::NavigationAction wheelAction = d26::ViewportInputRouter::classifyWheel(mouseWheel);
    check(wheelAction.type == d26::NavigationActionType::Zoom
              && wheelAction.source == d26::NavigationInputSource::AngleWheel
              && close(wheelAction.zoomDelta, 1.0),
          "physical angle wheel classifies as zoom");

    d26::WheelInputDescriptor pixelScrollCandidate;
    pixelScrollCandidate.pixelDelta = QPoint(6, -9);
    pixelScrollCandidate.phase = Qt::ScrollUpdate;
    pixelScrollCandidate.deviceType = QInputDevice::DeviceType::Unknown;
    const d26::NavigationAction pixelCandidateAction =
        d26::ViewportInputRouter::classifyWheel(pixelScrollCandidate);
    check(pixelCandidateAction.type == d26::NavigationActionType::Pan
              && pixelCandidateAction.source == d26::NavigationInputSource::PixelScroll
              && pixelCandidateAction.delta == QPointF(6.0, -9.0),
          "phased pixel-scroll behavior follows the normalized pan path without asserting hardware identity");

    d26::WheelInputDescriptor trackpadScroll;
    trackpadScroll.pixelDelta = QPoint(7, -11);
    trackpadScroll.phase = Qt::ScrollUpdate;
    trackpadScroll.deviceType = QInputDevice::DeviceType::TouchPad;
    trackpadScroll.deviceName = QStringLiteral("Apple Internal Keyboard / Trackpad");
    const d26::NavigationAction scrollAction = d26::ViewportInputRouter::classifyWheel(trackpadScroll);
    check(scrollAction.type == d26::NavigationActionType::Pan
              && scrollAction.source == d26::NavigationInputSource::PixelScroll
              && scrollAction.delta == QPointF(7.0, -11.0),
          "high-resolution trackpad pixel scroll classifies as pan");

    d26::WheelInputDescriptor highResolutionMouse;
    highResolutionMouse.pixelDelta = QPoint(0, 24);
    highResolutionMouse.phase = Qt::ScrollUpdate;
    highResolutionMouse.deviceType = QInputDevice::DeviceType::Mouse;
    highResolutionMouse.capabilities = QInputDevice::Capability::PixelScroll;
    highResolutionMouse.deviceName = QStringLiteral("High Resolution Mouse");
    const d26::NavigationAction pixelMouseAction =
        d26::ViewportInputRouter::classifyWheel(highResolutionMouse);
    check(pixelMouseAction.type == d26::NavigationActionType::Zoom
              && pixelMouseAction.source == d26::NavigationInputSource::PixelScroll,
          "pixelDelta alone is not treated as a trackpad invariant");

    d26::WheelInputDescriptor naturalScroll = trackpadScroll;
    naturalScroll.inverted = true;
    const d26::NavigationAction naturalAction = d26::ViewportInputRouter::classifyWheel(naturalScroll);
    check(naturalAction.delta == scrollAction.delta,
          "natural-scrolling flag is not inverted a second time");

    actions.clear();
    const QPointF point(10.0, 10.0);
    const QPointingDevice *device = QPointingDevice::primaryPointingDevice();
    QNativeGestureEvent begin(Qt::BeginNativeGesture, device, 2, point, point, point, 0.0, {});
    QNativeGestureEvent pinch(Qt::ZoomNativeGesture, device, 2, point, point, point, 0.05, {});
    QWheelEvent compatibilityWheel(point, point, {}, QPoint(0, 120), Qt::NoButton, Qt::NoModifier,
                                   Qt::NoScrollPhase, false);
    (void)router.routeEvent(&begin);
    (void)router.routeEvent(&pinch);
    check(actions.size() == 2 && actions.back().type == d26::NavigationActionType::Zoom
              && actions.back().source == d26::NavigationInputSource::NativeZoom
              && close(actions.back().zoomDelta, 0.5),
          "native pinch classifies as normalized NativeZoom action");
    const std::size_t beforeCompatibilityWheel = actions.size();
    const bool compatibilityConsumed = router.routeEvent(&compatibilityWheel);
    check(compatibilityConsumed && compatibilityWheel.isAccepted()
              && actions.size() == beforeCompatibilityWheel,
          "native gesture suppresses its compatibility wheel duplicate");

    actions.clear();
    QKeyEvent frontKey(QEvent::KeyPress, Qt::Key_1, Qt::NoModifier);
    QKeyEvent backKey(QEvent::KeyPress, Qt::Key_1, Qt::ShiftModifier);
    (void)router.routeEvent(&frontKey);
    (void)router.routeEvent(&backKey);
    check(actions.size() == 2 && actions[0].standardView == d26::StandardView::Front
              && actions[1].standardView == d26::StandardView::Back,
          "viewport numeric shortcuts map to deterministic standard views");

    actions.clear();
    QLineEdit editor;
    editor.installEventFilter(&router);
    QKeyEvent editorFitKey(QEvent::KeyPress, Qt::Key_F, Qt::NoModifier);
    QApplication::sendEvent(&editor, &editorFitKey);
    check(actions.empty(), "viewport shortcuts stay inactive in line editors");
}

void cameraTests()
{
    vtkSmartPointer<vtkRenderer> renderer = rendererWithModel();
    d26::ViewportCameraController controller(renderer);
    check(controller.fit(), "Fit View succeeds for visible model bounds");
    const auto pivot = controller.rotationCenter();
    check(close(pivot[0], 4.0) && close(pivot[1], -1.0) && close(pivot[2], 11.0),
          "Fit View sets rotation center to visible bounds center");

    const std::array<d26::StandardView, 7> views = {
        d26::StandardView::Front, d26::StandardView::Back, d26::StandardView::Top,
        d26::StandardView::Bottom, d26::StandardView::Left, d26::StandardView::Right,
        d26::StandardView::Isometric};
    for (const d26::StandardView view : views) {
        controller.setStandardView(view);
        double direction[3];
        renderer->GetActiveCamera()->GetDirectionOfProjection(direction);
        const d26::StandardViewFrame frame = d26::standardViewFrame(view);
        const double frameNorm = std::sqrt(frame.cameraDirection[0] * frame.cameraDirection[0]
                                           + frame.cameraDirection[1] * frame.cameraDirection[1]
                                           + frame.cameraDirection[2] * frame.cameraDirection[2]);
        const bool correct = close(direction[0], -frame.cameraDirection[0] / frameNorm)
            && close(direction[1], -frame.cameraDirection[1] / frameNorm)
            && close(direction[2], -frame.cameraDirection[2] / frameNorm);
        check(correct, "standard-view camera direction " + std::to_string(static_cast<int>(view)));
    }

    controller.setStandardView(d26::StandardView::Top);
    double topUp[3];
    renderer->GetActiveCamera()->GetViewUp(topUp);
    controller.setStandardView(d26::StandardView::Bottom);
    double bottomUp[3];
    renderer->GetActiveCamera()->GetViewUp(bottomUp);
    check(close(topUp[0], 0.0) && close(topUp[1], 1.0) && close(topUp[2], 0.0)
              && close(bottomUp[0], 0.0) && close(bottomUp[1], -1.0) && close(bottomUp[2], 0.0),
          "Top and Bottom view-up vectors are deterministic");

    const std::array<double, 6> selectionBounds = {-2.0, 0.0, 6.0, 10.0, 1.0, 5.0};
    check(controller.zoomToBounds(selectionBounds)
              && controller.rotationCenter() == std::array<double, 3>{-1.0, 8.0, 3.0},
          "zoom-to-selection bounds API is functional without a SelectionManager");

    vtkNew<vtkRenderer> emptyRenderer;
    d26::ViewportCameraController emptyController(emptyRenderer);
    check(!emptyController.fit(), "Fit View is safe for empty geometry");
}

void interactorAndOverlayTests()
{
    vtkNew<vtkRenderer> renderer;
    vtkNew<vtkRenderWindow> window;
    window->SetOffScreenRendering(1);
    window->SetSize(400, 400);
    window->AddRenderer(renderer);
    vtkNew<vtkRenderWindowInteractor> interactor;
    interactor->SetRenderWindow(window);
    vtkNew<d26::Dynamics26InteractorStyle> style;
    style->SetDefaultRenderer(renderer);
    interactor->SetInteractorStyle(style);

    renderer->GetActiveCamera()->SetPosition(3.0, -4.0, 5.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    double before[3];
    renderer->GetActiveCamera()->GetPosition(before);
    interactor->SetEventInformation(30, 30);
    style->OnLeftButtonDown();
    interactor->SetEventInformation(100, 80);
    style->OnMouseMove();
    style->OnLeftButtonUp();
    style->OnMouseWheelForward();
    double after[3];
    renderer->GetActiveCamera()->GetPosition(after);
    check(close(before[0], after[0]) && close(before[1], after[1]) && close(before[2], after[2]),
          "application interactor style cannot rotate on left drag or zoom on VTK wheel");

    vtkNew<vtkAxesActor> axes;
    vtkNew<vtkOrientationMarkerWidget> marker;
    marker->SetOrientationMarker(axes);
    marker->SetInteractor(interactor);
    marker->SetViewport(0.0, 0.0, 0.13, 0.13);
    marker->SetEnabled(1);
    marker->SetInteractive(0);
    const double *viewport = marker->GetViewport();
    check(marker->GetEnabled() != 0 && marker->GetInteractive() == 0 && close(viewport[2], 0.13)
              && close(viewport[3], 0.13),
          "non-interactive lower-left axis triad is created");

#ifdef FEMCAE_GUI_HAS_CAMERA_ORIENTATION_WIDGET
    vtkNew<vtkCameraOrientationWidget> cube;
    cube->SetParentRenderer(renderer);
    cube->AnimateOff();
    cube->SetEnabled(1);
    check(cube->GetEnabled() != 0 && cube->GetParentRenderer() == renderer,
          "orientation cube is available, enabled, and bound to the model camera");
#else
    check(true, "orientation cube safely deferred for this VTK build");
#endif
}

} // namespace

int main(int argc, char **argv)
{
    QApplication application(argc, argv);
    inputRouterTests();
    cameraTests();
    interactorAndOverlayTests();
    std::cout << (failures == 0 ? "viewport navigation PASS\n" : "viewport navigation FAIL\n");
    return failures == 0 ? 0 : 1;
}
