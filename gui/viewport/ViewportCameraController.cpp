#include "ViewportCameraController.h"

#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkRenderer.h>

#include <algorithm>
#include <cmath>
#include <utility>

namespace d26 {
namespace {

constexpr double degreesToRadians = vtkMath::Pi() / 180.0;

bool validBounds(const std::array<double, 6> &bounds)
{
    for (const double value : bounds) {
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return bounds[0] <= bounds[1] && bounds[2] <= bounds[3] && bounds[4] <= bounds[5];
}

std::array<double, 3> boundsCenter(const std::array<double, 6> &bounds)
{
    return {0.5 * (bounds[0] + bounds[1]), 0.5 * (bounds[2] + bounds[3]),
            0.5 * (bounds[4] + bounds[5])};
}

void normalize(double vector[3])
{
    if (vtkMath::Normalize(vector) == 0.0) {
        vector[0] = 1.0;
        vector[1] = 0.0;
        vector[2] = 0.0;
    }
}

} // namespace

StandardViewFrame standardViewFrame(const StandardView view) noexcept
{
    switch (view) {
    case StandardView::Front:
        return {{0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::Back:
        return {{0.0, 1.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::Top:
        return {{0.0, 0.0, 1.0}, {0.0, 1.0, 0.0}};
    case StandardView::Bottom:
        return {{0.0, 0.0, -1.0}, {0.0, -1.0, 0.0}};
    case StandardView::Left:
        return {{-1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::Right:
        return {{1.0, 0.0, 0.0}, {0.0, 0.0, 1.0}};
    case StandardView::Isometric:
        return {{1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
    }
    return {{1.0, -1.0, 1.0}, {0.0, 0.0, 1.0}};
}

ViewportCameraController::ViewportCameraController(vtkRenderer *renderer, RenderCallback render)
    : renderer_(renderer), render_(std::move(render))
{
}

bool ViewportCameraController::visibleBounds(std::array<double, 6> &bounds) const
{
    if (renderer_ == nullptr) {
        return false;
    }
    renderer_->ComputeVisiblePropBounds(bounds.data());
    return validBounds(bounds);
}

bool ViewportCameraController::zoomToBounds(const std::array<double, 6> &bounds)
{
    if (renderer_ == nullptr || !validBounds(bounds)) {
        return false;
    }
    rotationCenter_ = boundsCenter(bounds);
    vtkCamera *camera = renderer_->GetActiveCamera();
    if (camera == nullptr) {
        return false;
    }
    camera->SetFocalPoint(rotationCenter_.data());
    renderer_->ResetCamera(bounds.data());
    resetClippingRange();
    requestRender();
    return true;
}

bool ViewportCameraController::fit()
{
    std::array<double, 6> bounds{};
    return visibleBounds(bounds) && zoomToBounds(bounds);
}

void ViewportCameraController::setStandardView(const StandardView view)
{
    if (renderer_ == nullptr) {
        return;
    }
    vtkCamera *camera = renderer_->GetActiveCamera();
    if (camera == nullptr) {
        return;
    }

    std::array<double, 6> bounds{};
    const bool hasBounds = visibleBounds(bounds);
    if (hasBounds) {
        rotationCenter_ = boundsCenter(bounds);
    }

    const StandardViewFrame frame = standardViewFrame(view);
    double direction[3] = {frame.cameraDirection[0], frame.cameraDirection[1], frame.cameraDirection[2]};
    normalize(direction);
    double distance = camera->GetDistance();
    if (!std::isfinite(distance) || distance < 1.0e-9) {
        distance = 1.0;
    }
    camera->SetFocalPoint(rotationCenter_.data());
    camera->SetPosition(rotationCenter_[0] + distance * direction[0],
                        rotationCenter_[1] + distance * direction[1],
                        rotationCenter_[2] + distance * direction[2]);
    camera->SetViewUp(frame.viewUp.data());
    camera->OrthogonalizeViewUp();

    if (hasBounds) {
        renderer_->ResetCamera(bounds.data());
    }
    resetClippingRange();
    requestRender();
}

void ViewportCameraController::orbit(const QPointF &pixelDelta)
{
    if (renderer_ == nullptr || pixelDelta.isNull()) {
        return;
    }
    vtkCamera *camera = renderer_->GetActiveCamera();
    if (camera == nullptr) {
        return;
    }

    camera->SetFocalPoint(rotationCenter_.data());
    camera->Azimuth(-0.35 * pixelDelta.x());
    camera->Elevation(0.35 * pixelDelta.y());
    camera->OrthogonalizeViewUp();
    resetClippingRange();
    requestRender();
}

void ViewportCameraController::pan(const QPointF &pixelDelta)
{
    if (renderer_ == nullptr || pixelDelta.isNull()) {
        return;
    }
    vtkCamera *camera = renderer_->GetActiveCamera();
    if (camera == nullptr) {
        return;
    }

    double position[3];
    double focalPoint[3];
    double viewUp[3];
    camera->GetPosition(position);
    camera->GetFocalPoint(focalPoint);
    camera->GetViewUp(viewUp);

    double forward[3] = {focalPoint[0] - position[0], focalPoint[1] - position[1],
                         focalPoint[2] - position[2]};
    normalize(forward);
    normalize(viewUp);
    double right[3];
    vtkMath::Cross(forward, viewUp, right);
    normalize(right);

    const int *size = renderer_->GetSize();
    const double height = size != nullptr ? std::max(1, size[1]) : 1;
    double worldPerPixel = 1.0;
    if (camera->GetParallelProjection()) {
        worldPerPixel = 2.0 * camera->GetParallelScale() / height;
    } else {
        const double distance = std::max(camera->GetDistance(), 1.0e-9);
        worldPerPixel = 2.0 * distance * std::tan(0.5 * camera->GetViewAngle() * degreesToRadians) / height;
    }

    const double translation[3] = {
        (-pixelDelta.x() * right[0] + pixelDelta.y() * viewUp[0]) * worldPerPixel,
        (-pixelDelta.x() * right[1] + pixelDelta.y() * viewUp[1]) * worldPerPixel,
        (-pixelDelta.x() * right[2] + pixelDelta.y() * viewUp[2]) * worldPerPixel};
    for (int i = 0; i < 3; ++i) {
        position[i] += translation[i];
        focalPoint[i] += translation[i];
        rotationCenter_[static_cast<std::size_t>(i)] += translation[i];
    }
    camera->SetPosition(position);
    camera->SetFocalPoint(focalPoint);
    resetClippingRange();
    requestRender();
}

void ViewportCameraController::zoom(const double normalizedDelta)
{
    if (renderer_ == nullptr || !std::isfinite(normalizedDelta) || std::abs(normalizedDelta) < 1.0e-12) {
        return;
    }
    vtkCamera *camera = renderer_->GetActiveCamera();
    if (camera == nullptr) {
        return;
    }
    const double limitedDelta = std::clamp(normalizedDelta, -8.0, 8.0);
    const double factor = std::pow(1.15, limitedDelta);
    if (camera->GetParallelProjection()) {
        camera->SetParallelScale(camera->GetParallelScale() / factor);
    } else {
        camera->Dolly(factor);
    }
    resetClippingRange();
    requestRender();
}

void ViewportCameraController::setRotationCenter(const std::array<double, 3> &worldPoint)
{
    if (std::all_of(worldPoint.begin(), worldPoint.end(), [](const double value) { return std::isfinite(value); })) {
        rotationCenter_ = worldPoint;
    }
}

bool ViewportCameraController::setRotationCenterToBounds(const std::array<double, 6> &bounds)
{
    if (!validBounds(bounds)) {
        return false;
    }
    rotationCenter_ = boundsCenter(bounds);
    return true;
}

bool ViewportCameraController::resetRotationCenter()
{
    std::array<double, 6> bounds{};
    if (!visibleBounds(bounds)) {
        return false;
    }
    rotationCenter_ = boundsCenter(bounds);
    return true;
}

void ViewportCameraController::resetClippingRange()
{
    if (renderer_ != nullptr) {
        renderer_->ResetCameraClippingRange();
    }
}

void ViewportCameraController::requestRender()
{
    if (render_) {
        render_();
    }
}

} // namespace d26
