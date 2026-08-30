#pragma once

// Dynamics26 Alpha.3.2 — viewport selection input state machine.
//
// Bu katman kamera hareketi YAPMAZ ve VTK pick YAPMAZ. Qt pointer/key olayindan
// "hover / commit / clear" niyetini uretir. ViewportInputRouter navigation
// olaylarini once tuketir; bu controller yalniz selection'a kalan olaylarla
// calisir. Boylece macOS modifier kontrati rendering katmanindan ayrilir.

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
    Clear,
    ClearPreselection
};

struct SelectionInputAction {
    SelectionInputActionType type{SelectionInputActionType::Hover};
    QPointF position;
    SelectionOperation operation{SelectionOperation::Replace};
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
                pressPosition_ = input.position;
                pressModifiers_ = input.modifiers;
            }
            return std::nullopt;

        case SelectionPointerEventType::Move:
            if (leftPressed_ || input.buttons != Qt::NoButton) {
                return std::nullopt;
            }
            return SelectionInputAction{SelectionInputActionType::Hover, input.position,
                                        SelectionOperation::Replace};

        case SelectionPointerEventType::Release: {
            if (input.button != Qt::LeftButton || !leftPressed_) {
                return std::nullopt;
            }
            leftPressed_ = false;
            const QPointF delta = input.position - pressPosition_;
            if (std::abs(delta.x()) > clickTolerance_ || std::abs(delta.y()) > clickTolerance_) {
                return std::nullopt;
            }
            // Press anindaki modifier niyeti kullanilir. Pointer release sirasinda
            // kullanicinin modifier'i birakmasi selection semantigini degistirmez.
            if (pressModifiers_.testFlag(Qt::AltModifier)) {
                return std::nullopt;
            }
            return SelectionInputAction{SelectionInputActionType::Commit, input.position,
                                        operationForModifiers(pressModifiers_)};
        }

        case SelectionPointerEventType::Leave:
            leftPressed_ = false;
            return SelectionInputAction{SelectionInputActionType::ClearPreselection, {},
                                        SelectionOperation::Clear};
        }
        return std::nullopt;
    }

    [[nodiscard]] std::optional<SelectionInputAction> routeKey(const int key,
                                                                const Qt::KeyboardModifiers modifiers)
    {
        if (key == Qt::Key_Escape && modifiers == Qt::NoModifier) {
            leftPressed_ = false;
            return SelectionInputAction{SelectionInputActionType::Clear, {}, SelectionOperation::Clear};
        }
        return std::nullopt;
    }

    [[nodiscard]] bool clickInProgress() const noexcept { return leftPressed_; }

    [[nodiscard]] static SelectionOperation operationForModifiers(const Qt::KeyboardModifiers modifiers) noexcept
    {
        // macOS'ta fiziksel Command, Qt shortcut semantiginde ControlModifier
        // olarak ele alinir. Qt::MetaModifier kullanilmaz. Command daha spesifik
        // toggle niyetidir; Shift ile birlikte gelse de Toggle onceliklidir.
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
    QPointF pressPosition_;
    Qt::KeyboardModifiers pressModifiers_{Qt::NoModifier};
};

} // namespace d26
