#include "DelaunayLocation.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

bool finiteCell(const DelaunayCellRecord& cell) {
    return std::all_of(cell.vertices.begin(), cell.vertices.end(),
                       [](const auto& v) { return v.isFinite(); });
}

LocatedCellEvidence evidence(std::size_t slot, const DelaunayCellSlot& cell) {
    auto key = cell.record.vertices;
    std::sort(key.begin(), key.end());
    return {key, {static_cast<std::uint32_t>(slot), cell.generation}};
}

} // namespace

DelaunayLocationResult locateBruteForceExact(
    std::span<const DelaunayCellSlot> slots,
    std::span<const CanonicalSite> sites,
    const geometry::Vec3& query) {
    if (!std::isfinite(query.x) || !std::isfinite(query.y) || !std::isfinite(query.z)) {
        throw std::invalid_argument("M2 location requires a finite query");
    }
    if (slots.size() > std::numeric_limits<std::uint32_t>::max()) {
        throw std::length_error("M2 location exceeds the cell handle domain");
    }
    std::map<PointId, geometry::Vec3> points;
    std::set<std::array<double, 3>> coordinates;
    for (const auto& site : sites) {
        if (site.id == InvalidPointId || !std::isfinite(site.point.x) ||
            !std::isfinite(site.point.y) || !std::isfinite(site.point.z) ||
            !points.emplace(site.id, site.point).second ||
            !coordinates.insert({site.point.x, site.point.y, site.point.z}).second) {
            throw std::invalid_argument("M2 location requires unique canonical finite sites");
        }
    }
    if (!validateDelaunayTopology(slots, sites).ok()) {
        throw std::invalid_argument("M2 location received invalid topology");
    }

    DelaunayLocationResult result;
    bool contained = false;
    for (const auto& slot : slots) {
        if (!slot.live || !finiteCell(slot.record)) continue;
        ++result.finiteCellsTested;
        std::array<geometry::Vec3, 4> p;
        for (std::size_t i = 0; i < 4; ++i) {
            p[i] = points.at(*slot.record.vertices[i].finitePointId());
        }
        // DELAUNAY_MATHEMATICS: lambda_i = N_i / D, D>0.
        // Bolme yapmadan dort payin isareti yeterlidir. Zero geometrik gercektir;
        // baska bir pay Negative ise duzlem uzantisindaki query contained degildir.
        const std::array<PredicateSign, 4> numerator{
            predicates::orient3d(query, p[1], p[2], p[3]).sign,
            predicates::orient3d(p[0], query, p[2], p[3]).sign,
            predicates::orient3d(p[0], p[1], query, p[3]).sign,
            predicates::orient3d(p[0], p[1], p[2], query).sign};
        if (std::find(numerator.begin(), numerator.end(), PredicateSign::Negative) !=
            numerator.end()) continue;

        std::vector<PointId> identity;
        for (std::size_t i = 0; i < 4; ++i) {
            if (numerator[i] == PredicateSign::Positive)
                identity.push_back(*slot.record.vertices[i].finitePointId());
        }
        if (identity.empty()) throw std::logic_error("M2 positive cell has four zero numerators");
        std::sort(identity.begin(), identity.end());
        if (contained && identity != result.entityVertices) {
            throw std::logic_error("M2 location found incompatible containing entities");
        }
        result.entityVertices = std::move(identity);
        contained = true;
        // Oracle ilk hit'te donmez; tum finite hucreleri bagimsiz kontrol eder.
    }
    if (result.finiteCellsTested == 0) {
        throw std::invalid_argument("M2 location requires a nonempty 3D finite complex");
    }
    if (contained) {
        switch (result.entityVertices.size()) {
        case 4: result.kind = DelaunayLocationKind::Cell; break;
        case 3: result.kind = DelaunayLocationKind::Facet; break;
        case 2: result.kind = DelaunayLocationKind::Edge; break;
        case 1: result.kind = DelaunayLocationKind::Vertex; break;
        default: throw std::logic_error("M2 invalid location identity");
        }
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if (!slots[i].live) continue;
            const auto& vertices = slots[i].record.vertices;
            const bool incident = std::all_of(result.entityVertices.begin(), result.entityVertices.end(),
                [&](PointId id) {
                    return std::find(vertices.begin(), vertices.end(), DelaunayVertexRef::finite(id)) !=
                           vertices.end();
                });
            if (incident) result.incidentCells.push_back(evidence(i, slots[i]));
        }
        std::sort(result.incidentCells.begin(), result.incidentCells.end(),
                  [](const auto& a, const auto& b) { return a.canonicalVertices < b.canonicalVertices; });
        return result;
    }

    // Outside bir tahmin degildir: outward hull facet icin O(a,b,c,query)>0
    // kaniti gerekir. Birden cok violated facet varsa canonical en kucugu secilir.
    for (std::size_t i = 0; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        if (!slot.live || finiteCell(slot.record)) continue;
        const std::array<PointId, 3> outward{
            *slot.record.vertices[1].finitePointId(),
            *slot.record.vertices[2].finitePointId(),
            *slot.record.vertices[3].finitePointId()};
        if (predicates::orient3d(points.at(outward[0]), points.at(outward[1]),
                                 points.at(outward[2]), query).sign != PredicateSign::Positive) continue;
        auto key = outward;
        std::sort(key.begin(), key.end());
        if (!result.outsideWitness || key < result.outsideWitness->canonicalFace) {
            result.outsideWitness = ViolatedHullWitness{
                key, outward, slot.record.neighbors[0],
                {static_cast<std::uint32_t>(i), slot.generation}};
        }
    }
    if (!result.outsideWitness) {
        throw std::logic_error("M2 location has neither containment nor a violated hull witness");
    }
    return result;
}

} // namespace femcae::meshing::m2
