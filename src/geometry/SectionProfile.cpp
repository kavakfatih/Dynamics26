#include "femcae/geometry/SectionProfile.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace femcae::geometry {
namespace {
struct RawMoments {
    double area{};
    double qx{}; // integral x dA
    double qy{}; // integral y dA
    double ixx0{};
    double iyy0{};
    double ixy0{};
};

RawMoments rawMoments(const SectionContour& contour) {
    if (contour.points.size() < 3) throw std::invalid_argument("Section contour en az 3 nokta icermeli.");
    RawMoments m;
    const double wantedSign = contour.hole ? -1.0 : 1.0;
    double signedA = signedPolygonArea(contour.points);
    if (std::abs(signedA) < 1.0e-18) throw std::invalid_argument("Section contour alani sifir/degenerate.");
    const double orientation = signedA > 0.0 ? 1.0 : -1.0;
    const double factor = wantedSign / orientation;
    for (std::size_t i = 0; i < contour.points.size(); ++i) {
        const auto& a = contour.points[i];
        const auto& b = contour.points[(i + 1) % contour.points.size()];
        const double cross = factor * (a.x * b.y - b.x * a.y);
        m.area += 0.5 * cross;
        m.qx += (a.x + b.x) * cross / 6.0;
        m.qy += (a.y + b.y) * cross / 6.0;
        m.ixx0 += (a.y * a.y + a.y * b.y + b.y * b.y) * cross / 12.0;
        m.iyy0 += (a.x * a.x + a.x * b.x + b.x * b.x) * cross / 12.0;
        m.ixy0 += (2.0 * a.x * a.y + a.x * b.y + b.x * a.y + 2.0 * b.x * b.y) * cross / 24.0;
    }
    return m;
}
}

void SectionProfile::clear() { contours_.clear(); }
void SectionProfile::addContour(SectionContour contour) { contours_.push_back(std::move(contour)); }
const std::vector<SectionContour>& SectionProfile::contours() const noexcept { return contours_; }

SectionProperties SectionProfile::properties() const {
    if (contours_.empty()) throw std::invalid_argument("Section profile contour icermiyor.");
    RawMoments total;
    for (const auto& contour : contours_) {
        const auto m = rawMoments(contour);
        total.area += m.area;
        total.qx += m.qx;
        total.qy += m.qy;
        total.ixx0 += m.ixx0;
        total.iyy0 += m.iyy0;
        total.ixy0 += m.ixy0;
    }
    if (total.area <= 1.0e-18) throw std::invalid_argument("Net section alani pozitif olmali.");

    SectionProperties p;
    p.area = total.area;
    p.centroid.x = total.qx / total.area;
    p.centroid.y = total.qy / total.area;
    p.ixx = total.ixx0 - total.area * p.centroid.y * p.centroid.y;
    p.iyy = total.iyy0 - total.area * p.centroid.x * p.centroid.x;
    p.ixy = total.ixy0 - total.area * p.centroid.x * p.centroid.y;
    p.polarJ = p.ixx + p.iyy;

    const double avg = 0.5 * (p.ixx + p.iyy);
    const double radius = std::hypot(0.5 * (p.ixx - p.iyy), p.ixy);
    p.principalI1 = avg + radius;
    p.principalI2 = avg - radius;
    p.principalAngleRad = 0.5 * std::atan2(-2.0 * p.ixy, p.iyy - p.ixx);
    return p;
}

double signedPolygonArea(const std::vector<Vec2>& points) {
    if (points.size() < 3) return 0.0;
    double twiceArea = 0.0;
    for (std::size_t i = 0; i < points.size(); ++i) {
        const auto& a = points[i];
        const auto& b = points[(i + 1) % points.size()];
        twiceArea += a.x * b.y - b.x * a.y;
    }
    return 0.5 * twiceArea;
}

bool pointInPolygon(const Vec2 point, const std::vector<Vec2>& polygon) {
    if (polygon.size() < 3) return false;
    bool inside = false;
    for (std::size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const auto& pi = polygon[i];
        const auto& pj = polygon[j];
        const bool crosses = ((pi.y > point.y) != (pj.y > point.y)) &&
            (point.x < (pj.x - pi.x) * (point.y - pi.y) / ((pj.y - pi.y) + 1.0e-300) + pi.x);
        if (crosses) inside = !inside;
    }
    return inside;
}

void classifyNestedContours(std::vector<SectionContour>& contours) {
    std::vector<double> absAreas(contours.size(), 0.0);
    for (std::size_t i = 0; i < contours.size(); ++i) {
        absAreas[i] = std::abs(signedPolygonArea(contours[i].points));
    }
    for (std::size_t i = 0; i < contours.size(); ++i) {
        if (contours[i].points.empty()) continue;
        // Bir loop yalniz kendisinden daha buyuk enclosing loop'lar tarafindan hole olabilir.
        // Es merkezli outer/inner contour'larda ortak centroid kullanmak outer loop'u yanlis
        // siniflandiracagi icin probe olarak ilk vertex'i kullaniyoruz.
        const Vec2 probe = contours[i].points.front();
        int depth = 0;
        for (std::size_t j = 0; j < contours.size(); ++j) {
            if (i == j || absAreas[j] <= absAreas[i]) continue;
            if (pointInPolygon(probe, contours[j].points)) ++depth;
        }
        contours[i].hole = (depth % 2) == 1;
    }
}

} // namespace femcae::geometry
