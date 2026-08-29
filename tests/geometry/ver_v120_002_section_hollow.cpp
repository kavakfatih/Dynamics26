#include "femcae/geometry/SectionProfile.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

using namespace femcae::geometry;
static void near(double a, double b, double tol, const char* m) { if (std::abs(a-b) > tol) { std::cerr<<"FAIL: "<<m<<" got="<<a<<" ref="<<b<<'\n'; std::exit(1);} }

int main() {
    constexpr double bo=0.100, ho=0.060, bi=0.080, hi=0.040;
    SectionProfile p;
    p.addContour({{{-bo/2,-ho/2},{bo/2,-ho/2},{bo/2,ho/2},{-bo/2,ho/2}}, false});
    // Ic contour'u bilerek ayni orientation ile veriyoruz; hole flag isareti fiziksel cikarmayi belirler.
    p.addContour({{{-bi/2,-hi/2},{bi/2,-hi/2},{bi/2,hi/2},{-bi/2,hi/2}}, true});
    const auto s=p.properties();
    near(s.area, bo*ho-bi*hi, 1e-14, "hollow area");
    near(s.ixx, (bo*ho*ho*ho-bi*hi*hi*hi)/12.0, 1e-15, "hollow Ixx");
    near(s.iyy, (ho*bo*bo*bo-hi*bi*bi*bi)/12.0, 1e-15, "hollow Iyy");
    near(s.centroid.x,0,1e-14,"hollow cx"); near(s.centroid.y,0,1e-14,"hollow cy");
    std::cout << "PASS VER-V120-002 hollow section properties\n";
}
