#pragma once

#include "ViewportNavigation.h"

#include <QElapsedTimer>
#include <QInputDevice>
#include <QObject>

#include <functional>

class QEvent;
class QMouseEvent;
class QNativeGestureEvent;
class QWheelEvent;

namespace d26 {

struct WheelInputDescriptor {
    QPoint pixelDelta;
    QPoint angleDelta;
    Qt::ScrollPhase phase{Qt::NoScrollPhase};
    QInputDevice::DeviceType deviceType{QInputDevice::DeviceType::Unknown};
    QInputDevice::Capabilities capabilities{QInputDevice::Capability::None};
    QString deviceName;
    bool inverted{false};
};

class ViewportInputRouter final : public QObject
{
public:
    using ActionSink = std::function<void(const NavigationAction &)>;

    explicit ViewportInputRouter(ActionSink sink, QObject *parent = nullptr);

    // Public for deterministic input-contract tests; eventFilter delegates here.
    [[nodiscard]] bool routeEvent(QEvent *event);
    [[nodiscard]] static NavigationAction classifyWheel(const WheelInputDescriptor &input);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    enum class DragMode { None, Orbit, Pan };

    [[nodiscard]] bool routeMouse(QMouseEvent *event);
    [[nodiscard]] bool routeWheel(QWheelEvent *event);
    [[nodiscard]] bool routeNativeGesture(QNativeGestureEvent *event);
    void dispatch(const NavigationAction &action) const;
    static NavigationPhase phaseForScroll(Qt::ScrollPhase phase);
    static bool isEditor(const QObject *object);

    ActionSink sink_;
    DragMode dragMode_{DragMode::None};
    Qt::MouseButton dragButton_{Qt::NoButton};
    QPointF lastPointerPosition_;
    QElapsedTimer clock_;
    bool nativeSequenceActive_{false};
    NavigationActionType nativeAction_{NavigationActionType::None};
    NavigationActionType wheelAction_{NavigationActionType::None};
    qint64 lastNativeEventMs_{-1000};
};

} // namespace d26
