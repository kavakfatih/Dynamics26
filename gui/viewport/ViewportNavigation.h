#pragma once

#include <QPointF>

#include <array>

namespace d26 {

// Dynamics26 global coordinate convention:
//   +X = Right, +Y = Back, +Z = Up.
enum class StandardView {
    Isometric,
    Front,
    Back,
    Top,
    Bottom,
    Left,
    Right
};

enum class NavigationActionType {
    None,
    Orbit,
    Pan,
    Zoom,
    Fit,
    SetStandardView,
    SetRotationCenter
};

enum class NavigationPhase {
    Begin,
    Update,
    End
};

enum class NavigationInputSource {
    MouseDrag,
    PixelScroll,
    AngleWheel,
    NativePan,
    NativeZoom,
    Keyboard
};

struct NavigationAction {
    NavigationActionType type{NavigationActionType::None};
    NavigationPhase phase{NavigationPhase::Update};
    NavigationInputSource source{NavigationInputSource::MouseDrag};
    QPointF delta;
    double zoomDelta{0.0};
    StandardView standardView{StandardView::Isometric};
    std::array<double, 3> worldPoint{0.0, 0.0, 0.0};
};

struct StandardViewFrame {
    // Unit vector from the focal point towards the camera.
    std::array<double, 3> cameraDirection;
    std::array<double, 3> viewUp;
};

[[nodiscard]] StandardViewFrame standardViewFrame(StandardView view) noexcept;

} // namespace d26
