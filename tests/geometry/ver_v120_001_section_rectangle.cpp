#include "femcae/geometry/SectionProfile.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace femcae::geometry;
static void near(double a, double b, double tol, const char* m) { if (std::abs(a-b) > tol) { std::cerr<<"FAIL: "<<m<<" got="<<a<<" ref="<<b<<'\n'; std::exit(1);} }

int main() {
    constexpr double b = 0.040;
    constexpr double h = 0.020;
    SectionProfile profile;
    profile.addContour({{{-b/2,-h/2},{b/2,-h/2},{b/2,h/2},{-b/2,h/2}}, false});
    const auto p = profile.properties();
    near(p.area, b*h, 1e-14, "rectangle area");
    near(p.centroid.x, 0.0, 1e-14, "rectangle cx");
    near(p.centroid.y, 0.0, 1e-14, "rectangle cy");
    near(p.ixx, b*h*h*h/12.0, 1e-16, "rectangle Ixx");
    near(p.iyy, h*b*b*b/12.0, 1e-16, "rectangle Iyy");
    near(p.ixy, 0.0, 1e-16, "rectangle Ixy");
    near(p.polarJ, p.ixx+p.iyy, 1e-16, "rectangle polar J");
    std::cout << "PASS VER-V120-001 rectangle section properties\n";
}
