#pragma once

#include "femcae/geometry/GeometryDocument.h"

#include <memory>
#include <optional>
#include <string>

namespace femcae::geometry {

struct StepImportResult {
    bool success{false};
    std::string message;
    std::size_t assemblyCount{0};
    std::size_t bodyCount{0};
    std::size_t faceCount{0};
    std::size_t edgeCount{0};
    std::size_t vertexCount{0};
};


struct StepAxisAlignedBoxDescriptor {
    Vec3 min;
    Vec3 max;
    GeometryEntityId bodyId{InvalidGeometryId};
    GeometryEntityId xMinFace{InvalidGeometryId};
    GeometryEntityId xMaxFace{InvalidGeometryId};
    GeometryEntityId yMinFace{InvalidGeometryId};
    GeometryEntityId yMaxFace{InvalidGeometryId};
    GeometryEntityId zMinFace{InvalidGeometryId};
    GeometryEntityId zMaxFace{InvalidGeometryId};
};

class OcctStepImporter {
public:
    OcctStepImporter();
    ~OcctStepImporter();
    OcctStepImporter(OcctStepImporter&&) noexcept;
    OcctStepImporter& operator=(OcctStepImporter&&) noexcept;
    OcctStepImporter(const OcctStepImporter&) = delete;
    OcctStepImporter& operator=(const OcctStepImporter&) = delete;

    [[nodiscard]] static bool available() noexcept;
    [[nodiscard]] StepImportResult importFile(const std::string& path, GeometryDocument& document);
    [[nodiscard]] GeometryTessellation tessellate(GeometryEntityId bodyId,
                                                   double linearDeflection,
                                                   double angularDeflectionRad = 0.5) const;
    [[nodiscard]] std::optional<StepAxisAlignedBoxDescriptor> axisAlignedBoxDescriptor(GeometryEntityId bodyId,
                                                                                       double tolerance = 1.0e-8) const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace femcae::geometry
