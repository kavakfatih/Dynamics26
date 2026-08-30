#include "ViewportInputRouter.h"

#include <QAbstractSpinBox>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace d26 {

#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
Q_LOGGING_CATEGORY(viewportInputLog, "dynamics26.viewport.input", QtWarningMsg)
#endif

namespace {

bool nameSuggestsTrackpad(const QString &name)
{
    return name.contains(QStringLiteral("trackpad"), Qt::CaseInsensitive)
        || name.contains(QStringLiteral("touchpad"), Qt::CaseInsensitive);
}

double boundedPixelDelta(const int value)
{
    return static_cast<double>(std::clamp(value, -80, 80));
}

#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE

const char *actionName(const NavigationActionType type)
{
    switch (type) {
    case NavigationActionType::None: return "None";
    case NavigationActionType::Orbit: return "Orbit";
    case NavigationActionType::Pan: return "Pan";
    case NavigationActionType::Zoom: return "Zoom";
    case NavigationActionType::Fit: return "Fit";
    case NavigationActionType::SetStandardView: return "SetStandardView";
    case NavigationActionType::SetRotationCenter: return "SetRotationCenter";
    }
    return "Unknown";
}

const char *sourceName(const NavigationInputSource source)
{
    switch (source) {
    case NavigationInputSource::MouseDrag: return "MouseDrag";
    case NavigationInputSource::PixelScroll: return "PixelScroll";
    case NavigationInputSource::AngleWheel: return "AngleWheel";
    case NavigationInputSource::NativePan: return "NativePan";
    case NavigationInputSource::NativeZoom: return "NativeZoom";
    case NavigationInputSource::Keyboard: return "Keyboard";
    }
    return "Unknown";
}

const char *navigationPhaseName(const NavigationPhase phase)
{
    switch (phase) {
    case NavigationPhase::Begin: return "Begin";
    case NavigationPhase::Update: return "Update";
    case NavigationPhase::End: return "End";
    }
    return "Unknown";
}

const char *scrollPhaseName(const Qt::ScrollPhase phase)
{
    switch (phase) {
    case Qt::NoScrollPhase: return "NoScrollPhase";
    case Qt::ScrollBegin: return "ScrollBegin";
    case Qt::ScrollUpdate: return "ScrollUpdate";
    case Qt::ScrollEnd: return "ScrollEnd";
    case Qt::ScrollMomentum: return "ScrollMomentum";
    }
    return "Unknown";
}

const char *deviceTypeName(const QInputDevice::DeviceType type)
{
    switch (type) {
    case QInputDevice::DeviceType::Unknown: return "Unknown";
    case QInputDevice::DeviceType::Mouse: return "Mouse";
    case QInputDevice::DeviceType::TouchScreen: return "TouchScreen";
    case QInputDevice::DeviceType::TouchPad: return "TouchPad";
    case QInputDevice::DeviceType::Puck: return "Puck";
    case QInputDevice::DeviceType::Stylus: return "Stylus";
    case QInputDevice::DeviceType::Airbrush: return "Airbrush";
    case QInputDevice::DeviceType::Keyboard: return "Keyboard";
    case QInputDevice::DeviceType::AllDevices: return "AllDevices";
    }
    return "Unknown";
}

const char *gestureName(const Qt::NativeGestureType type)
{
    switch (type) {
    case Qt::BeginNativeGesture: return "BeginNativeGesture";
    case Qt::EndNativeGesture: return "EndNativeGesture";
    case Qt::PanNativeGesture: return "PanNativeGesture";
    case Qt::ZoomNativeGesture: return "ZoomNativeGesture";
    case Qt::SmartZoomNativeGesture: return "SmartZoomNativeGesture";
    case Qt::RotateNativeGesture: return "RotateNativeGesture";
    case Qt::SwipeNativeGesture: return "SwipeNativeGesture";
    }
    return "UnknownNativeGesture";
}

void traceWheel(const WheelInputDescriptor &input, const NavigationAction &action,
                const char *resultOverride = nullptr)
{
    qCDebug(viewportInputLog).noquote().nospace()
        << "Wheel Event pixelDelta=" << input.pixelDelta
        << " angleDelta=" << input.angleDelta
        << " phase=" << scrollPhaseName(input.phase)
        << " inverted=" << input.inverted
        << " deviceType=" << deviceTypeName(input.deviceType)
        << " deviceName=\"" << input.deviceName << "\""
        << " deviceCapabilities=0x"
        << QString::number(static_cast<qulonglong>(input.capabilities.toInt()), 16)
        << " InputSource=" << sourceName(action.source)
        << " Action=" << (resultOverride != nullptr ? resultOverride : actionName(action.type))
        << " NavigationPhase=" << navigationPhaseName(action.phase);
}

void traceNativeGesture(const QNativeGestureEvent *event, const NavigationAction *action,
                        const char *resultOverride = nullptr)
{
    qCDebug(viewportInputLog).noquote().nospace()
        << "Native Gesture gestureType=" << gestureName(event->gestureType())
        << " value=" << event->value()
        << " position=" << event->position()
        << " InputSource=" << (action != nullptr ? sourceName(action->source) : "NativeSequence")
        << " Action=" << (resultOverride != nullptr
                                ? resultOverride
                                : (action != nullptr ? actionName(action->type) : "None"))
        << " NavigationPhase="
        << (action != nullptr ? navigationPhaseName(action->phase) : "Update");
}

#endif

} // namespace

