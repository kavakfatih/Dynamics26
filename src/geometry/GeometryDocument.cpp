#include "femcae/geometry/GeometryDocument.h"

#include <stdexcept>
#include <utility>

namespace femcae::geometry {
namespace {
constexpr std::uint64_t fnvOffset = 14695981039346656037ull;
constexpr std::uint64_t fnvPrime = 1099511628211ull;

void fnvAppend(std::uint64_t& hash, std::string_view value) {
    for (const unsigned char byte : value) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= fnvPrime;
    }
}
}

GeometryEntityId makePersistentGeometryId(const std::string_view documentNamespace,
                                          const std::string_view persistentKey) {
    std::uint64_t hash = fnvOffset;
    fnvAppend(hash, documentNamespace);
    hash ^= 0xffu;
    hash *= fnvPrime;
    fnvAppend(hash, persistentKey);
    // 0 tum geometry domaininde invalid ID'dir.
    return hash == InvalidGeometryId ? 1u : hash;
}

GeometryDocument::GeometryDocument(std::string documentNamespace)
    : namespace_(std::move(documentNamespace)) {
    if (namespace_.empty()) {
        throw std::invalid_argument("Geometry document namespace bos olamaz.");
    }
}

void GeometryDocument::clear() {
    entities_.clear();
    positionById_.clear();
    ++revision_;
}

const std::string& GeometryDocument::documentNamespace() const noexcept { return namespace_; }
std::uint64_t GeometryDocument::revision() const noexcept { return revision_; }

GeometryEntityId GeometryDocument::addEntity(const GeometryEntityKind kind,
                                             const GeometryEntityId parentId,
                                             std::string name,
                                             std::string persistentKey,
                                             std::string sourcePath) {
    if (persistentKey.empty()) {
        throw std::invalid_argument("Geometry persistent key bos olamaz.");
    }
    if (parentId != InvalidGeometryId && !contains(parentId)) {
        throw std::invalid_argument("Geometry parent ID dokumanda bulunamadi.");
    }
    const auto id = makePersistentGeometryId(namespace_, persistentKey);
    if (contains(id)) {
        throw std::invalid_argument("Duplicate persistent geometry key/ID reddedildi.");
    }
    GeometryEntity entity;
    entity.id = id;
    entity.parentId = parentId;
    entity.kind = kind;
    entity.name = std::move(name);
    entity.persistentKey = std::move(persistentKey);
    entity.sourcePath = std::move(sourcePath);
    positionById_.emplace(id, entities_.size());
    entities_.push_back(std::move(entity));
    ++revision_;
    return id;
}

bool GeometryDocument::contains(const GeometryEntityId id) const noexcept {
    return positionById_.find(id) != positionById_.end();
}

const GeometryEntity* GeometryDocument::find(const GeometryEntityId id) const noexcept {
    const auto it = positionById_.find(id);
    return it == positionById_.end() ? nullptr : &entities_[it->second];
}

GeometryEntity* GeometryDocument::findMutable(const GeometryEntityId id) noexcept {
    const auto it = positionById_.find(id);
    if (it == positionById_.end()) return nullptr;
    ++revision_;
    return &entities_[it->second];
}

std::vector<GeometryEntityId> GeometryDocument::childrenOf(const GeometryEntityId parentId) const {
    std::vector<GeometryEntityId> result;
    for (const auto& entity : entities_) {
        if (entity.parentId == parentId) result.push_back(entity.id);
    }
    return result;
}

std::vector<GeometryEntityId> GeometryDocument::entitiesOfKind(const GeometryEntityKind kind) const {
    std::vector<GeometryEntityId> result;
    for (const auto& entity : entities_) {
        if (entity.kind == kind) result.push_back(entity.id);
    }
    return result;
}

std::size_t GeometryDocument::size() const noexcept { return entities_.size(); }

void GeometryAssociationMap::clear() { associations_.clear(); }
void GeometryAssociationMap::set(GeometryAssociation association) {
    if (association.geometryId == InvalidGeometryId) {
        throw std::invalid_argument("Geometry association invalid ID kullanamaz.");
    }
    associations_[association.geometryId] = std::move(association);
}
const GeometryAssociation* GeometryAssociationMap::find(const GeometryEntityId geometryId) const noexcept {
    const auto it = associations_.find(geometryId);
    return it == associations_.end() ? nullptr : &it->second;
}
std::size_t GeometryAssociationMap::size() const noexcept { return associations_.size(); }

} // namespace femcae::geometry
