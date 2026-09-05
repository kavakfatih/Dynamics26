#pragma once

// Yalnız presentation örneklemesi: FEM load vector veya scope kimliği üretmez.
// Tohumlar gerçek boundary facet merkezlerinden seçilir; çok yüzlü kapsamın
// havada kalabilecek ortalaması sembol başlangıcı olarak kullanılmaz.
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace d26 {
struct BoundaryGlyphPlacement {
    std::array<double, 3> origin;
    std::array<double, 3> direction;
    double scale{1.0};
};

inline std::vector<BoundaryGlyphPlacement> boundaryGlyphLayout(
    const std::vector<std::array<double, 3>> &centres,
    std::array<double, 3> direction, bool load)
{
    const double norm = std::hypot(direction[0], direction[1], direction[2]);
    if (centres.empty() || !std::isfinite(norm) || norm <= 1.0e-12) return {};
    for (auto &v : direction) v /= norm;
    std::array<double, 3> mean{};
    for (const auto &p : centres) for (int a = 0; a < 3; ++a) {
        if (!std::isfinite(p[a])) return {};
        mean[a] += p[a] / static_cast<double>(centres.size());
    }
    const auto distance = [](const auto &a, const auto &b) {
        return std::hypot(a[0]-b[0], a[1]-b[1], a[2]-b[2]);
    };
    std::size_t next = 0;
    for (std::size_t i = 1; i < centres.size(); ++i)
        if (distance(centres[i], mean) < distance(centres[next], mean)) next = i;
    std::vector<BoundaryGlyphPlacement> result;
    std::vector<double> nearest(centres.size(), std::numeric_limits<double>::infinity());
    const auto count = std::min<std::size_t>(centres.size(), load ? 5 : 6);
    for (std::size_t n = 0; n < count; ++n) {
        result.push_back({centres[next], direction, load && n > 0 ? 0.55 : 1.0});
        for (std::size_t i = 0; i < centres.size(); ++i)
            nearest[i] = std::min(nearest[i], distance(centres[i], centres[next]));
        // Farthest-point örnekleme çizgisel indeks atlamasının aynı sütunda
        // kümelenmesini önler. Eşit mesafede ilk indeks deterministic kalır.
        next = static_cast<std::size_t>(std::max_element(nearest.begin(), nearest.end()) - nearest.begin());
        if (nearest[next] == 0.0) break;
    }
    return result;
}
} // namespace d26
