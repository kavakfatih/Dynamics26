#include "meshing/m2/DelaunayLocation.h"
#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace femcae::meshing;
using namespace femcae::meshing::m2;
using femcae::geometry::Vec3;
using predicates::PredicateSign;
using Kind = DelaunayLocationKind;
std::size_t checks = 0;
void require(bool value, const char* message) {
    ++checks;
    if (!value) throw std::runtime_error(message);
}
template<class F> void invalid(F&& f) {
    bool caught = false;
    try { f(); } catch (const std::invalid_argument&) { caught = true; }
    require(caught, "invalid input must be rejected explicitly");
}
struct Fixture {
    std::vector<CanonicalSite> sites;
    std::vector<DelaunayCellSlot> slots;
};
std::array<DelaunayVertexRef, 3> face(const DelaunayCellRecord& cell, std::size_t f) {
    std::array<DelaunayVertexRef, 3> key;
    std::size_t j = 0;
    for (std::size_t i = 0; i < 4; ++i) if (i != f) key[j++] = cell.vertices[i];
    std::sort(key.begin(), key.end());
    return key;
}

Fixture bootstrap(double scale = 1, double height = 1) {
    Fixture f;
    f.sites = {{10, {0,0,0}, {}}, {80, {scale,0,0}, {}},
               {30, {0,scale,0}, {}}, {900, {0,0,height}, {}}};
    const auto b = buildDelaunayBootstrap(f.sites);
    f.slots.assign(b.arena.slots().begin(), b.arena.slots().end());
    return f;
}

Fixture twoCells() {
    // Bagimsiz sabit fixture: iki tetra z=0 ucgenini paylasir.
    // Production insertion/cavity helper'i degildir.
    Fixture f;
    f.sites = {{10, {0,0,0}, {}}, {80, {1,0,0}, {}}, {30, {0,1,0}, {}},
               {900, {0,0,1}, {}}, {700, {0,0,-1}, {}}};
    std::map<PointId, Vec3> points;
    for (const auto& s : f.sites) points.emplace(s.id, s.point);
    for (const auto ids : {std::array<PointId,4>{10,80,30,900},
                           std::array<PointId,4>{10,80,30,700}}) {
        DelaunayCellSlot slot;
        slot.live = true;
        for (std::size_t i = 0; i < 4; ++i) slot.record.vertices[i] = DelaunayVertexRef::finite(ids[i]);
        if (predicates::orient3d(points.at(ids[0]), points.at(ids[1]),
                                 points.at(ids[2]), points.at(ids[3])).sign == PredicateSign::Negative)
            std::swap(slot.record.vertices[0], slot.record.vertices[1]);
        f.slots.push_back(slot);
    }
    using Owner = std::pair<std::size_t, std::size_t>;
    std::map<std::array<DelaunayVertexRef,3>, std::vector<Owner>> faces;
    for (std::size_t i = 0; i < 2; ++i)
        for (std::size_t j = 0; j < 4; ++j) faces[face(f.slots[i].record,j)].push_back({i,j});
    for (const auto& [key, owners] : faces) {
        if (owners.size() != 1) continue;
        const auto [cell, opposite] = owners[0];
        DelaunayCellSlot ghost;
        ghost.live = true;
        ghost.record.vertices[0] = DelaunayVertexRef::infinite();
        for (std::size_t i = 0; i < 3; ++i) ghost.record.vertices[i+1] = key[i];
        const auto q = points.at(*f.slots[cell].record.vertices[opposite].finitePointId());
        if (predicates::orient3d(points.at(*key[0].finitePointId()),
                                points.at(*key[1].finitePointId()),
                                points.at(*key[2].finitePointId()),q).sign == PredicateSign::Positive)
            std::swap(ghost.record.vertices[1],ghost.record.vertices[2]);
        f.slots.push_back(ghost);
    }
    faces.clear();
    for (std::size_t i = 0; i < f.slots.size(); ++i) {
        f.slots[i].generation = static_cast<std::uint32_t>(i+11);
        for (std::size_t j = 0; j < 4; ++j) faces[face(f.slots[i].record,j)].push_back({i,j});
    }
    for (const auto& [key, owners] : faces) {
        (void)key;
        require(owners.size() == 2, "fixture face does not have two owners");
        for (std::size_t j = 0; j < 2; ++j) {
            const auto [a, af] = owners[j];
            const auto b = owners[1-j].first;
            f.slots[a].record.neighbors[af] = {static_cast<std::uint32_t>(b), f.slots[b].generation};
        }
    }
    require(validateDelaunayTopology(f.slots,f.sites).ok(), "two-cell fixture invalid");
    return f;
}