ViewportInputRouter::ViewportInputRouter(ActionSink sink, QObject *parent)
    : QObject(parent), sink_(std::move(sink))
{
    clock_.start();
}

NavigationPhase ViewportInputRouter::phaseForScroll(const Qt::ScrollPhase phase)
{
    if (phase == Qt::ScrollBegin) {
        return NavigationPhase::Begin;
    }
    if (phase == Qt::ScrollEnd) {
        return NavigationPhase::End;
    }
    return NavigationPhase::Update;
}

NavigationAction ViewportInputRouter::classifyWheel(const WheelInputDescriptor &input)
{
    NavigationAction action;
    action.phase = phaseForScroll(input.phase);

    const bool hasPixels = !input.pixelDelta.isNull();
    const bool hasAngles = !input.angleDelta.isNull();
    const bool trackpadIdentity = input.deviceType == QInputDevice::DeviceType::TouchPad
        || nameSuggestsTrackpad(input.deviceName);
    const bool explicitMouse = input.deviceType == QInputDevice::DeviceType::Mouse && !trackpadIdentity;
    const bool phasedHighResolutionScroll = hasPixels && input.phase != Qt::NoScrollPhase;
    const bool explicitPixelMouse = explicitMouse
        && input.capabilities.testFlag(QInputDevice::Capability::PixelScroll);

    // pixelDelta is intentionally only one signal. A named/typed trackpad is
    // authoritative; otherwise a phased pixel stream is the macOS high-resolution
    // pan candidate. An explicit mouse with angular ticks remains a zoom device.
    // This is behavioral classification, not a claim about the physical
    // hardware. pixelDelta alone never proves that a device is a trackpad.
    const bool panCandidate = trackpadIdentity
        || (hasPixels && !explicitMouse)
        || (phasedHighResolutionScroll && !hasAngles && !explicitPixelMouse);

    if (panCandidate && (hasPixels || hasAngles)) {
        const QPoint sourceDelta = hasPixels ? input.pixelDelta : input.angleDelta / 8;
        const double momentumScale = input.phase == Qt::ScrollMomentum ? 0.35 : 1.0;
        action.type = NavigationActionType::Pan;
        action.source = NavigationInputSource::PixelScroll;
        action.delta = QPointF(boundedPixelDelta(sourceDelta.x()) * momentumScale,
                               boundedPixelDelta(sourceDelta.y()) * momentumScale);
        return action;
    }

    if (hasAngles || hasPixels) {
        const double raw = hasAngles ? static_cast<double>(input.angleDelta.y()) / 120.0
                                     : static_cast<double>(input.pixelDelta.y()) / 120.0;
        action.type = NavigationActionType::Zoom;
        action.source = hasAngles ? NavigationInputSource::AngleWheel : NavigationInputSource::PixelScroll;
        action.zoomDelta = std::clamp(raw, -8.0, 8.0);
        return action;
    }

    // Begin/End events can legally carry no delta. Consume their lifecycle
    // without generating a camera mutation.
    action.type = NavigationActionType::None;
    action.source = hasPixels ? NavigationInputSource::PixelScroll : NavigationInputSource::AngleWheel;
    return action;
}

bool ViewportInputRouter::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::KeyPress && isEditor(watched)) {
        return QObject::eventFilter(watched, event);
    }
    if (routeEvent(event)) {
        event->accept();
        return true;
    }
    return QObject::eventFilter(watched, event);
}

