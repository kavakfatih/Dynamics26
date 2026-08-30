#pragma once

// Semantik render rolleri (§18).
//
// Viewport aktörleri "wireframe mi?", "edgeVisibility açık mı?" gibi property
// incelemesiyle renklendirilmez. Her aktör oluşturulurken taşıdığı mühendislik
// anlamını (rolünü) bildirir; Light/Dark ve bağlam değişiminde renk yalnız bu
// role bakılarak yeniden uygulanır.

#include <QColor>

#include <array>

namespace d26 {

enum class RenderRole {
    GeometrySurface,
    GeometryEdge,
    MeshSurface,
    MeshEdge,
    MeshNode,
    Selection,
    Preselection,
    BoundaryCondition,
    LoadGlyph,
    ReferenceShape,
    ResultContour,
    ResultVector,
    Background,
    BackgroundGradient,
    OverlayText
};

struct Rgb {
    double r{0.0};
    double g{0.0};
    double b{0.0};
    [[nodiscard]] QColor toQColor() const;
};

// Rol -> renk eşlemesi. Light ve Dark için iki ayrı tam palet vardır; geçişte
// hiçbir rol eski paletten renk taşımaz.
class ViewportPalette
{
public:
    static ViewportPalette forAppearance(bool dark);

    [[nodiscard]] Rgb color(RenderRole role) const;
    [[nodiscard]] bool isDark() const noexcept { return dark_; }
    // Sonuç konturu için hue aralığı (mavi -> kırmızı).
    [[nodiscard]] double contourHueStart() const noexcept { return 0.667; }
    [[nodiscard]] double contourHueEnd() const noexcept { return 0.0; }

private:
    bool dark_{false};
    std::array<Rgb, 15> colors_{};
};

} // namespace d26
