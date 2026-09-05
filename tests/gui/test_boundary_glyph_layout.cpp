#include "../../gui/viewport/BoundaryGlyphLayout.h"
#include <iostream>
#include <set>

int main()
{
    int failures = 0;
    const auto check = [&](bool ok, const char *message) {
        std::cout << (ok ? "PASS " : "FAIL ") << message << '\n';
        if (!ok) ++failures;
    };
    for (int divisions : {1, 2, 4, 16}) {
        std::vector<std::array<double, 3>> centres;
        for (int i = 0; i < divisions; ++i) for (int j = 0; j < divisions; ++j)
            centres.push_back({2.0 + (i+0.5)/divisions, -3.0+(j+0.5)/divisions, 7.0});
        for (int axis = 0; axis < 3; ++axis) for (double sign : {-1.0, 1.0}) {
            std::array<double, 3> vector{}; vector[axis] = 1000.0 * sign;
            const auto placements = d26::boundaryGlyphLayout(centres, vector, true);
            std::set<std::array<double, 3>> origins;
            bool valid = placements.size() == std::min<std::size_t>(5, centres.size());
            for (const auto &p : placements) {
                origins.insert(p.origin);
                valid &= std::find(centres.begin(), centres.end(), p.origin) != centres.end()
                    && p.direction[axis] == sign && p.origin[2] == 7.0;
            }
            check(valid && origins.size() == placements.size(), "signed XYZ directions keep unique seeds on translated surface; density capped");
        }
        check(d26::boundaryGlyphLayout(centres, {0,0,0}, true).empty(), "zero force has no resultant glyph");
        check(d26::boundaryGlyphLayout(centres, {NAN,0,0}, true).empty(), "invalid force has no glyph");
        check(d26::boundaryGlyphLayout(centres, {INFINITY,0,0}, true).empty(), "infinite force has no glyph");
    }
    const std::vector<std::array<double, 3>> multi{{0,0,1},{0,0,1},{1,0,0},{1,1,0},{0,1,1}};
    const auto a = d26::boundaryGlyphLayout(multi, {1,2,3}, true);
    const auto b = d26::boundaryGlyphLayout(multi, {1,2,3}, true);
    bool valid = a.size() == 4 && b.size() == a.size();
    for (std::size_t i=0; i<a.size(); ++i) valid &= a[i].origin == b[i].origin
        && std::find(multi.begin(), multi.end(), a[i].origin) != multi.end()
        && std::abs(std::hypot(a[i].direction[0],a[i].direction[1],a[i].direction[2])-1.0)<1e-14;
    check(valid, "multi-Face samples remain on actual facets and ordering is deterministic");
    return failures ? 1 : 0;
}