bool ViewportInputRouter::routeEvent(QEvent *event)
{
    if (event == nullptr) {
        return false;
    }
    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
        return routeMouse(static_cast<QMouseEvent *>(event));
    case QEvent::Wheel:
        return routeWheel(static_cast<QWheelEvent *>(event));
    case QEvent::NativeGesture:
        return routeNativeGesture(static_cast<QNativeGestureEvent *>(event));
    case QEvent::KeyPress: {
        auto *key = static_cast<QKeyEvent *>(event);
        const Qt::KeyboardModifiers modifiers = key->modifiers();
        NavigationAction action;
        action.source = NavigationInputSource::Keyboard;
        if (key->key() == Qt::Key_F && modifiers == Qt::NoModifier) {
            action.type = NavigationActionType::Fit;
        } else if (key->key() == Qt::Key_0 && modifiers == Qt::NoModifier) {
            action.type = NavigationActionType::SetStandardView;
            action.standardView = StandardView::Isometric;
        } else if (key->key() >= Qt::Key_1 && key->key() <= Qt::Key_3
                   && (modifiers == Qt::NoModifier || modifiers == Qt::ShiftModifier)) {
            action.type = NavigationActionType::SetStandardView;
            const bool shifted = modifiers == Qt::ShiftModifier;
            if (key->key() == Qt::Key_1) {
                action.standardView = shifted ? StandardView::Back : StandardView::Front;
            } else if (key->key() == Qt::Key_2) {
                action.standardView = shifted ? StandardView::Bottom : StandardView::Top;
            } else {
                action.standardView = shifted ? StandardView::Left : StandardView::Right;
            }
        } else {
            return false;
        }
        dispatch(action);
        event->accept();
        return true;
    }
    default:
        return false;
    }
}

bool ViewportInputRouter::routeMouse(QMouseEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        const bool optionLeft = event->button() == Qt::LeftButton
            && event->modifiers().testFlag(Qt::AltModifier);
        if (event->button() == Qt::MiddleButton || optionLeft) {
            dragMode_ = event->button() == Qt::MiddleButton
                    && event->modifiers().testFlag(Qt::ShiftModifier)
                ? DragMode::Pan
                : DragMode::Orbit;
            dragButton_ = event->button();
            lastPointerPosition_ = event->position();
            NavigationAction action;
            action.type = dragMode_ == DragMode::Pan ? NavigationActionType::Pan : NavigationActionType::Orbit;
            action.phase = NavigationPhase::Begin;
            action.source = NavigationInputSource::MouseDrag;
            dispatch(action);
            event->accept();
            return true;
        }
        return false; // plain left is selection; right is context-menu territory
    }

    if (event->type() == QEvent::MouseMove && dragMode_ != DragMode::None) {
        NavigationAction action;
        action.type = dragMode_ == DragMode::Pan ? NavigationActionType::Pan : NavigationActionType::Orbit;
        action.phase = NavigationPhase::Update;
        action.source = NavigationInputSource::MouseDrag;
        action.delta = event->position() - lastPointerPosition_;
        lastPointerPosition_ = event->position();
        dispatch(action);
        event->accept();
        return true;
    }

    if (event->type() == QEvent::MouseButtonRelease && dragMode_ != DragMode::None) {
        const bool matchingRelease = event->button() == dragButton_;
        if (matchingRelease) {
            NavigationAction action;
            action.type = dragMode_ == DragMode::Pan ? NavigationActionType::Pan : NavigationActionType::Orbit;
            action.phase = NavigationPhase::End;
            action.source = NavigationInputSource::MouseDrag;
            dispatch(action);
            dragMode_ = DragMode::None;
            dragButton_ = Qt::NoButton;
            event->accept();
            return true;
        }
    }
    return false;
}

