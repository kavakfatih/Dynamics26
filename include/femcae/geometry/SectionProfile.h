#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <string>
#include <vector>

namespace femcae::geometry {

struct SectionContour {
    std::vector<Vec2> points;
    bool hole{false};
};

struct SectionProperties {
    double area{0.0};
    Vec2 centroid{};
    double ixx{0.0};
    double iyy{0.0};
    double ixy{0.0};
    double principalI1{0.0};
    double principalI2{0.0};
    double principalAngleRad{0.0};
    double polarJ{0.0};
};

class SectionProfile {
public:
    void clear();
    void addContour(SectionContour contour);
    [[nodiscard]] const std::vector<SectionContour>& contours() const noexcept;
    [[nodiscard]] SectionProperties properties() const;

private:
    std::vector<SectionContour> contours_;
};

[[nodiscard]] double signedPolygonArea(const std::vector<Vec2>& points);
[[nodiscard]] bool pointInPolygon(Vec2 point, const std::vector<Vec2>& polygon);
void classifyNestedContours(std::vector<SectionContour>& contours);

} // namespace femcae::geometry
