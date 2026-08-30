#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace femcae::geometry {

using GeometryEntityId = std::uint64_t;
inline constexpr GeometryEntityId InvalidGeometryId = 0;

enum class GeometryEntityKind : std::uint8_t {
    Assembly,
    Body,
    Face,
    Edge,
    Vertex,
    Surface,
    Curve
};

struct Vec2 {
    double x{0.0};
    double y{0.0};
};

struct Vec3 {
    double x{0.0};
    double y{0.0};
    double z{0.0};
};

struct RgbaColor {
    double r{0.75};
    double g{0.75};
    double b{0.78};
    double a{1.0};
};

struct GeometryEntity {
    GeometryEntityId id{InvalidGeometryId};
    GeometryEntityId parentId{InvalidGeometryId};
    GeometryEntityKind kind{GeometryEntityKind::Body};
    std::string name;
    std::string persistentKey;
    std::string sourcePath;
    std::optional<RgbaColor> color;
};

// Bu veri yalnizca ekranda gosterim icindir. Solver node/element kimlikleri burada yoktur.
struct GeometryTessellation {
    GeometryEntityId sourceGeometryId{InvalidGeometryId};
    std::uint64_t sourceRevision{0};
    std::vector<Vec3> points;
    std::vector<std::array<std::uint32_t, 3>> triangles;
};

// CAD topolojisi ile display tessellation arasindaki provenance yan-karti.
//
// KRITIK: triangleFaceIds içindeki bir kimlik CAD Face kimligidir; display
// triangle'in kendisi CAD Face veya FEM elemani DEĞILDIR. Mevcut
// GeometryTessellation yapisinin layout'unu degistirmeden Alpha.3.2 Face
// selection icin gereken bire-bir triangle -> Face bagini tasir.
struct TopologyTessellation {
    GeometryTessellation display;
    std::vector<GeometryEntityId> triangleFaceIds;

    [[nodiscard]] bool hasConsistentProvenance() const noexcept {
        return triangleFaceIds.size() == display.triangles.size();
    }
};

struct GeometryAssociation {
    GeometryEntityId geometryId{InvalidGeometryId};
    std::vector<std::int64_t> femNodeIds;
    std::vector<std::int64_t> femElementIds;
    std::vector<std::int64_t> femFacetIds;
};

} // namespace femcae::geometry
