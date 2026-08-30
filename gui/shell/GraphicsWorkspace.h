#pragma once

// Merkez grafik çalışma alanı.
//
// Viewport + ince bir grafik araç çubuğu + bağlam göstergesi. Araç çubuğu
// COMSOL'daki grafik penceresi çubuğu gibi tek satır, ikon boyutlu ve
// bağlama duyarlıdır; ikinci bir ribbon değildir.

#include "../core/ProjectTypes.h"
#include "../viewport/ViewportWidget.h"

#include <QFrame>

class QLabel;
class QToolBar;
class QToolButton;
class QActionGroup;

namespace d26 {

enum class SelectionFilter { Body, Face };

class GraphicsWorkspace final : public QFrame
{
    Q_OBJECT
public:
    explicit GraphicsWorkspace(QWidget *parent = nullptr);

    [[nodiscard]] ViewportWidget *viewport() const noexcept { return viewport_; }
    void setContextLabel(const QString &text);
    void setSelectionLabel(const QString &text);
    [[nodiscard]] SelectionFilter selectionFilter() const noexcept { return filter_; }
    void setSelectionFilter(SelectionFilter filter);
    // Face seçimi FEM mesh'e değil gerçek CAD display provenance'ına bağlıdır.
    // CAD sahnesi cell -> Face kimliği taşıyabildiğinde etkinleşir.
    void setFaceSelectionAvailable(bool available);
    void refreshIcons();

signals:
    void fitViewRequested();
    void isometricViewRequested();
    void selectionFilterChanged(SelectionFilter filter);

private:
    ViewportWidget *viewport_{nullptr};
    QToolBar *toolbar_{nullptr};
    QLabel *contextLabel_{nullptr};
    QLabel *selectionLabel_{nullptr};
    QAction *selectBody_{nullptr};
    QAction *selectFace_{nullptr};
    QAction *fit_{nullptr};
    QAction *isometric_{nullptr};
    SelectionFilter filter_{SelectionFilter::Body};
};

} // namespace d26
