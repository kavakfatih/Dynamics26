#pragma once

#include "ViewportNavigation.h"

#include <array>
#include <functional>

class vtkRenderer;

namespace d26 {

// Owns every camera mutation initiated by application navigation. Input
// routing stays independent of VTK implementation details.
class ViewportCameraController final
{
public:
    using RenderCallback = std::function<void()>;

    explicit ViewportCameraController(vtkRenderer *renderer, RenderCallback render = {});

    void orbit(const QPointF &pixelDelta);
    void pan(const QPointF &pixelDelta);
    void zoom(double normalizedDelta);

    [[nodiscard]] bool fit();
    [[nodiscard]] bool zoomToBounds(const std::array<double, 6> &bounds);
    void setStandardView(StandardView view);

    void setRotationCenter(const std::array<double, 3> &worldPoint);
    [[nodiscard]] bool setRotationCenterToBounds(const std::array<double, 6> &bounds);
    [[nodiscard]] bool resetRotationCenter();
    [[nodiscard]] const std::array<double, 3> &rotationCenter() const noexcept { return rotationCenter_; }

    [[nodiscard]] bool visibleBounds(std::array<double, 6> &bounds) const;

private:
    void requestRender();
    void resetClippingRange();

    vtkRenderer *renderer_{nullptr};
    RenderCallback render_;
    std::array<double, 3> rotationCenter_{0.0, 0.0, 0.0};
};

} // namespace d26
