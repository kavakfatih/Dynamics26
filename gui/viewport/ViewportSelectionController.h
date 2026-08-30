#pragma once

// Dynamics26 Alpha.3.2 — viewport selection input state machine.
//
// Bu katman kamera hareketi YAPMAZ ve VTK pick YAPMAZ. Qt pointer/key olayından
// "hover / commit / clear" niyetini üretir. Navigation katmanı kamera jestlerini
// işler; bu state machine yalnız selection'a kalan semantiği sınıflandırır.

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
            // Press anındaki modifier niyeti kullanılır. Pointer release sırasında
            // kullanıcının modifier'ı bırakması selection semantiğini değiştirmez.
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
        // Qt raw pointer event'lerinde macOS fiziksel Command tuşu MetaModifier,
        // standart shortcut gösteriminde ise Ctrl semantiğiyle sunulur. Selection
        // mouse kontratı her iki kaynağı Toggle olarak kabul eder; platforma özgü
        // sabit/hack yolu kullanılmaz.
        if (modifiers.testFlag(Qt::MetaModifier) || modifiers.testFlag(Qt::ControlModifier)) {
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
