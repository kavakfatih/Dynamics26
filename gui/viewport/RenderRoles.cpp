#include "RenderRoles.h"

namespace d26 {

QColor Rgb::toQColor() const
{
    return QColor::fromRgbF(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b));
}

ViewportPalette ViewportPalette::forAppearance(const bool dark)
{
    ViewportPalette palette;
    palette.dark_ = dark;
    const auto set = [&palette](const RenderRole role, const double r, const double g, const double b) {
        palette.colors_[static_cast<std::size_t>(role)] = Rgb{r, g, b};
    };

    if (dark) {
        // Koyu görünüm: model arka plana gömülmez; yüzey nötr gri-mavi kalır,
        // kenarlar açık tonda okunur. Sonuç konturu bu paletten bağımsızdır.
        set(RenderRole::Background,         0.086, 0.094, 0.106);
        set(RenderRole::BackgroundGradient, 0.129, 0.141, 0.161);
        set(RenderRole::GeometrySurface,    0.478, 0.522, 0.588);
        set(RenderRole::GeometryEdge,       0.820, 0.855, 0.902);
        set(RenderRole::MeshSurface,        0.400, 0.451, 0.529);
        set(RenderRole::MeshEdge,           0.729, 0.776, 0.839);
        set(RenderRole::MeshNode,           0.851, 0.882, 0.925);
        set(RenderRole::Selection,          0.298, 0.686, 1.000);
        set(RenderRole::BoundaryCondition,  0.353, 0.784, 0.620);
        set(RenderRole::LoadGlyph,          0.961, 0.706, 0.278);
        set(RenderRole::ReferenceShape,     0.361, 0.388, 0.435);
        set(RenderRole::ResultContour,      0.800, 0.800, 0.800);
        set(RenderRole::ResultVector,       0.902, 0.925, 0.961);
        set(RenderRole::OverlayText,        0.902, 0.918, 0.945);
    } else {
        set(RenderRole::Background,         0.945, 0.949, 0.957);
        set(RenderRole::BackgroundGradient, 0.878, 0.890, 0.910);
        set(RenderRole::GeometrySurface,    0.702, 0.737, 0.788);
        set(RenderRole::GeometryEdge,       0.243, 0.278, 0.333);
        set(RenderRole::MeshSurface,        0.678, 0.718, 0.776);
        set(RenderRole::MeshEdge,           0.235, 0.267, 0.318);
        set(RenderRole::MeshNode,           0.169, 0.196, 0.239);
        set(RenderRole::Selection,          0.000, 0.427, 0.859);
        set(RenderRole::BoundaryCondition,  0.110, 0.529, 0.400);
        set(RenderRole::LoadGlyph,          0.839, 0.514, 0.075);
        set(RenderRole::ReferenceShape,     0.616, 0.639, 0.678);
        set(RenderRole::ResultContour,      0.300, 0.300, 0.300);
        set(RenderRole::ResultVector,       0.157, 0.184, 0.227);
        set(RenderRole::OverlayText,        0.145, 0.161, 0.192);
    }
    return palette;
}

Rgb ViewportPalette::color(const RenderRole role) const
{
    return colors_[static_cast<std::size_t>(role)];
}

} // namespace d26
