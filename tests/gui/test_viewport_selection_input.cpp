#include "viewport/ViewportSelectionController.h"

#include <QCoreApplication>

#include <iostream>
#include <string>

namespace {

int failures = 0;

void check(const bool condition, const std::string &message)
{
    std::cout << (condition ? "PASS  " : "FAIL  ") << message << '\n';
    failures += condition ? 0 : 1;
}

} // namespace

int main(int argc, char **argv)
{
    QCoreApplication application(argc, argv);

    using d26::SelectionInputActionType;
    using d26::SelectionOperation;
    using d26::SelectionPointerEventType;
    using d26::SelectionPointerInput;
    using d26::ViewportSelectionController;

    check(ViewportSelectionController::operationForModifiers(Qt::NoModifier)
              == SelectionOperation::Replace,
          "plain click maps to Replace");
    check(ViewportSelectionController::operationForModifiers(Qt::ShiftModifier)
              == SelectionOperation::Add,
          "Shift click maps to Add");
    check(ViewportSelectionController::operationForModifiers(Qt::ControlModifier)
              == SelectionOperation::Toggle,
          "macOS Command/Qt Control click maps to Toggle");
    check(ViewportSelectionController::operationForModifiers(Qt::MetaModifier)
              == SelectionOperation::Replace,
          "macOS physical Control/Qt Meta click does not masquerade as Command Toggle");
    check(ViewportSelectionController::operationForModifiers(Qt::ControlModifier | Qt::ShiftModifier)
              == SelectionOperation::Toggle,
          "Command Toggle has deterministic priority over Shift Add");

    ViewportSelectionController controller;
    SelectionPointerInput press;
    press.type = SelectionPointerEventType::Press;
    press.position = QPointF(10.0, 10.0);
    press.button = Qt::LeftButton;
    press.buttons = Qt::LeftButton;
    press.modifiers = Qt::ControlModifier;
    check(!controller.routePointer(press).has_value() && controller.clickInProgress(),
          "Command-left press starts a selection click candidate");

    SelectionPointerInput release;
    release.type = SelectionPointerEventType::Release;
    release.position = QPointF(11.0, 11.0);
    release.button = Qt::LeftButton;
    release.buttons = Qt::NoButton;
    release.modifiers = Qt::NoModifier;
    const auto commandCommit = controller.routePointer(release);
    check(commandCommit.has_value()
              && commandCommit->type == SelectionInputActionType::Commit
              && commandCommit->operation == SelectionOperation::Toggle,
          "Command modifier is preserved from press through release");

    press.position = QPointF(15.0, 15.0);
    press.modifiers = Qt::MetaModifier;
    (void)controller.routePointer(press);
    release.position = QPointF(16.0, 15.0);
    const auto physicalControlCommit = controller.routePointer(release);
    check(physicalControlCommit.has_value()
              && physicalControlCommit->type == SelectionInputActionType::Commit
              && physicalControlCommit->operation == SelectionOperation::Replace,
          "physical Control click remains an ordinary Replace selection");

    press.position = QPointF(20.0, 20.0);
    press.modifiers = Qt::NoModifier;
    (void)controller.routePointer(press);
    release.position = QPointF(36.0, 31.0);
    const auto windowCommit = controller.routePointer(release);
    check(windowCommit.has_value()
              && windowCommit->type == SelectionInputActionType::WindowCommit
              && windowCommit->anchor == QPointF(20.0, 20.0)
              && windowCommit->position == QPointF(36.0, 31.0)
              && windowCommit->operation == SelectionOperation::Replace,
          "left drag beyond click tolerance emits WindowCommit with both corners");

    press.position = QPointF(25.0, 25.0);
    press.modifiers = Qt::ShiftModifier;
    (void)controller.routePointer(press);
    release.position = QPointF(45.0, 40.0);
    release.modifiers = Qt::NoModifier;
    const auto shiftWindow = controller.routePointer(release);
    check(shiftWindow.has_value()
              && shiftWindow->type == SelectionInputActionType::WindowCommit
              && shiftWindow->operation == SelectionOperation::Add,
          "window selection preserves Shift Add intent from press time");

    press.position = QPointF(50.0, 50.0);
    press.modifiers = Qt::AltModifier;
    check(!controller.routePointer(press).has_value() && !controller.clickInProgress(),
          "Option-left remains reserved for navigation");
    release.position = press.position;
    check(!controller.routePointer(release).has_value(),
          "Option navigation release cannot leak into selection commit");

    SelectionPointerInput rightPress;
    rightPress.type = SelectionPointerEventType::Press;
    rightPress.position = QPointF(40.0, 40.0);
    rightPress.button = Qt::RightButton;
    rightPress.buttons = Qt::RightButton;
    check(!controller.routePointer(rightPress).has_value() && controller.clickInProgress(),
          "secondary press starts a context-click candidate without mutating selection");

    SelectionPointerInput rightRelease;
    rightRelease.type = SelectionPointerEventType::Release;
    rightRelease.position = QPointF(42.0, 41.0);
    rightRelease.button = Qt::RightButton;
    rightRelease.buttons = Qt::NoButton;
    const auto contextClick = controller.routePointer(rightRelease);
    check(contextClick.has_value()
              && contextClick->type == SelectionInputActionType::ContextMenu
              && contextClick->position == rightRelease.position,
          "small secondary click emits ContextMenu intent");

    rightPress.position = QPointF(50.0, 50.0);
    (void)controller.routePointer(rightPress);
    rightRelease.position = QPointF(58.0, 50.0);
    check(!controller.routePointer(rightRelease).has_value() && !controller.clickInProgress(),
          "secondary pointer travel beyond click tolerance does not open a context menu");

    const auto escape = controller.routeKey(Qt::Key_Escape, Qt::NoModifier);
    check(escape.has_value() && escape->type == SelectionInputActionType::Clear,
          "Escape emits clear-selection intent");

    std::cout << (failures == 0 ? "viewport selection input PASS\n"
                                : "viewport selection input FAIL\n");
    return failures == 0 ? 0 : 1;
}
