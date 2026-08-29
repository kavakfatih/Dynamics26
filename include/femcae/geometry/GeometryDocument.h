#pragma once

#include "femcae/geometry/GeometryTypes.h"

#include <cstddef>
#include <string_view>
#include <unordered_map>

namespace femcae::geometry {

GeometryEntityId makePersistentGeometryId(std::string_view documentNamespace,
                                          std::string_view persistentKey);

class GeometryDocument {
public:
    explicit GeometryDocument(std::string documentNamespace = "femcae-default");

    void clear();
    [[nodiscard]] const std::string& documentNamespace() const noexcept;
    [[nodiscard]] std::uint64_t revision() const noexcept;

    GeometryEntityId addEntity(GeometryEntityKind kind,
                               GeometryEntityId parentId,
                               std::string name,
                               std::string persistentKey,
                               std::string sourcePath = {});

    [[nodiscard]] bool contains(GeometryEntityId id) const noexcept;
    [[nodiscard]] const GeometryEntity* find(GeometryEntityId id) const noexcept;
    [[nodiscard]] GeometryEntity* findMutable(GeometryEntityId id) noexcept;
    [[nodiscard]] std::vector<GeometryEntityId> childrenOf(GeometryEntityId parentId) const;
    [[nodiscard]] std::vector<GeometryEntityId> entitiesOfKind(GeometryEntityKind kind) const;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::string namespace_;
    std::uint64_t revision_{0};
    std::vector<GeometryEntity> entities_;
    std::unordered_map<GeometryEntityId, std::size_t> positionById_;
};

class GeometryAssociationMap {
public:
    void clear();
    void set(GeometryAssociation association);
    [[nodiscard]] const GeometryAssociation* find(GeometryEntityId geometryId) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

private:
    std::unordered_map<GeometryEntityId, GeometryAssociation> associations_;
};

} // namespace femcae::geometry
