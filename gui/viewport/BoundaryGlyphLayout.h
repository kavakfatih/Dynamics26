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
    std::array<double, 3> direction, bool load,
    std::array<double, 3> tangentU = {1,0,0},
    std::array<double, 3> tangentV = {0,1,0})
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
    if (load) {
        // Her CAD Face ayrı örneklenir. İlk gerçek QUAD4 facet'in kenarları
        // yerel yüz koordinatlarını verir; dünya eksenleri veya kuvvet yönü
        // örnekleme düzlemini belirlemez. Gram-Schmidt eğik kenarları da ayırır.
        const auto dot = [](const auto &a, const auto &b) {
            return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
        };
        const auto normalize = [](auto &v) {
            const double n = std::hypot(v[0],v[1],v[2]);
            if (!std::isfinite(n) || n <= 0) return false;
            for (auto &x : v) x /= n;
            return true;
        };
        if (!normalize(tangentU)) return {};
        const double uv = dot(tangentU,tangentV);
        for (int a=0; a<3; ++a) tangentV[a] -= uv*tangentU[a];
        if (!normalize(tangentV)) return {};
        std::vector<std::array<double,2>> coordinates;
        std::array<double,2> lo{INFINITY,INFINITY}, hi{-INFINITY,-INFINITY};
        for (const auto &p : centres) {
            std::array<double,3> relative{};
            for (int a=0; a<3; ++a) relative[a] = p[a]-mean[a];
            const std::array<double,2> c{dot(relative,tangentU),dot(relative,tangentV)};
            if (!std::isfinite(c[0]) || !std::isfinite(c[1])) return {};
            coordinates.push_back(c);
            for (int a=0; a<2; ++a) { lo[a]=std::min(lo[a],c[a]); hi[a]=std::max(hi[a],c[a]); }
        }
        const double span = std::max(hi[0]-lo[0],hi[1]-lo[1]);
        if (!std::isfinite(span)) return {};
        std::array<int,2> bins{};
        for (int a=0; a<2; ++a)
            bins[a] = span == 0 || hi[a]==lo[a] ? 1
                : std::clamp(static_cast<int>(std::ceil(20*(hi[a]-lo[a])/span)),4,20);
        // Uzun yönde 20, kısa yönde en az 4 hücre: beş seyrek ok yerine
        // düzenli bir alan gösterimi. Her dolu hücrede hedefe en yakın gerçek
        // facet merkezi seçilir; yüz boşluklarına yeni nokta uydurulmaz.
        const auto missing = centres.size();
        std::vector<std::size_t> chosen(static_cast<std::size_t>(bins[0]*bins[1]),missing);
        std::vector<double> best(chosen.size(),INFINITY);
        for (std::size_t i=0; i<centres.size(); ++i) {
            std::array<int,2> cell{};
            double score=0;
            for (int a=0; a<2; ++a) {
                const double t = hi[a]==lo[a] ? 0.5 : (coordinates[i][a]-lo[a])/(hi[a]-lo[a]);
                cell[a] = std::min(bins[a]-1,static_cast<int>(t*bins[a]));
                score += std::pow(t*bins[a]-cell[a]-0.5,2);
            }
            const auto index=static_cast<std::size_t>(cell[1]*bins[0]+cell[0]);
            if (score < best[index] || (score == best[index]
                    && (chosen[index]==missing || centres[i]<centres[chosen[index]]))) {
                chosen[index]=i; best[index]=score;
            }
        }
        std::vector<BoundaryGlyphPlacement> result;
        for (const auto i : chosen) if (i != missing) result.push_back({centres[i],direction,1.0});
        return result;
    }
    std::size_t next = 0;
    for (std::size_t i = 1; i < centres.size(); ++i)
        if (distance(centres[i], mean) < distance(centres[next], mean)) next = i;
    std::vector<BoundaryGlyphPlacement> result;
    std::vector<double> nearest(centres.size(), std::numeric_limits<double>::infinity());
    const auto count = std::min<std::size_t>(centres.size(), 6);
    for (std::size_t n = 0; n < count; ++n) {
        result.push_back({centres[next], direction, 1.0});
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
