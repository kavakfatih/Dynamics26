#include "femcae/geometry/DxfSectionReader.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace femcae::geometry {
namespace {
struct Pair { int code{}; std::string value; };
struct Segment { Vec2 a{}, b{}; };

std::string trim(std::string s) {
    const auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

double valueAsDouble(const std::vector<Pair>& e, int code, double fallback = 0.0) {
    for (const auto& p : e) if (p.code == code) return std::stod(p.value);
    return fallback;
}

int valueAsInt(const std::vector<Pair>& e, int code, int fallback = 0) {
    for (const auto& p : e) if (p.code == code) return std::stoi(p.value);
    return fallback;
}

bool close(Vec2 a, Vec2 b, double tol) {
    return std::hypot(a.x - b.x, a.y - b.y) <= tol;
}

std::vector<Vec2> sampleArc(Vec2 center, double radius, double startDeg, double endDeg, double maxAngleDeg) {
    while (endDeg <= startDeg) endDeg += 360.0;
    const double span = endDeg - startDeg;
    const std::size_t n = std::max<std::size_t>(2, static_cast<std::size_t>(std::ceil(span / std::max(0.1, maxAngleDeg))) + 1);
    std::vector<Vec2> points;
    points.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        const double angleDeg = startDeg + span * static_cast<double>(i) / static_cast<double>(n - 1);
        const double a = angleDeg * std::numbers::pi / 180.0;
        points.push_back({center.x + radius * std::cos(a), center.y + radius * std::sin(a)});
    }
    return points;
}

void appendPolylineSegments(const std::vector<Vec2>& pts, bool closed, std::vector<Segment>& segments) {
    if (pts.size() < 2) return;
    for (std::size_t i = 1; i < pts.size(); ++i) segments.push_back({pts[i - 1], pts[i]});
    if (closed && !close(pts.front(), pts.back(), 1.0e-14)) segments.push_back({pts.back(), pts.front()});
}

std::vector<SectionContour> buildContours(std::vector<Segment> segments, double tol) {
    std::vector<SectionContour> contours;
    while (!segments.empty()) {
        SectionContour contour;
        contour.points.push_back(segments.front().a);
        Vec2 current = segments.front().b;
        contour.points.push_back(current);
        segments.erase(segments.begin());

        std::size_t guard = 0;
        while (!close(current, contour.points.front(), tol)) {
            if (++guard > 100000) throw std::runtime_error("DXF contour reconstruction iteration guard.");
            auto it = std::find_if(segments.begin(), segments.end(), [&](const Segment& s) {
                return close(s.a, current, tol) || close(s.b, current, tol);
            });
            if (it == segments.end()) throw std::runtime_error("DXF section acik contour iceriyor.");
            if (close(it->a, current, tol)) current = it->b;
            else current = it->a;
            if (!close(current, contour.points.front(), tol)) contour.points.push_back(current);
            segments.erase(it);
        }
        if (contour.points.size() < 3) throw std::runtime_error("DXF section contour en az 3 benzersiz nokta icermeli.");
        contours.push_back(std::move(contour));
    }
    classifyNestedContours(contours);
    return contours;
}

std::vector<Pair> readPairs(std::istream& input) {
    std::vector<Pair> pairs;
    std::string codeLine, valueLine;
    while (std::getline(input, codeLine)) {
        if (!std::getline(input, valueLine)) throw std::runtime_error("DXF group code/value cifti yarim kaldi.");
        pairs.push_back({std::stoi(trim(codeLine)), trim(valueLine)});
    }
    return pairs;
}
}

DxfSectionResult DxfSectionReader::readFile(const std::string& path, const DxfSectionOptions& options) const {
    DxfSectionResult result;
    try {
        if (options.joinTolerance <= 0.0 || options.circleSegments < 16 || options.arcMaxAngleDeg <= 0.0) {
            throw std::invalid_argument("DXF section options gecersiz.");
        }
        std::ifstream input(path);
        if (!input) throw std::runtime_error("DXF dosyasi acilamadi: " + path);
        const auto pairs = readPairs(input);
        bool inEntities = false;
        std::vector<Segment> segments;
        std::size_t i = 0;
        while (i < pairs.size()) {
            if (pairs[i].code == 0 && pairs[i].value == "SECTION") {
                if (i + 1 < pairs.size() && pairs[i + 1].code == 2 && pairs[i + 1].value == "ENTITIES") inEntities = true;
                i += 2;
                continue;
            }
            if (pairs[i].code == 0 && pairs[i].value == "ENDSEC") { inEntities = false; ++i; continue; }
            if (!inEntities || pairs[i].code != 0) { ++i; continue; }

            const std::string type = pairs[i].value;
            std::vector<Pair> entity;
            entity.push_back(pairs[i++]);
            while (i < pairs.size() && pairs[i].code != 0) entity.push_back(pairs[i++]);

            if (type == "LINE") {
                segments.push_back({{valueAsDouble(entity, 10), valueAsDouble(entity, 20)},
                                    {valueAsDouble(entity, 11), valueAsDouble(entity, 21)}});
                ++result.entityCount;
            } else if (type == "LWPOLYLINE") {
                std::vector<Vec2> pts;
                std::optional<double> pendingX;
                for (const auto& p : entity) {
                    if (p.code == 42 && std::abs(std::stod(p.value)) > 1.0e-14) {
                        throw std::runtime_error("LWPOLYLINE bulge arc desteklenmiyor; ARC entity kullanin.");
                    }
                    if (p.code == 10) pendingX = std::stod(p.value);
                    else if (p.code == 20 && pendingX) { pts.push_back({*pendingX, std::stod(p.value)}); pendingX.reset(); }
                }
                appendPolylineSegments(pts, (valueAsInt(entity, 70) & 1) != 0, segments);
                ++result.entityCount;
            } else if (type == "CIRCLE") {
                const Vec2 c{valueAsDouble(entity, 10), valueAsDouble(entity, 20)};
                const double r = valueAsDouble(entity, 40);
                if (r <= 0.0) throw std::runtime_error("DXF circle radius pozitif olmali.");
                std::vector<Vec2> pts;
                pts.reserve(options.circleSegments);
                for (std::size_t k = 0; k < options.circleSegments; ++k) {
                    const double a = 2.0 * std::numbers::pi * static_cast<double>(k) / static_cast<double>(options.circleSegments);
                    pts.push_back({c.x + r * std::cos(a), c.y + r * std::sin(a)});
                }
                appendPolylineSegments(pts, true, segments);
                ++result.entityCount;
            } else if (type == "ARC") {
                const Vec2 c{valueAsDouble(entity, 10), valueAsDouble(entity, 20)};
                const double r = valueAsDouble(entity, 40);
                if (r <= 0.0) throw std::runtime_error("DXF arc radius pozitif olmali.");
                const auto pts = sampleArc(c, r, valueAsDouble(entity, 50), valueAsDouble(entity, 51), options.arcMaxAngleDeg);
                appendPolylineSegments(pts, false, segments);
                ++result.entityCount;
            }
        }
        if (segments.empty()) throw std::runtime_error("DXF ENTITIES bolumunde section geometrisi bulunamadi.");
        auto contours = buildContours(std::move(segments), options.joinTolerance);
        for (auto& contour : contours) result.profile.addContour(std::move(contour));
        (void)result.profile.properties(); // Net area/closed contour validation.
        result.success = true;
        result.message = "OK";
    } catch (const std::exception& ex) {
        result.success = false;
        result.message = ex.what();
    }
    return result;
}

} // namespace femcae::geometry