std::vector<std::array<DelaunayVertexRef,4>> identities(const DelaunayLocationResult& result) {
    std::vector<std::array<DelaunayVertexRef,4>> keys;
    for (const auto& cell : result.incidentCells) keys.push_back(cell.canonicalVertices);
    return keys;
}

void verifyWitness(const Fixture& f, const Vec3& query, const DelaunayLocationResult& r) {
    require(r.kind == Kind::OutsideConvexHull && r.outsideWitness.has_value(), "outside has no witness");
    require(r.entityVertices.empty() && r.incidentCells.empty(), "outside contains a fake finite identity");
    std::map<PointId, Vec3> p;
    for (const auto& s : f.sites) p.emplace(s.id,s.point);
    const auto& w = *r.outsideWitness;
    require(w.ghostCell.slot < f.slots.size() && w.finiteCell.slot < f.slots.size(), "witness handle out of bounds");
    const auto& g = f.slots[w.ghostCell.slot];
    require(g.generation == w.ghostCell.generation &&
            f.slots[w.finiteCell.slot].generation == w.finiteCell.generation, "witness generation mismatch");
    require(g.record.vertices[0].isInfinite() && g.record.neighbors[0] == w.finiteCell, "witness is not adjacent ghost");
    require(predicates::orient3d(p.at(w.outwardFace[0]),p.at(w.outwardFace[1]),
                                p.at(w.outwardFace[2]),query).sign == PredicateSign::Positive, "witness is not strictly violated");
    std::vector<std::array<PointId,3>> violated;
    for (const auto& slot : f.slots) {
        if (!slot.live || !slot.record.vertices[0].isInfinite()) continue;
        std::array<PointId,3> ids{*slot.record.vertices[1].finitePointId(),
                                *slot.record.vertices[2].finitePointId(),
                                *slot.record.vertices[3].finitePointId()};
        if (predicates::orient3d(p.at(ids[0]),p.at(ids[1]),p.at(ids[2]),query).sign == PredicateSign::Positive) {
            std::sort(ids.begin(),ids.end()); violated.push_back(ids);
        }
    }
    require(w.canonicalFace == *std::min_element(violated.begin(),violated.end()), "witness choice depends on cell order");
}

void verifyBasic() {
    auto f = bootstrap();
    const std::vector<std::pair<Vec3,Kind>> cases{
        {{0.125,0.125,0.125},Kind::Cell}, {{0.25,0.25,0},Kind::Facet},
        {{0.5,0,0},Kind::Edge}, {{-0.0,0,-0.0},Kind::Vertex},
        {{-1,-1,-1},Kind::OutsideConvexHull}, {{2,0,0},Kind::OutsideConvexHull}};
    const std::vector<std::vector<PointId>> expected{
        {10,30,80,900},{10,30,80},{10,80},{10},{},{}};
    for (std::size_t i = 0; i < cases.size(); ++i) {
        const auto r = locateBruteForceExact(f.slots,f.sites,cases[i].first);
        require(r.kind == cases[i].second && r.entityVertices == expected[i], "basic location/identity mismatch");
        require(r.finiteCellsTested == 1, "finite scan count wrong");
        if (r.kind == Kind::OutsideConvexHull) verifyWitness(f,cases[i].first,r);
        else require(!r.outsideWitness.has_value() && !r.incidentCells.empty(), "contained evidence invalid");
    }
    // Canonical sites may include not-yet-inserted points: those are not vertices.
    f.sites.push_back({12345,{0.125,0.125,0.125},{}});
    require(locateBruteForceExact(f.slots,f.sites,f.sites.back().point).kind == Kind::Cell,
            "uninserted site was incorrectly treated as a vertex");
}

