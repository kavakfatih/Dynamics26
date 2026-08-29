#pragma once

#include "femcae/geometry/SectionProfile.h"

#include <cstddef>
#include <string>

namespace femcae::geometry {

struct DxfSectionOptions {
    double joinTolerance{1.0e-8};
    std::size_t circleSegments{128};
    double arcMaxAngleDeg{5.0};
};

struct DxfSectionResult {
    bool success{false};
    std::string message;
    SectionProfile profile;
    std::size_t entityCount{0};
};

class DxfSectionReader {
public:
    [[nodiscard]] DxfSectionResult readFile(const std::string& path,
                                            const DxfSectionOptions& options = {}) const;
};

} // namespace femcae::geometry