bool ViewportInputRouter::routeWheel(QWheelEvent *event)
{
    WheelInputDescriptor descriptor;
    descriptor.pixelDelta = event->pixelDelta();
    descriptor.angleDelta = event->angleDelta();
    descriptor.phase = event->phase();
    descriptor.inverted = event->inverted();
    if (const QInputDevice *device = event->device()) {
        descriptor.deviceType = device->type();
        descriptor.capabilities = device->capabilities();
        descriptor.deviceName = device->name();
    }
    NavigationAction action = classifyWheel(descriptor);

    // A native gesture stream owns the physical gesture. Swallow its possible
    // compatibility wheel event so Qt and VTK cannot apply it a second time.
    if (nativeSequenceActive_ || clock_.elapsed() - lastNativeEventMs_ < 60) {
#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
        traceWheel(descriptor, action, "SuppressedDuplicate");
#endif
        event->accept();
        return true;
    }

#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
    traceWheel(descriptor, action);
#endif
    if (action.type != NavigationActionType::None) {
        if (event->phase() != Qt::NoScrollPhase && wheelAction_ == NavigationActionType::None
            && action.phase != NavigationPhase::Begin) {
            NavigationAction begin = action;
            begin.phase = NavigationPhase::Begin;
            begin.delta = {};
            begin.zoomDelta = 0.0;
            dispatch(begin);
        }
        if (event->phase() != Qt::NoScrollPhase) {
            wheelAction_ = action.type;
        }
        dispatch(action);
    }
    if (event->phase() == Qt::ScrollEnd) {
        if (action.type == NavigationActionType::None && wheelAction_ != NavigationActionType::None) {
            NavigationAction end;
            end.type = wheelAction_;
            end.phase = NavigationPhase::End;
            end.source = wheelAction_ == NavigationActionType::Pan ? NavigationInputSource::PixelScroll
                                                                   : NavigationInputSource::AngleWheel;
            dispatch(end);
        }
        wheelAction_ = NavigationActionType::None;
    }
    event->accept();
    return true;
}

bool ViewportInputRouter::routeNativeGesture(QNativeGestureEvent *event)
{
    const Qt::NativeGestureType gesture = event->gestureType();
    if (gesture == Qt::BeginNativeGesture) {
        nativeSequenceActive_ = true;
        nativeAction_ = NavigationActionType::None;
        lastNativeEventMs_ = clock_.elapsed();
#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
        traceNativeGesture(event, nullptr, "Begin");
#endif
        event->accept();
        return true;
    }
    if (gesture == Qt::PanNativeGesture || gesture == Qt::ZoomNativeGesture) {
        const NavigationActionType type = gesture == Qt::PanNativeGesture ? NavigationActionType::Pan
                                                                          : NavigationActionType::Zoom;
        if (!nativeSequenceActive_) {
            nativeSequenceActive_ = true;
        }
        if (nativeAction_ != type) {
            NavigationAction begin;
            begin.type = type;
            begin.phase = NavigationPhase::Begin;
            begin.source = type == NavigationActionType::Pan ? NavigationInputSource::NativePan
                                                              : NavigationInputSource::NativeZoom;
            dispatch(begin);
            nativeAction_ = type;
        }
        NavigationAction update;
        update.type = type;
        update.phase = NavigationPhase::Update;
        update.source = type == NavigationActionType::Pan ? NavigationInputSource::NativePan
                                                          : NavigationInputSource::NativeZoom;
        if (type == NavigationActionType::Pan) {
            update.delta = event->delta();
        } else {
            // Qt documents value() as an incremental gesture delta, not a
            // wheel angle or an absolute scale factor.
            update.zoomDelta = std::clamp(static_cast<double>(event->value()) * 10.0, -4.0, 4.0);
        }
#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
        traceNativeGesture(event, &update);
#endif
        dispatch(update);
        lastNativeEventMs_ = clock_.elapsed();
        event->accept();
        return true;
    }
    if (gesture == Qt::EndNativeGesture && nativeSequenceActive_) {
        if (nativeAction_ != NavigationActionType::None) {
            NavigationAction end;
            end.type = nativeAction_;
            end.phase = NavigationPhase::End;
            end.source = nativeAction_ == NavigationActionType::Pan ? NavigationInputSource::NativePan
                                                                     : NavigationInputSource::NativeZoom;
#ifdef DYNAMICS26_VIEWPORT_INPUT_TRACE
            traceNativeGesture(event, &end);
#endif
            dispatch(end);
        }
        nativeSequenceActive_ = false;
        nativeAction_ = NavigationActionType::None;
        lastNativeEventMs_ = clock_.elapsed();
        event->accept();
        return true;
    }
    return false;
}

void ViewportInputRouter::dispatch(const NavigationAction &action) const
{
    if (sink_) {
        sink_(action);
    }
}

bool ViewportInputRouter::isEditor(const QObject *object)
{
    return qobject_cast<const QLineEdit *>(object) != nullptr
        || qobject_cast<const QAbstractSpinBox *>(object) != nullptr
        || qobject_cast<const QTextEdit *>(object) != nullptr
        || qobject_cast<const QPlainTextEdit *>(object) != nullptr;
}

} // namespace d26