void verifyTwoCellOracleAndDeterminism() {
    auto f = twoCells();
    f.slots.push_back({}); // dead slots must be ignored
    auto reversed = f;
    std::reverse(reversed.slots.begin(),reversed.slots.end());
    for (auto& slot : reversed.slots) {
        if (!slot.live) continue;
        for (auto& n : slot.record.neighbors)
            n.slot = static_cast<std::uint32_t>(f.slots.size()-1-n.slot);
    }
    std::reverse(reversed.sites.begin(),reversed.sites.end());
    const auto before = f.slots;
    // Oracle: x>=0, y>=0, x+y+abs(z)<=1. Integer grid has no roundoff.
    for (int i = -1; i <= 5; ++i) for (int j = -1; j <= 5; ++j) for (int k = -5; k <= 5; ++k) {
        const Vec3 q{i/4.0,j/4.0,k/4.0};
        const auto r = locateBruteForceExact(f.slots,f.sites,q);
        const auto rr = locateBruteForceExact(reversed.slots,reversed.sites,q);
        const bool inside = i>=0 && j>=0 && i+j+std::abs(k)<=4;
        if (!inside) verifyWitness(f,q,r);
        else {
            const std::array<int,4> weights{i,j,std::abs(k),4-i-j-std::abs(k)};
            const auto zero = std::count(weights.begin(),weights.end(),0);
            const std::array<Kind,4> kinds{Kind::Cell,Kind::Facet,Kind::Edge,Kind::Vertex};
            require(r.kind == kinds[static_cast<std::size_t>(zero)], "exact integer barycentric oracle disagrees");
        }
        require(r.finiteCellsTested == 2 && rr.finiteCellsTested == 2, "oracle failed to scan every finite cell");
        require(r.kind == rr.kind && r.entityVertices == rr.entityVertices && identities(r)==identities(rr), "cell/input reversal changed canonical result");
        if (!inside) require(r.outsideWitness->canonicalFace==rr.outsideWitness->canonicalFace, "reversal changed hull witness");
        for (const auto& e : rr.incidentCells) {
            auto key = reversed.slots[e.handle.slot].record.vertices;
            std::sort(key.begin(),key.end());
            require(key == e.canonicalVertices && reversed.slots[e.handle.slot].generation==e.handle.generation, "incident evidence points at wrong cell");
        }
    }
    const auto shared = locateBruteForceExact(f.slots,f.sites,{0.25,0.25,0});
    require(shared.kind==Kind::Facet && shared.incidentCells.size()==2, "shared facet lost incident cells");
    const auto edge = locateBruteForceExact(f.slots,f.sites,{0.5,0,0});
    require(edge.kind==Kind::Edge && edge.incidentCells.size()==4, "hull edge lost unified star evidence");
    for (std::size_t i=0;i<before.size();++i)
        require(before[i].live==f.slots[i].live && before[i].generation==f.slots[i].generation &&
                before[i].record.vertices==f.slots[i].record.vertices && before[i].record.neighbors==f.slots[i].record.neighbors &&
                before[i].record.visitEpoch==f.slots[i].record.visitEpoch, "location mutated snapshot");
}

void verifyRangeAndRejection() {
    for (int exponent : {-500,0,500}) {
        const double s = std::ldexp(1.0,exponent);
        const auto f = bootstrap(s,s);
        require(locateBruteForceExact(f.slots,f.sites,{s/8,s/8,s/8}).kind==Kind::Cell, "binary scale changed interior decision");
        require(locateBruteForceExact(f.slots,f.sites,{s/4,s/4,0}).kind==Kind::Facet, "binary scale changed boundary decision");
    }
    const auto f = bootstrap();
    for (double z : {std::nextafter(0.0,1.0),std::nextafter(0.0,-1.0)}) {
        const auto r = locateBruteForceExact(f.slots,f.sites,{0.25,0.25,z});
        require(r.kind == (z>0 ? Kind::Cell : Kind::OutsideConvexHull), "one-ulp side was collapsed into a face");
    }
    const double h=std::ldexp(1.0,-1000);
    const auto thin = bootstrap(1,h);
    require(locateBruteForceExact(thin.slots,thin.sites,{0.125,0.125,h/4}).kind==Kind::Cell, "near-coplanar exact nonzero cell lost");
    invalid([&]{(void)locateBruteForceExact(f.slots,f.sites,{std::numeric_limits<double>::infinity(),0,0});});
    invalid([&]{(void)locateBruteForceExact(f.slots,f.sites,{0,std::numeric_limits<double>::quiet_NaN(),0});});
    invalid([&]{(void)locateBruteForceExact({},f.sites,{0,0,0});});
    auto stale=f; stale.slots[0].record.neighbors[0].generation++;
    invalid([&]{(void)locateBruteForceExact(stale.slots,stale.sites,{0,0,0});});
    auto duplicate=f;duplicate.sites.push_back({999,{-0.0,0,0},{}});
    invalid([&]{(void)locateBruteForceExact(duplicate.slots,duplicate.sites,{0,0,0});});
    auto missing=f;missing.sites.pop_back();
    invalid([&]{(void)locateBruteForceExact(missing.slots,missing.sites,{0,0,0});});
}
} // namespace
int main() {
    try {
        verifyBasic(); verifyTwoCellOracleAndDeterminism(); verifyRangeAndRejection();
        std::cout << "M2.1-C exact location PASS checks=" << checks << " grid_queries=539 kinds=5\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "M2.1-C exact location FAIL: " << e.what() << '\n'; return 1;
    }
}
