#pragma once

// Dynamics26 Alpha.3.4 — viewport selection input state machine.
//
// Bu katman kamera hareketi YAPMAZ ve VTK pick YAPMAZ. Qt pointer/key olayından
// "hover / commit / clear / secondary-context" niyetini üretir. Navigation
// katmanı kamera jestlerini işler; bu state machine yalnız selection'a kalan
// semantiği sınıflandırır. Geometry ve Mesh bridge'leri ayni state machine'i
// kullanir; baglam degisiminde yarim gesture acikca iptal edilir.

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
    ContextMenu,
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
                leftPressPosition_ = input.position;
                leftPressModifiers_ = input.modifiers;
            } else if (input.button == Qt::RightButton) {
                rightPressed_ = true;
                rightPressPosition_ = input.position;
            }
            return std::nullopt;

        case SelectionPointerEventType::Move:
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
                // Secondary click selection'i burada değiştirmez. Hit'in mevcut
                // selection setinde olup olmadığına coordinator karar verir;
                // böylece seçili entity sağ tıkta multi-selection korunabilir.
                return SelectionInputAction{SelectionInputActionType::ContextMenu, input.position,
                                            SelectionOperation::Replace};
            }

            if (input.button != Qt::LeftButton || !leftPressed_) {
                return std::nullopt;
            }
            leftPressed_ = false;
            const QPointF delta = input.position - leftPressPosition_;
            if (std::abs(delta.x()) > clickTolerance_ || std::abs(delta.y()) > clickTolerance_) {
                return std::nullopt;
            }
            // Press anındaki modifier niyeti kullanılır. Pointer release sırasında
            // kullanıcının modifier'ı bırakması selection semantiğini değiştirmez.
            if (leftPressModifiers_.testFlag(Qt::AltModifier)) {
                return std::nullopt;
            }
            return SelectionInputAction{SelectionInputActionType::Commit, input.position,
                                        operationForModifiers(leftPressModifiers_)};
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
        // Qt'nin varsayılan Apple platform eşlemesinde fiziksel Command tuşu
        // Qt::ControlModifier, fiziksel Control tuşu ise Qt::MetaModifier olarak
        // raporlanır. Dynamics26 selection kontratı yalnız Command+Click'i Toggle
        // kabul eder; fiziksel Control+Click sessizce aynı komut gibi davranmaz.
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
