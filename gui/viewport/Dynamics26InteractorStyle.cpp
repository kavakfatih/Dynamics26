#include "Dynamics26InteractorStyle.h"

#include <vtkObjectFactory.h>
#include <vtkRenderWindowInteractor.h>

#include <cmath>
#include <utility>

namespace d26 {

vtkStandardNewMacro(Dynamics26InteractorStyle);

void Dynamics26InteractorStyle::SetPickCallback(PickCallback callback)
{
    pickCallback_ = std::move(callback);
}

void Dynamics26InteractorStyle::SetCenterOfRotation(const double center[3]) noexcept
{
    if (center == nullptr || !std::isfinite(center[0]) || !std::isfinite(center[1])
        || !std::isfinite(center[2])) {
        return;
    }
    centerOfRotation_ = {center[0], center[1], center[2]};
}

void Dynamics26InteractorStyle::OnMouseMove()
{
    // Camera motion is owned by ViewportInputRouter/ViewportCameraController.
}

void Dynamics26InteractorStyle::OnLeftButtonDown()
{
    if (this->Interactor == nullptr) {
        return;
    }
    int position[2] = {0, 0};
    this->Interactor->GetEventPosition(position);
    leftPress_[0] = position[0];
    leftPress_[1] = position[1];
    leftPressed_ = true;
}

void Dynamics26InteractorStyle::OnLeftButtonUp()
{
    if (!leftPressed_ || this->Interactor == nullptr) {
        return;
    }
    leftPressed_ = false;
    int position[2] = {0, 0};
    this->Interactor->GetEventPosition(position);
    if (std::abs(position[0] - leftPress_[0]) <= 3 && std::abs(position[1] - leftPress_[1]) <= 3
        && pickCallback_) {
        pickCallback_(position[0], position[1]);
    }
}

void Dynamics26InteractorStyle::OnMiddleButtonDown() {}
void Dynamics26InteractorStyle::OnMiddleButtonUp() {}
void Dynamics26InteractorStyle::OnRightButtonDown() {}
void Dynamics26InteractorStyle::OnRightButtonUp() {}
void Dynamics26InteractorStyle::OnMouseWheelForward() {}
void Dynamics26InteractorStyle::OnMouseWheelBackward() {}
void Dynamics26InteractorStyle::OnChar() {}

} // namespace d26
