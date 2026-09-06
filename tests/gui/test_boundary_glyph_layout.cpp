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
            bool valid = placements.size() == centres.size();
            for (const auto &p : placements) {
                origins.insert(p.origin);
                valid &= std::find(centres.begin(), centres.end(), p.origin) != centres.end()
                    && p.direction[axis] == sign && p.origin[2] == 7.0 && p.scale == 1.0;
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
    // Kullanıcının uzun üst yüzü: düzenli 20x4 alan ve çok daha ince mesh.
    // Başlangıç noktaları gerçek facet merkezleri, oklar aynı boydadır.
    for (int refine : {1,4}) {
        std::vector<std::array<double,3>> face;
        for (int v=0; v<4*refine; ++v) for (int u=0; u<20*refine; ++u)
            face.push_back({(u+0.5)/(20*refine),0.2*(v+0.5)/(4*refine),0});
        const auto layout=d26::boundaryGlyphLayout(face,{0,0,1},true);
        std::set<double> rows,columns;
        bool uniform=layout.size()==80;
        for (const auto &p : layout) {
            rows.insert(p.origin[1]); columns.insert(p.origin[0]);
            uniform &= p.scale==1.0 && p.direction[2]==1.0
                && std::find(face.begin(),face.end(),p.origin)!=face.end();
        }
        check(uniform && rows.size()==4 && columns.size()==20,
              "long Face retains a dense Cartesian 20-column 4-row field under refinement");
    }
    for (double scale : {0.001,1.0,1000.0}) {
        const double c=std::sqrt(0.5);
        const std::array<double,3> u{c,0,c}, v{0,1,0};
        std::vector<std::array<double,3>> face;
        for (int i=0; i<20; ++i) for (int j=0; j<4; ++j) {
            const double x=scale*(i+0.5)/20, y=scale*0.2*(j+0.5)/4;
            face.push_back({c*x,y,c*x});
        }
        const auto layout=d26::boundaryGlyphLayout(face,{1,0,-1},true,u,v);
        bool onFace=layout.size()==80;
        for (const auto &p : layout) onFace &= p.origin[0]==p.origin[2] && p.scale==1.0;
        check(onFace,"rotated Face uses local tangents and keeps density across SI scale changes");
    }
    std::vector<std::array<double,3>> fine;
    for (int i=0; i<80; ++i) for (int j=0; j<80; ++j)
        fine.push_back({double(i),double(j),0});
    check(d26::boundaryGlyphLayout(fine,{0,0,1},true).size()==400,
          "fine square Face is bounded at 20x20 arrows, not one glyph per FEM node");
    check(d26::boundaryGlyphLayout(fine,{0,0,1},true,{0,0,0},{0,1,0}).empty(),
          "invalid surface basis cannot invent a display plane");
    return failures ? 1 : 0;
}
