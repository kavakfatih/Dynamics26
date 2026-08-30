#pragma once

#include <vtkInteractorStyleTrackballCamera.h>

#include <functional>

namespace d26 {

// The sole VTK interaction owner. Qt navigation is routed through
// ViewportInputRouter, so all default VTK camera bindings are intentionally
// inert. Left click remains available for the current body-level pick bridge.
class Dynamics26InteractorStyle final : public vtkInteractorStyleTrackballCamera
{
public:
    static Dynamics26InteractorStyle *New();
    vtkTypeMacro(Dynamics26InteractorStyle, vtkInteractorStyleTrackballCamera);

    using PickCallback = std::function<void(int, int)>;
    void SetPickCallback(PickCallback callback);

    void OnMouseMove() override;
    void OnLeftButtonDown() override;
    void OnLeftButtonUp() override;
    void OnMiddleButtonDown() override;
    void OnMiddleButtonUp() override;
    void OnRightButtonDown() override;
    void OnRightButtonUp() override;
    void OnMouseWheelForward() override;
    void OnMouseWheelBackward() override;
    void OnChar() override;

protected:
    Dynamics26InteractorStyle() = default;
    ~Dynamics26InteractorStyle() override = default;

private:
    PickCallback pickCallback_;
    int leftPress_[2]{0, 0};
    bool leftPressed_{false};
};

} // namespace d26
