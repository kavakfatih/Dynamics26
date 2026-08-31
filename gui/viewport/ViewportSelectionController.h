#pragma once

// Dynamics26 Alpha.3.5 — viewport selection input state machine.
//
// Bu katman kamera hareketi YAPMAZ ve VTK pick YAPMAZ. Qt pointer/key olayindan
// hover / click-commit / window-preview / window-commit / clear /
// secondary-context niyetini uretir. Navigation katmani kamera jestlerini işler;
// bu state machine yalnız selection'a kalan semantigi siniflandirir. Geometry ve
// Mesh bridge'leri ayni state machine'i kullanir; baglam degisiminde yarim gesture
// acikca iptal edilir.

#include "../core/SelectionTypes.h"

#include <QPointF>
#include <Qt>

#include <cmath>
#include <optional>

namespace d26 {

enum class SelectionPointerEventType {
    Press,
    Move,
    Release,
    Leave
};

struct SelectionPointerInput {
    SelectionPointerEventType type{SelectionPointerEventType::Move};
    QPointF position;
    Qt::MouseButton button{Qt::NoButton};
    Qt::MouseButtons buttons{Qt::NoButton};
    Qt::KeyboardModifiers modifiers{Qt::NoModifier};
};

enum class SelectionInputActionType {
    Hover,
    Commit,
    WindowPreview,
    WindowCommit,
    ContextMenu,
    Clear,
    ClearPreselection
};

struct SelectionInputAction {
    SelectionInputActionType type{SelectionInputActionType::Hover};
    QPointF position;
    SelectionOperation operation{SelectionOperation::Replace};
    // WindowPreview/WindowCommit icin press baslangici. Diger action tiplerinde
    // default kalir.
    QPointF anchor;
};

class ViewportSelectionController final
{
public:
    [[nodiscard]] std::optional<SelectionInputAction> routePointer(const SelectionPointerInput &input)
    {
        switch (input.type) {
        case SelectionPointerEventType::Press:
            if (input.button == Qt::LeftButton && !input.modifiers.testFlag(Qt::AltModifier)) {
                leftPressed_ = true;
                leftPressPosition_ = input.position;
                leftPressModifiers_ = input.modifiers;
            } else if (input.button == Qt::RightButton) {
                rightPressed_ = true;
                rightPressPosition_ = input.position;
            }
            return std::nullopt;

        case SelectionPointerEventType::Move:
            if (leftPressed_ && input.buttons.testFlag(Qt::LeftButton)) {
                const QPointF delta = input.position - leftPressPosition_;
                if (std::abs(delta.x()) > clickTolerance_ || std::abs(delta.y()) > clickTolerance_) {
                    return SelectionInputAction{SelectionInputActionType::WindowPreview,
                                                input.position,
                                                operationForModifiers(leftPressModifiers_),
                                                leftPressPosition_};
                }
                return std::nullopt;
            }
            if (leftPressed_ || rightPressed_ || input.buttons != Qt::NoButton) {
                return std::nullopt;
            }
            return SelectionInputAction{SelectionInputActionType::Hover, input.position,
                                        SelectionOperation::Replace};

        case SelectionPointerEventType::Release: {
            if (input.button == Qt::RightButton && rightPressed_) {
                rightPressed_ = false;
                const QPointF delta = input.position - rightPressPosition_;
                if (std::abs(delta.x()) > clickTolerance_ || std::abs(delta.y()) > clickTolerance_) {
                    return std::nullopt;
                }
                return SelectionInputAction{SelectionInputActionType::ContextMenu, input.position,
                                            SelectionOperation::Replace};
            }

            if (input.button != Qt::LeftButton || !leftPressed_) {
                return std::nullopt;
            }
            leftPressed_ = false;
            const QPointF delta = input.position - leftPressPosition_;

            // Press anindaki modifier niyeti kullanilir. Pointer release sirasinda
            // kullanicinin modifier'i birakmasi selection semantigini degistirmez.
            if (leftPressModifiers_.testFlag(Qt::AltModifier)) {
                return std::nullopt;
            }
            const SelectionOperation operation = operationForModifiers(leftPressModifiers_);
            if (std::abs(delta.x()) > clickTolerance_ || std::abs(delta.y()) > clickTolerance_) {
                return SelectionInputAction{SelectionInputActionType::WindowCommit, input.position,
                                            operation, leftPressPosition_};
            }
            return SelectionInputAction{SelectionInputActionType::Commit, input.position, operation};
        }

        case SelectionPointerEventType::Leave:
            cancelPointerGesture();
            return SelectionInputAction{SelectionInputActionType::ClearPreselection, {},
                                        SelectionOperation::Clear};
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<SelectionInputAction> routeKey(const int key,
                                                                const Qt::KeyboardModifiers modifiers)
    {
        if (key == Qt::Key_Escape && modifiers == Qt::NoModifier) {
            cancelPointerGesture();
            return SelectionInputAction{SelectionInputActionType::Clear, {}, SelectionOperation::Clear};
        }
        return std::nullopt;
    }

    void cancelPointerGesture() noexcept
    {
        leftPressed_ = false;
        rightPressed_ = false;
        leftPressModifiers_ = Qt::NoModifier;
    }

    [[nodiscard]] bool clickInProgress() const noexcept { return leftPressed_ || rightPressed_; }

    [[nodiscard]] static SelectionOperation operationForModifiers(const Qt::KeyboardModifiers modifiers) noexcept
    {
        // Qt'nin varsayilan Apple platform eslemesinde fiziksel Command tusu
        // Qt::ControlModifier, fiziksel Control tusu ise Qt::MetaModifier olarak
        // raporlanir. Dynamics26 selection kontrati yalniz Command+Click/Drag'i
        // Toggle kabul eder; fiziksel Control sessizce ayni komut gibi davranmaz.
        if (modifiers.testFlag(Qt::ControlModifier)) {
            return SelectionOperation::Toggle;
        }
        if (modifiers.testFlag(Qt::ShiftModifier)) {
            return SelectionOperation::Add;
        }
        return SelectionOperation::Replace;
    }

private:
    static constexpr double clickTolerance_ = 3.0;
    bool leftPressed_{false};
    bool rightPressed_{false};
    QPointF leftPressPosition_;
    QPointF rightPressPosition_;
    Qt::KeyboardModifiers leftPressModifiers_{Qt::NoModifier};
};

} // namespace d26
