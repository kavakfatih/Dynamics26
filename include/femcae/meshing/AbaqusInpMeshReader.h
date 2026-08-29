#pragma once

#include "femcae/meshing/MeshTypes.h"

#include <filesystem>

namespace femcae::meshing {

// Portable external-mesh baseline: Abaqus ASCII .inp *NODE + C3D8 *ELEMENT.
// Material/BC keyword'leri bu reader'in kapsaminda degildir; yalniz mesh topolojisi okunur.
class AbaqusInpMeshReader {
public:
    [[nodiscard]] SimulationMesh read(const std::filesystem::path& path) const;
};

} // namespace femcae::meshing
