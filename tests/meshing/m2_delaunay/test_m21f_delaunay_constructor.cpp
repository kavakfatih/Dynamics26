#include "meshing/m2/DelaunayConstructor.h"

#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <array>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace {

using namespace femcae::meshing;
using namespace femcae::meshing::m2;
using femcae::geometry::Vec3;
using predicates::PredicateSign;

std::uint64_t checks = 0U;

struct QualificationStats {
    std::uint64_t constructorFixtures{0};
    std::uint64_t successfulBuilds{0};
    std::uint64_t expectedFailedBuilds{0};

    std::uint64_t fiveSitePermutations{0};
    std::uint64_t fiveSiteMismatches{0};
    std::uint64_t cubePermutations{0};
    std::uint64_t cubeMismatches{0};
    std::uint64_t fingerprintMismatches{0};

    std::uint64_t globalOracleSets{0};
    std::uint64_t globalTetSiteTests{0};
    std::uint64_t globalOracleFailures{0};

    std::uint64_t localFacetsChecked{0};
    std::uint64_t localLegalityFailures{0};
    std::uint64_t symbolicTiesChecked{0};
    std::uint64_t symbolicTieFailures{0};

    std::uint64_t s3StatesChecked{0};
    std::uint64_t s3Failures{0};

    std::uint64_t hullFacesChecked{0};
    std::uint64_t hullSupportViolations{0};
    std::uint64_t coplanarHullEdgesChecked{0};
    std::uint64_t hullSymbolicTiesChecked{0};
    std::uint64_t hullSymbolicFailures{0};

    std::uint64_t validatorStates{0};
    std::uint64_t resourceCases{0};
    std::uint64_t metamorphicCases{0};
    std::uint64_t metamorphicFailures{0};
    std::uint64_t inputEnumerationCases{0};
};

QualificationStats stats;

void require(bool value, const std::string& message) {
    ++checks;
    if (!value) {
        throw std::runtime_error(message);
    }
}

InputSite input(
    double x,
    double y,
    double z,
    std::uint64_t sourceId) {
    return {{x, y, z}, sourceId};
}

std::vector<InputSite> fiveSiteFixture() {
    return {
        input(0.0, 0.0, 0.0, 101U),
        input(0.0, 0.0, 1.0, 102U),
        input(0.0, 1.0, 0.0, 103U),
        input(1.0, 0.0, 0.0, 104U),
        input(1.0, 1.0, 1.0, 105U)};
}

std::vector<InputSite> cubeFixture() {
    return {
        input(0.0, 0.0, 0.0, 201U),
        input(0.0, 0.0, 1.0, 202U),
        input(0.0, 1.0, 0.0, 203U),
        input(0.0, 1.0, 1.0, 204U),
        input(1.0, 0.0, 0.0, 205U),
        input(1.0, 0.0, 1.0, 206U),
        input(1.0, 1.0, 0.0, 207U),
        input(1.0, 1.0, 1.0, 208U)};
}

std::vector<InputSite> interiorGoldenFixture() {
    // D26SITE1 canonical IDs:
    // 1=(0,0,0), 2=(0,0,4), 3=(0,4,0), 4=(1,1,1), 5=(4,0,0).
    // Site 4 is strictly inside the outer tetra 1-2-3-5.
    return {
        input(0.0, 0.0, 0.0, 301U),
        input(0.0, 0.0, 4.0, 302U),
        input(0.0, 4.0, 0.0, 303U),
        input(1.0, 1.0, 1.0, 304U),
        input(4.0, 0.0, 0.0, 305U)};
}

const std::vector<std::array<PointId, 4>> FiveFinite{{
    {1U, 2U, 3U, 4U},
    {2U, 3U, 4U, 5U},
}};

const std::vector<std::array<PointId, 3>> FiveHull{{
    {1U, 2U, 3U},
    {1U, 2U, 4U},
    {1U, 3U, 4U},
    {2U, 3U, 5U},
    {2U, 4U, 5U},
    {3U, 4U, 5U},
}};

const std::vector<std::array<PointId, 4>> CubeFinite{{
    {1U, 2U, 3U, 5U},
    {2U, 3U, 4U, 5U},
    {2U, 4U, 5U, 6U},
    {3U, 4U, 5U, 7U},
    {4U, 5U, 6U, 7U},
    {4U, 6U, 7U, 8U},
}};

const std::vector<std::array<PointId, 3>> CubeHull{{
    {1U, 2U, 3U},
    {1U, 2U, 5U},
    {1U, 3U, 5U},
    {2U, 3U, 4U},
    {2U, 4U, 6U},
    {2U, 5U, 6U},
    {3U, 4U, 7U},
    {3U, 5U, 7U},
    {4U, 6U, 8U},
    {4U, 7U, 8U},
    {5U, 6U, 7U},
    {6U, 7U, 8U},
}};

const std::vector<std::array<PointId, 4>> InteriorFinite{{
    {1U, 2U, 3U, 4U},
    {1U, 2U, 4U, 5U},
    {1U, 3U, 4U, 5U},
    {2U, 3U, 4U, 5U},
}};

const std::vector<std::array<PointId, 3>> InteriorHull{{
    {1U, 2U, 3U},
    {1U, 2U, 5U},
    {1U, 3U, 5U},
    {2U, 3U, 5U},
}};

std::vector<std::array<PointId, 3>> boundaryFromFinite(
    const std::vector<std::array<PointId, 4>>& tets) {
    std::map<std::array<PointId, 3>, std::size_t> count;
    for (const auto& tet : tets) {
        for (std::size_t opposite = 0U;
             opposite < 4U; ++opposite) {
            std::array<PointId, 3> face{};
            std::size_t write = 0U;
            for (std::size_t i = 0U; i < 4U; ++i) {
                if (i != opposite) {
                    face[write++] = tet[i];
                }
            }
            std::sort(face.begin(), face.end());
            ++count[face];
        }
    }
    std::vector<std::array<PointId, 3>> result;
    for (const auto& [face, owners] : count) {
        if (owners == 1U) {
            result.push_back(face);
        }
    }
    return result;
}

std::uint64_t canonicalBits(double value) {
    std::uint64_t bits =
        std::bit_cast<std::uint64_t>(value);
    if ((bits & 0x7FFFFFFFFFFFFFFFULL) == 0ULL) {
        bits = 0ULL;
    }
    return bits;
}

std::string hex64(std::uint64_t value) {
    std::ostringstream out;
    out << std::hex << std::setfill('0')
        << std::setw(16) << value;
    return out.str();
}

std::string independentExpectedRecord(
    const std::vector<InputSite>& raw,
    const std::vector<std::array<PointId, 4>>& finite,
    const std::vector<std::array<PointId, 3>>& hull) {
    const auto canonical = canonicalizeSites(raw);
    std::ostringstream out;
    out << "D26DT1\n"
        << "site_policy=D26SITE1\n"
        << "symbolic_policy=D26LIFT1\n"
        << "fingerprint_schema=D26DT1\n"
        << "canonical_sites=" << canonical.size() << "\n"
        << "finite_tets=" << finite.size() << "\n"
        << "hull_facets=" << hull.size() << "\n";
    for (const auto& site : canonical) {
        out << "site " << site.id << ' '
            << hex64(canonicalBits(site.point.x)) << ' '
            << hex64(canonicalBits(site.point.y)) << ' '
            << hex64(canonicalBits(site.point.z)) << "\n";
    }
    for (const auto& tet : finite) {
        out << "tet " << tet[0] << ' ' << tet[1]
            << ' ' << tet[2] << ' ' << tet[3] << "\n";
    }
    for (const auto& face : hull) {
        out << "hull " << face[0] << ' '
            << face[1] << ' ' << face[2] << "\n";
    }
    return out.str();
}

std::map<PointId, Vec3> pointMap(
    const DelaunayConstructorResult& result) {
    std::map<PointId, Vec3> points;
    for (const auto& site : result.canonicalSites) {
        points.emplace(site.id, site.point);
    }
    return points;
}

bool finiteCell(const DelaunayCellRecord& cell) {
    return std::all_of(
        cell.vertices.begin(),
        cell.vertices.end(),
        [](const auto& v) { return v.isFinite(); });
}

bool ghostCell(const DelaunayCellRecord& cell) {
    return cell.vertices[0].isInfinite() &&
           cell.vertices[1].isFinite() &&
           cell.vertices[2].isFinite() &&
           cell.vertices[3].isFinite();
}

std::array<IndexedPoint3, 4> indexedTet(
    const DelaunayCellRecord& cell,
    const std::map<PointId, Vec3>& points) {
    std::array<IndexedPoint3, 4> tet{};
    for (std::size_t i = 0U; i < 4U; ++i) {
        const PointId id =
            *cell.vertices[i].finitePointId();
        tet[i] = {id, points.at(id)};
    }
    return tet;
}

void checkGolden(
    const DelaunayConstructorResult& result,
    const std::vector<std::array<PointId, 4>>& finite,
    const std::vector<std::array<PointId, 3>>& hull,
    const std::string& expectedRecord,
    const char* name) {
    require(result.ok(), std::string(name) + " constructor failed");
    require(
        result.finalFingerprint->sitePolicy == D26SitePolicyId &&
        result.finalFingerprint->symbolicPolicy == D26SymbolicPolicyId &&
        result.finalFingerprint->schema == D26FingerprintSchemaId,
        std::string(name) + " fingerprint policy IDs mismatch");
    require(
        result.finalFingerprint->finiteTets == finite,
        std::string(name) + " finite golden mismatch");
    require(
        result.finalFingerprint->hullFacets == hull,
        std::string(name) + " hull golden mismatch");
    require(
        result.finalFingerprint->canonicalRecord == expectedRecord,
        std::string(name) + " D26DT1 canonical record mismatch");
}

void accumulateValidatorEvidence(
    const DelaunayConstructorResult& result) {
    stats.validatorStates +=
        static_cast<std::uint64_t>(
            result.validatedStates.size());
    require(
        result.validatedStates.size() ==
            result.completedInsertions + 1U,
        "successful constructor must validate bootstrap and every committed insertion state");
}

void checkGlobalWeakDelaunay(
    const DelaunayConstructorResult& result) {
    require(result.ok(), "global oracle requires successful constructor");
    ++stats.globalOracleSets;
    const auto points = pointMap(result);

    for (const auto& slot : result.arena.slots()) {
        if (!slot.live || !finiteCell(slot.record)) {
            continue;
        }
        const auto tet = indexedTet(slot.record, points);
        require(
            predicates::orient3d(
                tet[0].point, tet[1].point,
                tet[2].point, tet[3].point).sign ==
                PredicateSign::Positive,
            "global oracle found non-positive finite tetra");

        std::set<PointId> ids{
            tet[0].id, tet[1].id, tet[2].id, tet[3].id};
        for (const auto& site : result.canonicalSites) {
            if (ids.contains(site.id)) {
                continue;
            }
            ++stats.globalTetSiteTests;
            const PredicateSign raw =
                predicates::insphere(
                    tet[0].point,
                    tet[1].point,
                    tet[2].point,
                    tet[3].point,
                    site.point).sign;
            if (raw == PredicateSign::Positive) {
                ++stats.globalOracleFailures;
                throw std::runtime_error(
                    "global raw weak-Delaunay oracle found strict circumsphere interior violation");
            }
        }
    }
}

struct FiniteFaceOwner {
    const DelaunayCellRecord* cell{nullptr};
    std::size_t opposite{0U};
};

std::array<PointId, 3> finiteFaceKey(
    const DelaunayCellRecord& cell,
    std::size_t opposite) {
    std::array<PointId, 3> face{};
    std::size_t write = 0U;
    for (std::size_t i = 0U; i < 4U; ++i) {
        if (i != opposite) {
            face[write++] =
                *cell.vertices[i].finitePointId();
        }
    }
    std::sort(face.begin(), face.end());
    return face;
}

void checkLocalFiniteLegality(
    const DelaunayConstructorResult& result) {
    require(result.ok(), "local legality requires successful constructor");
    const auto points = pointMap(result);
    std::map<std::array<PointId, 3>,
             std::vector<FiniteFaceOwner>> faces;

    for (const auto& slot : result.arena.slots()) {
        if (!slot.live || !finiteCell(slot.record)) {
            continue;
        }
        for (std::size_t opposite = 0U;
             opposite < 4U; ++opposite) {
            faces[finiteFaceKey(
                slot.record, opposite)].push_back(
                    {&slot.record, opposite});
        }
    }

    for (const auto& [face, owners] : faces) {
        (void)face;
        if (owners.size() != 2U) {
            continue;
        }
        ++stats.localFacetsChecked;

        for (std::size_t side = 0U; side < 2U; ++side) {
            const auto& owner = owners[side];
            const auto& other = owners[1U - side];
            const PointId oppositeId =
                *other.cell->vertices[other.opposite]
                     .finitePointId();
            const auto tet =
                indexedTet(*owner.cell, points);
            const PredicateSign raw =
                predicates::insphere(
                    tet[0].point,
                    tet[1].point,
                    tet[2].point,
                    tet[3].point,
                    points.at(oppositeId)).sign;
            if (raw == PredicateSign::Positive) {
                ++stats.localLegalityFailures;
                throw std::runtime_error(
                    "internal finite facet violates raw weak local Delaunay legality");
            }
            if (raw == PredicateSign::Zero) {
                ++stats.symbolicTiesChecked;
                const ResolvedDelaunaySign resolved =
                    resolveLiftOnlyInsphere({
                        tet[0], tet[1], tet[2], tet[3],
                        {oppositeId, points.at(oppositeId)}});
                if (resolved.resolvedSign ==
                    PredicateSign::Positive) {
                    ++stats.symbolicTieFailures;
                    throw std::runtime_error(
                        "exact internal finite tie violates D26LIFT1 canonical local legality");
                }
            }
        }
    }
}

std::array<PointId, 2> edgeKey(PointId a, PointId b) {
    std::array<PointId, 2> edge{a, b};
    std::sort(edge.begin(), edge.end());
    return edge;
}

void checkHullOracle(
    const DelaunayConstructorResult& result) {
    require(result.ok(), "hull oracle requires successful constructor");
    const auto points = pointMap(result);

    std::vector<std::array<PointId, 3>> hull;
    for (const auto& slot : result.arena.slots()) {
        if (!slot.live || !ghostCell(slot.record)) {
            continue;
        }
        const std::array<PointId, 3> outward{
            *slot.record.vertices[1].finitePointId(),
            *slot.record.vertices[2].finitePointId(),
            *slot.record.vertices[3].finitePointId()};
        ++stats.hullFacesChecked;
        for (const auto& site : result.canonicalSites) {
            if (site.id == outward[0] ||
                site.id == outward[1] ||
                site.id == outward[2]) {
                continue;
            }
            const PredicateSign side =
                predicates::orient3d(
                    points.at(outward[0]),
                    points.at(outward[1]),
                    points.at(outward[2]),
                    site.point).sign;
            if (side == PredicateSign::Positive) {
                ++stats.hullSupportViolations;
                throw std::runtime_error(
                    "hull supporting-plane oracle found an exterior finite site");
            }
        }

        auto canonical = outward;
        std::sort(canonical.begin(), canonical.end());
        hull.push_back(canonical);
    }

    std::map<std::array<PointId, 2>,
             std::vector<PointId>> edgeOpposites;
    for (const auto& tri : hull) {
        edgeOpposites[edgeKey(tri[0], tri[1])].push_back(tri[2]);
        edgeOpposites[edgeKey(tri[0], tri[2])].push_back(tri[1]);
        edgeOpposites[edgeKey(tri[1], tri[2])].push_back(tri[0]);
    }

    for (const auto& [edge, opposite] : edgeOpposites) {
        require(
            opposite.size() == 2U,
            "closed hull edge must have exactly two triangles");
        if (opposite[0] == opposite[1]) {
            throw std::runtime_error(
                "hull edge has duplicate opposite vertex");
        }

        const std::array<IndexedPoint3, 3> triangle{{
            {edge[0], points.at(edge[0])},
            {edge[1], points.at(edge[1])},
            {opposite[0], points.at(opposite[0])}}};
        const IndexedPoint3 query{
            opposite[1], points.at(opposite[1])};

        if (predicates::orient3d(
                triangle[0].point,
                triangle[1].point,
                triangle[2].point,
                query.point).sign != PredicateSign::Zero) {
            continue;
        }

        ++stats.coplanarHullEdgesChecked;
        const ResolvedDelaunaySign circle =
            classifyProjectedCoplanarCircumcircle(
                triangle, query);
        if (circle.geometricSign ==
            PredicateSign::Positive) {
            ++stats.hullSymbolicFailures;
            throw std::runtime_error(
                "coplanar hull diagonal violates raw weak circumcircle legality");
        }
        if (circle.geometricSign == PredicateSign::Zero) {
            ++stats.hullSymbolicTiesChecked;
            if (circle.resolvedSign ==
                PredicateSign::Positive) {
                ++stats.hullSymbolicFailures;
                throw std::runtime_error(
                    "coplanar hull exact tie violates D26LIFT1 diagonal legality");
            }
        }
    }
}

void checkS3(
    const DelaunayConstructorResult& result,
    std::optional<std::array<std::size_t, 4>> golden = std::nullopt) {
    require(result.ok(), "S3 oracle requires successful constructor");
    ++stats.s3StatesChecked;
    const auto complex =
        computeDelaunayComplexStats(result.arena.slots());

    if (complex.eulerCharacteristic != 0 ||
        4U * complex.cells != 2U * complex.faces ||
        complex.faces != 2U * complex.cells ||
        complex.edges != complex.vertices + complex.cells) {
        ++stats.s3Failures;
        throw std::runtime_error(
            "unified finite+ghost S3 Euler/incidence formula failed");
    }

    std::map<DelaunayFaceKey, std::size_t> faceOwners;
    for (const auto& slot : result.arena.slots()) {
        if (!slot.live) {
            continue;
        }
        for (std::size_t face = 0U; face < 4U; ++face) {
            ++faceOwners[
                canonicalDelaunayFaceKey(
                    slot.record, face)];
        }
    }
    for (const auto& [face, owners] : faceOwners) {
        (void)face;
        if (owners != 2U) {
            ++stats.s3Failures;
            throw std::runtime_error(
                "unified facet does not have exactly two live owners");
        }
    }

    std::set<PointId> hullVertices;
    std::set<std::array<PointId, 2>> hullEdges;
    for (const auto& tri :
         result.finalFingerprint->hullFacets) {
        hullVertices.insert(tri.begin(), tri.end());
        hullEdges.insert(edgeKey(tri[0], tri[1]));
        hullEdges.insert(edgeKey(tri[0], tri[2]));
        hullEdges.insert(edgeKey(tri[1], tri[2]));
    }

    const std::size_t vh = hullVertices.size();
    const std::size_t h =
        result.finalFingerprint->hullFacets.size();
    const std::size_t eh = hullEdges.size();
    require(vh >= 4U, "3D hull must have at least four vertices");
    if (h != 2U * vh - 4U ||
        eh != 3U * vh - 6U ||
        complex.ghostCells != h) {
        ++stats.s3Failures;
        throw std::runtime_error(
            "hull triangulated 2-sphere count formula failed");
    }

    if (golden.has_value()) {
        const auto [vertices, edges, faces, cells] =
            *golden;
        require(
            complex.vertices == vertices &&
            complex.edges == edges &&
            complex.faces == faces &&
            complex.cells == cells,
            "frozen global S3 counts mismatch");
    }
}

std::uint64_t factorial(std::uint64_t n) {
    std::uint64_t value = 1U;
    for (std::uint64_t i = 2U; i <= n; ++i) {
        require(
            value <=
                std::numeric_limits<std::uint64_t>::max() / i,
            "factorial counter overflow");
        value *= i;
    }
    return value;
}

std::vector<PointId> ascendingIds(std::size_t count) {
    std::vector<PointId> ids;
    ids.reserve(count);
    for (std::size_t i = 0U; i < count; ++i) {
        ids.push_back(static_cast<PointId>(i + 1U));
    }
    return ids;
}

DelaunayConstructorResult buildWithOrder(
    const std::vector<InputSite>& raw,
    const std::vector<PointId>& order,
    bool telemetry = false,
    bool replay = false,
    std::size_t maxSlots =
        static_cast<std::size_t>(
            std::numeric_limits<std::uint32_t>::max())) {
    DelaunayConstructorOptions options;
    options.fullInsertionOrder = order;
    options.telemetryEnabled = telemetry;
    options.captureReplayDecisions = replay;
    options.compareBruteForceLocation = true;
    options.resourceLimits.maxTotalSlots = maxSlots;
    ++stats.constructorFixtures;
    auto result = constructDelaunayReference(raw, options);
    if (result.ok()) {
        ++stats.successfulBuilds;
    }
    return result;
}

void testGoldenDerivations() {
    require(
        boundaryFromFinite(FiveFinite) == FiveHull,
        "five-site frozen hull is inconsistent with two-tetra golden");
    require(
        boundaryFromFinite(CubeFinite) == CubeHull,
        "cube frozen hull is inconsistent with six-tetra golden");
    require(
        boundaryFromFinite(InteriorFinite) == InteriorHull,
        "interior analytic hull derivation mismatch");
}

struct GoldenManifests {
    std::string fiveRecord;
    std::string cubeRecord;
    std::string interiorRecord;
    std::string fiveDigest;
    std::string cubeDigest;
    std::string interiorDigest;
};

GoldenManifests testGoldenReferenceBuilds() {
    GoldenManifests manifest;
    const auto fiveRaw = fiveSiteFixture();
    const auto cubeRaw = cubeFixture();
    const auto interiorRaw = interiorGoldenFixture();

    manifest.fiveRecord =
        independentExpectedRecord(
            fiveRaw, FiveFinite, FiveHull);
    manifest.cubeRecord =
        independentExpectedRecord(
            cubeRaw, CubeFinite, CubeHull);
    manifest.interiorRecord =
        independentExpectedRecord(
            interiorRaw, InteriorFinite, InteriorHull);

    const auto five =
        buildWithOrder(
            fiveRaw, ascendingIds(5U), true);
    checkGolden(
        five, FiveFinite, FiveHull,
        manifest.fiveRecord, "five-site");
    accumulateValidatorEvidence(five);
    checkGlobalWeakDelaunay(five);
    checkLocalFiniteLegality(five);
    checkHullOracle(five);
    checkS3(five, std::array<std::size_t, 4>{
        6U, 14U, 16U, 8U});
    manifest.fiveDigest =
        five.finalFingerprint->digestHex;

    const auto cube =
        buildWithOrder(
            cubeRaw, ascendingIds(8U), true);
    checkGolden(
        cube, CubeFinite, CubeHull,
        manifest.cubeRecord, "cube");
    accumulateValidatorEvidence(cube);
    checkGlobalWeakDelaunay(cube);
    checkLocalFiniteLegality(cube);
    checkHullOracle(cube);
    checkS3(cube, std::array<std::size_t, 4>{
        9U, 27U, 36U, 18U});
    manifest.cubeDigest =
        cube.finalFingerprint->digestHex;

    const auto interior =
        buildWithOrder(
            interiorRaw, ascendingIds(5U), true);
    checkGolden(
        interior, InteriorFinite, InteriorHull,
        manifest.interiorRecord, "interior");
    accumulateValidatorEvidence(interior);
    checkGlobalWeakDelaunay(interior);
    checkLocalFiniteLegality(interior);
    checkHullOracle(interior);
    checkS3(interior);
    manifest.interiorDigest =
        interior.finalFingerprint->digestHex;

    return manifest;
}

void testExhaustiveFive(
    const GoldenManifests& manifest) {
    auto permutation = ascendingIds(5U);
    const std::uint64_t expected = factorial(5U);
    do {
        const auto result =
            buildWithOrder(
                fiveSiteFixture(),
                permutation,
                false);
        ++stats.fiveSitePermutations;
        if (!result.ok() ||
            result.finalFingerprint->canonicalRecord !=
                manifest.fiveRecord ||
            result.finalFingerprint->finiteTets !=
                FiveFinite ||
            result.finalFingerprint->hullFacets !=
                FiveHull) {
            ++stats.fiveSiteMismatches;
            ++stats.fingerprintMismatches;
            std::ostringstream error;
            error << "five-site permutation "
                  << stats.fiveSitePermutations
                  << " mismatch/status="
                  << delaunayConstructorStatusName(
                         result.status);
            throw std::runtime_error(error.str());
        }
        const auto complex =
            computeDelaunayComplexStats(
                result.arena.slots());
        require(
            complex.vertices == 6U &&
            complex.edges == 14U &&
            complex.faces == 16U &&
            complex.cells == 8U &&
            complex.finiteCells == 2U &&
            complex.ghostCells == 6U &&
            complex.eulerCharacteristic == 0,
            "five-site permutation frozen global counts mismatch");
        accumulateValidatorEvidence(result);
    } while (std::next_permutation(
        permutation.begin(), permutation.end()));

    require(
        stats.fiveSitePermutations == expected &&
        expected == 120U,
        "five-site exhaustive permutation count is not 120");
}

void testExhaustiveCube(
    const GoldenManifests& manifest) {
    auto permutation = ascendingIds(8U);
    const std::uint64_t expected = factorial(8U);
    do {
        const auto result =
            buildWithOrder(
                cubeFixture(),
                permutation,
                false);
        ++stats.cubePermutations;
        if (!result.ok() ||
            result.finalFingerprint->canonicalRecord !=
                manifest.cubeRecord ||
            result.finalFingerprint->finiteTets !=
                CubeFinite ||
            result.finalFingerprint->hullFacets !=
                CubeHull) {
            ++stats.cubeMismatches;
            ++stats.fingerprintMismatches;
            std::ostringstream error;
            error << "cube permutation "
                  << stats.cubePermutations
                  << " mismatch/status="
                  << delaunayConstructorStatusName(
                         result.status);
            throw std::runtime_error(error.str());
        }
        const auto complex =
            computeDelaunayComplexStats(
                result.arena.slots());
        require(
            complex.vertices == 9U &&
            complex.edges == 27U &&
            complex.faces == 36U &&
            complex.cells == 18U &&
            complex.finiteCells == 6U &&
            complex.ghostCells == 12U &&
            complex.eulerCharacteristic == 0,
            "cube permutation frozen global counts mismatch");
        accumulateValidatorEvidence(result);
    } while (std::next_permutation(
        permutation.begin(), permutation.end()));

    require(
        stats.cubePermutations == expected &&
        expected == 40320U,
        "cube exhaustive permutation count is not 40320");
}

std::vector<InputSite> generatedIntegerCloud(
    std::size_t count,
    std::uint64_t seed) {
    std::vector<InputSite> raw;
    raw.reserve(count);
    std::uint64_t state = seed;
    for (std::size_t i = 0U; i < count; ++i) {
        state = state * 6364136223846793005ULL +
                1442695040888963407ULL;
        const int x =
            static_cast<int>((state >> 16U) % 29U) - 14;
        state = state * 6364136223846793005ULL +
                1442695040888963407ULL;
        const int y =
            static_cast<int>((state >> 20U) % 31U) - 15;
        state = state * 6364136223846793005ULL +
                1442695040888963407ULL;
        const int z =
            static_cast<int>((state >> 24U) % 37U) - 18;
        raw.push_back(input(
            static_cast<double>(x),
            static_cast<double>(y),
            static_cast<double>(z),
            1000U + i));
    }
    return raw;
}

std::vector<std::vector<InputSite>> smallOracleCorpus() {
    auto nearSphere = fiveSiteFixture();
    nearSphere.back().point.z =
        std::nextafter(1.0, 2.0);

    std::vector<InputSite> hullExpansion{
        input(0.0, 0.0, 0.0, 401U),
        input(0.0, 0.0, 2.0, 402U),
        input(0.0, 2.0, 0.0, 403U),
        input(2.0, 0.0, 0.0, 404U),
        input(3.0, 3.0, 3.0, 405U),
        input(1.0, 1.0, 0.5, 406U)};

    std::vector<InputSite> thin{
        input(0.0, 0.0, 0.0, 501U),
        input(4.0, 0.0, 0.0, 502U),
        input(0.0, 4.0, 0.0, 503U),
        input(0.0, 0.0, 0x1p-20, 504U),
        input(1.0, 1.0, 0x1p-22, 505U),
        input(2.0, 1.0, 0x1p-21, 506U)};

    return {
        interiorGoldenFixture(),
        fiveSiteFixture(),
        cubeFixture(),
        std::move(nearSphere),
        std::move(hullExpansion),
        std::move(thin),
        generatedIntegerCloud(8U, 26090701ULL),
        generatedIntegerCloud(9U, 26090702ULL),
        generatedIntegerCloud(10U, 26090703ULL)};
}

void testSmallNOracles() {
    for (const auto& raw : smallOracleCorpus()) {
        const auto canonical = canonicalizeSites(raw);
        require(
            classifyAffineDimension(canonical) ==
                AffineDimension::Three,
            "generated small-N fixture is unexpectedly lower-dimensional");
        const auto result =
            buildWithOrder(
                raw,
                ascendingIds(canonical.size()),
                true);
        require(
            result.ok(),
            std::string("small-N constructor failed: ") +
                delaunayConstructorStatusName(result.status) +
                " / " + result.detail);
        accumulateValidatorEvidence(result);
        checkGlobalWeakDelaunay(result);
        checkLocalFiniteLegality(result);
        checkHullOracle(result);
        checkS3(result);
    }
}

void testLowerDimensionalAndOrderFailures() {
    const std::vector<std::vector<InputSite>> lower{
        {},
        {input(0.0, 0.0, 0.0, 1U)},
        {input(0.0, 0.0, 0.0, 1U),
         input(1.0, 0.0, 0.0, 2U),
         input(2.0, 0.0, 0.0, 3U)},
        {input(0.0, 0.0, 0.0, 1U),
         input(1.0, 0.0, 0.0, 2U),
         input(0.0, 1.0, 0.0, 3U),
         input(1.0, 1.0, 0.0, 4U)}
    };

    for (const auto& raw : lower) {
        ++stats.constructorFixtures;
        const auto result =
            constructDelaunayReference(raw);
        require(
            result.status ==
                DelaunayConstructorStatus::LowerDimensional,
            "lower-dimensional input did not return explicit typed result");
        require(
            !result.finalFingerprint.has_value(),
            "lower-dimensional input advertised a successful final fingerprint");
    }

    const auto raw = fiveSiteFixture();
    for (const auto& order :
         std::vector<std::vector<PointId>>{
             {1U, 2U, 3U, 4U},
             {1U, 2U, 3U, 4U, 4U},
             {1U, 2U, 3U, 4U, 99U}}) {
        ++stats.constructorFixtures;
        DelaunayConstructorOptions options;
        options.fullInsertionOrder = order;
        const auto result =
            constructDelaunayReference(raw, options);
        require(
            result.status ==
                DelaunayConstructorStatus::InvalidInsertionOrder,
            "invalid full insertion order was silently repaired");
    }
}

std::vector<InputSite> twoSkewLines(std::size_t perLine) {
    std::vector<InputSite> raw;
    raw.reserve(perLine * 2U);
    for (std::size_t i = 0U; i < perLine; ++i) {
        raw.push_back(input(
            static_cast<double>(i),
            0.0,
            0.0,
            6000U + i));
    }
    for (std::size_t i = 0U; i < perLine; ++i) {
        raw.push_back(input(
            0.0,
            static_cast<double>(i),
            1.0,
            7000U + i));
    }
    return raw;
}

struct ComplexityEvidence {
    std::size_t sites{0};
    std::size_t finite{0};
    std::size_t ghost{0};
    std::size_t slots{0};
    std::string digest;
};

std::vector<ComplexityEvidence> testComplexityAndResource(
    std::string* replayPath) {
    std::vector<ComplexityEvidence> evidence;

    for (std::size_t perLine : {4U, 6U, 8U}) {
        ++stats.resourceCases;
        const auto raw = twoSkewLines(perLine);
        const auto canonical = canonicalizeSites(raw);
        const auto result =
            buildWithOrder(
                raw,
                ascendingIds(canonical.size()),
                true);
        require(
            result.ok(),
            std::string("two-skew-lines constructor failed: ") +
                delaunayConstructorStatusName(result.status) +
                " / " + result.detail);
        accumulateValidatorEvidence(result);
        checkGlobalWeakDelaunay(result);
        checkLocalFiniteLegality(result);
        checkHullOracle(result);
        checkS3(result);

        evidence.push_back({
            canonical.size(),
            result.finalFingerprint->finiteTets.size(),
            result.finalFingerprint->hullFacets.size(),
            result.arena.slots().size(),
            result.finalFingerprint->digestHex});
    }

    require(evidence.size() == 3U, "complexity evidence cardinality mismatch");
    require(
        evidence[1].finite * evidence[0].sites >
            evidence[0].finite * evidence[1].sites &&
        evidence[2].finite * evidence[1].sites >
            evidence[1].finite * evidence[2].sites,
        "two-skew-lines family did not demonstrate increasing finite-tetra/site growth");

    ++stats.resourceCases;
    const auto resourceRaw = twoSkewLines(6U);
    const auto canonical = canonicalizeSites(resourceRaw);
    auto failure =
        buildWithOrder(
            resourceRaw,
            ascendingIds(canonical.size()),
            true,
            true,
            5U);
    require(
        failure.status ==
            DelaunayConstructorStatus::ResourceLimit,
        "configured low budget did not return typed ResourceLimit");
    require(
        !failure.finalFingerprint.has_value() &&
            failure.partialFingerprint.has_value(),
        "resource failure advertised final success or lost diagnostic partial fingerprint");
    require(
        failure.failurePointId.has_value() &&
            failure.failureInsertionIndex.has_value(),
        "resource failure did not identify failing insertion");
    require(
        failure.replayRecord.has_value(),
        "resource failure did not produce versioned replay record");
    require(
        failure.replayRecord->replaySchema ==
            D26ReplaySchemaId &&
        failure.replayRecord->sitePolicy ==
            D26SitePolicyId &&
        failure.replayRecord->symbolicPolicy ==
            D26SymbolicPolicyId &&
        failure.replayRecord->fingerprintSchema ==
            D26FingerprintSchemaId,
        "resource replay policy IDs mismatch");
    require(
        validateDelaunayTopology(
            failure.arena.slots(),
            failure.canonicalSites).ok(),
        "resource failure corrupted previously valid partial topology");
    ++stats.expectedFailedBuilds;

    if (replayPath != nullptr) {
        std::ofstream output(
            *replayPath,
            std::ios::binary | std::ios::trunc);
        require(
            static_cast<bool>(output),
            "could not open P1F replay fixture output");
        const std::string serialized =
            serializeDelaunayReplay(
                *failure.replayRecord);
        output.write(
            serialized.data(),
            static_cast<std::streamsize>(
                serialized.size()));
        require(
            static_cast<bool>(output),
            "could not write P1F replay fixture");
    }

    return evidence;
}

std::vector<InputSite> duplicateEnumerationFixture(bool reversed) {
    std::vector<InputSite> raw{
        input(0.0, 0.0, 0.0, 801U),
        input(-0.0, +0.0, -0.0, 806U),
        input(0.0, 0.0, 1.0, 802U),
        input(0.0, 1.0, 0.0, 803U),
        input(1.0, 0.0, 0.0, 804U),
        input(1.0, 1.0, 1.0, 805U)};
    if (reversed) {
        std::reverse(raw.begin(), raw.end());
    }
    return raw;
}

void testInputEnumerationAndDeterminism(
    const GoldenManifests& manifest) {
    const auto aRaw = duplicateEnumerationFixture(false);
    const auto bRaw = duplicateEnumerationFixture(true);
    const auto aCanonical = canonicalizeSites(aRaw);
    const auto bCanonical = canonicalizeSites(bRaw);
    require(
        aCanonical.size() == 5U &&
        bCanonical.size() == 5U,
        "duplicate grouping did not produce five canonical sites");

    const auto a =
        buildWithOrder(
            aRaw, ascendingIds(5U), true);
    const auto b =
        buildWithOrder(
            bRaw, ascendingIds(5U), true);
    ++stats.inputEnumerationCases;
    require(a.ok() && b.ok(), "source enumeration constructor failed");
    require(
        a.finalFingerprint->canonicalSites ==
            b.finalFingerprint->canonicalSites &&
        a.finalFingerprint->canonicalRecord ==
            b.finalFingerprint->canonicalRecord &&
        a.finalFingerprint->canonicalRecord ==
            manifest.fiveRecord,
        "G30 input/source enumeration or signed-zero spelling leaked into D26DT1 identity");

    const auto raw = interiorGoldenFixture();
    const auto ascending = ascendingIds(5U);
    auto reverse = ascending;
    std::reverse(reverse.begin(), reverse.end());

    const auto baseline =
        buildWithOrder(raw, ascending, true);
    const auto repeated =
        buildWithOrder(raw, ascending, true);
    const auto reversed =
        buildWithOrder(raw, reverse, true);
    require(
        baseline.ok() && repeated.ok() && reversed.ok(),
        "G31 representative constructor failed");
    require(
        baseline.finalFingerprint->canonicalRecord ==
            manifest.interiorRecord &&
        repeated.finalFingerprint->canonicalRecord ==
            manifest.interiorRecord &&
        reversed.finalFingerprint->canonicalRecord ==
            manifest.interiorRecord,
        "G31 repeat/supported insertion order changed non-degenerate D26DT1 record");

    DelaunayConstructorOptions noTelemetry;
    noTelemetry.fullInsertionOrder = ascending;
    noTelemetry.telemetryEnabled = false;
    const auto telemetryOff =
        constructDelaunayReference(raw, noTelemetry);
    require(
        telemetryOff.ok() &&
        telemetryOff.finalFingerprint->canonicalRecord ==
            baseline.finalFingerprint->canonicalRecord,
        "telemetry enablement changed topology/fingerprint");

    DelaunayConstructorOptions replayEnabled;
    replayEnabled.fullInsertionOrder = ascending;
    replayEnabled.captureReplayDecisions = true;
    replayEnabled.telemetryEnabled = true;
    const auto recorderOn =
        constructDelaunayReference(raw, replayEnabled);
    require(
        recorderOn.ok() &&
        recorderOn.finalFingerprint->canonicalRecord ==
            baseline.finalFingerprint->canonicalRecord &&
        recorderOn.replayRecord.has_value(),
        "replay observer changed topology or failed to produce record");
}

using Transform = Vec3 (*)(const Vec3&);

Vec3 swapXY(const Vec3& p) {
    return {p.y, p.x, p.z};
}
Vec3 translateExact(const Vec3& p) {
    return {p.x + 8.0, p.y + 4.0, p.z + 2.0};
}
Vec3 scaleTwo(const Vec3& p) {
    return {p.x * 2.0, p.y * 2.0, p.z * 2.0};
}
Vec3 reflectX(const Vec3& p) {
    return {-p.x, p.y, p.z};
}

std::vector<InputSite> transformed(
    const std::vector<InputSite>& raw,
    Transform transform) {
    std::vector<InputSite> result = raw;
    for (auto& site : result) {
        site.point = transform(site.point);
    }
    return result;
}

PointId sourceToCanonical(
    const std::vector<CanonicalSite>& sites,
    std::uint64_t source) {
    for (const auto& site : sites) {
        if (std::find(
                site.sourceRecordIds.begin(),
                site.sourceRecordIds.end(),
                source) != site.sourceRecordIds.end()) {
            return site.id;
        }
    }
    throw std::runtime_error("source correspondence missing");
}

std::uint64_t canonicalToSource(
    const std::vector<CanonicalSite>& sites,
    PointId id) {
    const auto it = std::find_if(
        sites.begin(), sites.end(),
        [id](const CanonicalSite& s) { return s.id == id; });
    if (it == sites.end() ||
        it->sourceRecordIds.size() != 1U) {
        throw std::runtime_error(
            "metamorphic fixture requires unique source correspondence");
    }
    return it->sourceRecordIds.front();
}

std::vector<std::array<PointId, 4>> mappedFinite(
    const DelaunayConstructorResult& original,
    const DelaunayConstructorResult& mapped) {
    std::vector<std::array<PointId, 4>> result;
    for (auto tet : original.finalFingerprint->finiteTets) {
        for (PointId& id : tet) {
            id = sourceToCanonical(
                mapped.canonicalSites,
                canonicalToSource(
                    original.canonicalSites, id));
        }
        std::sort(tet.begin(), tet.end());
        result.push_back(tet);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<std::array<PointId, 3>> mappedHull(
    const DelaunayConstructorResult& original,
    const DelaunayConstructorResult& mapped) {
    std::vector<std::array<PointId, 3>> result;
    for (auto tri : original.finalFingerprint->hullFacets) {
        for (PointId& id : tri) {
            id = sourceToCanonical(
                mapped.canonicalSites,
                canonicalToSource(
                    original.canonicalSites, id));
        }
        std::sort(tri.begin(), tri.end());
        result.push_back(tri);
    }
    std::sort(result.begin(), result.end());
    return result;
}

void testMetamorphic() {
    const std::array<Transform, 4> transforms{
        swapXY,
        translateExact,
        scaleTwo,
        reflectX};

    const auto nondegRaw = interiorGoldenFixture();
    const auto original =
        buildWithOrder(
            nondegRaw, ascendingIds(5U), true);
    require(original.ok(), "non-degenerate metamorphic baseline failed");

    for (Transform transform : transforms) {
        ++stats.metamorphicCases;
        const auto mappedRaw =
            transformed(nondegRaw, transform);
        const auto canonical =
            canonicalizeSites(mappedRaw);
        const auto mapped =
            buildWithOrder(
                mappedRaw,
                ascendingIds(canonical.size()),
                true);
        if (!mapped.ok() ||
            mappedFinite(original, mapped) !=
                mapped.finalFingerprint->finiteTets ||
            mappedHull(original, mapped) !=
                mapped.finalFingerprint->hullFacets) {
            ++stats.metamorphicFailures;
            throw std::runtime_error(
                "non-degenerate exact metamorphic mapped connectivity mismatch");
        }
        checkGlobalWeakDelaunay(mapped);
        checkLocalFiniteLegality(mapped);
        checkHullOracle(mapped);
        checkS3(mapped);
    }

    for (const auto& degenerate :
         {fiveSiteFixture(), cubeFixture()}) {
        for (Transform transform : transforms) {
            ++stats.metamorphicCases;
            const auto mappedRaw =
                transformed(degenerate, transform);
            const auto canonical =
                canonicalizeSites(mappedRaw);
            auto forward = ascendingIds(canonical.size());
            auto reverse = forward;
            std::reverse(reverse.begin(), reverse.end());

            const auto a =
                buildWithOrder(mappedRaw, forward, true);
            const auto b =
                buildWithOrder(mappedRaw, reverse, true);
            if (!a.ok() || !b.ok() ||
                a.finalFingerprint->canonicalRecord !=
                    b.finalFingerprint->canonicalRecord) {
                ++stats.metamorphicFailures;
                throw std::runtime_error(
                    "exact-degenerate transformed policy is not deterministic");
            }
            checkGlobalWeakDelaunay(a);
            checkLocalFiniteLegality(a);
            checkHullOracle(a);
            checkS3(a);
        }
    }
}

void printTelemetry(
    const DelaunayConstructorTelemetry& t) {
    std::cout
        << "telemetry"
        << " sites.input=" << t.sitesInput
        << " sites.canonical=" << t.sitesCanonical
        << " insertion.calls=" << t.insertionCalls
        << " insertion.successes=" << t.insertionSuccesses
        << " insertion.failures=" << t.insertionFailures
        << " locate.calls=" << t.locateCalls
        << " locate.walk_steps=" << t.walkSteps
        << " locate.max_walk=" << t.maxWalk
        << " locate.walk_stalls=" << t.walkStalls
        << " location_oracle.calls="
        << t.bruteForceLocationCalls
        << " location_oracle.finite_cells="
        << t.bruteForceLocationFiniteCellsTested
        << " location_oracle.mismatches="
        << t.locationMismatches
        << " conflict_oracle.cells="
        << t.conflictOracleCellsTested
        << " conflict_oracle.mismatches="
        << t.conflictOracleMismatches
        << " cavity.cells=" << t.cavityCells
        << " cavity.internal_facets="
        << t.internalFacets
        << " cavity.boundary_facets="
        << t.boundaryFacets
        << " cavity.candidate_cells="
        << t.candidateCells
        << " cavity.max=" << t.maxCavity
        << " transaction.plan_failures="
        << t.transactionPlanFailures
        << " transaction.reserve_failures="
        << t.transactionReserveFailures
        << " transaction.commit_failures="
        << t.transactionCommitFailures
        << " orient3d.calls=" << t.orient3d.calls
        << " orient3d.fast=" << t.orient3d.fast
        << " orient3d.exact=" << t.orient3d.exact
        << " orient3d.zero=" << t.orient3d.zero
        << " insphere.calls=" << t.insphere.calls
        << " insphere.fast=" << t.insphere.fast
        << " insphere.exact=" << t.insphere.exact
        << " insphere.zero=" << t.insphere.zero
        << " symbolic.insphere_ties="
        << t.symbolicInsphereTies
        << " symbolic.incircle_ties="
        << t.symbolicIncircleTies
        << " topology.validations="
        << t.topologyValidations
        << " cells.finite_live=" << t.finiteLive
        << " cells.ghost_live=" << t.ghostLive
        << " cells.peak_live=" << t.peakLive
        << " slots.total=" << t.slotsTotal
        << " slots.dead=" << t.slotsDead
        << " hull.facets=" << t.hullFacets
        << " ratio.finite_tets_per_site="
        << t.finiteTetsPerSite
        << " ratio.unified_cells_per_site="
        << t.unifiedCellsPerSite
        << '\n';
}

void testTelemetryBaseline(
    const GoldenManifests& manifest) {
    const auto result =
        buildWithOrder(
            cubeFixture(), ascendingIds(8U), true);
    checkGolden(
        result,
        CubeFinite,
        CubeHull,
        manifest.cubeRecord,
        "telemetry cube");
    require(
        result.telemetry.insertionCalls == 4U &&
        result.telemetry.insertionSuccesses == 4U &&
        result.telemetry.insertionFailures == 0U &&
        result.telemetry.locateCalls == 4U &&
        result.telemetry.bruteForceLocationCalls == 4U &&
        result.telemetry.locationMismatches == 0U &&
        result.telemetry.conflictOracleMismatches == 0U &&
        result.telemetry.cavityCells > 0U &&
        result.telemetry.boundaryFacets > 0U &&
        result.telemetry.orient3d.calls > 0U &&
        result.telemetry.insphere.calls > 0U &&
        result.telemetry.topologyValidations == 5U &&
        result.telemetry.finiteLive == 6U &&
        result.telemetry.ghostLive == 12U &&
        result.telemetry.hullFacets == 12U,
        "P1F whole-constructor telemetry baseline is incomplete");
    printTelemetry(result.telemetry);
}


std::string bytesToHex(std::string_view bytes) {
    static constexpr char digits[] = "0123456789abcdef";
    if (bytes.size() >
        std::numeric_limits<std::size_t>::max() / 2U) {
        throw std::length_error(
            "P1F qualification manifest hex length overflow");
    }
    std::string result;
    result.reserve(bytes.size() * 2U);
    for (unsigned char byte : bytes) {
        result.push_back(digits[(byte >> 4U) & 0xFU]);
        result.push_back(digits[byte & 0xFU]);
    }
    return result;
}

void atomicWriteText(
    const std::string& path,
    const std::string& content) {
    const std::filesystem::path destination(path);
    if (destination.has_parent_path()) {
        std::filesystem::create_directories(
            destination.parent_path());
    }
    const std::filesystem::path temporary =
        destination.string() + ".tmp";
    std::error_code ignored;
    std::filesystem::remove(temporary, ignored);

    {
        std::ofstream output(
            temporary,
            std::ios::binary | std::ios::trunc);
        if (!output) {
            throw std::runtime_error(
                "could not create P1F qualification manifest temporary");
        }
        output.write(
            content.data(),
            static_cast<std::streamsize>(content.size()));
        if (!output) {
            throw std::runtime_error(
                "could not write P1F qualification manifest temporary");
        }
    }

    std::filesystem::remove(destination, ignored);
    std::filesystem::rename(temporary, destination);
}

std::uint64_t elapsedMilliseconds(
    std::chrono::steady_clock::time_point begin,
    std::chrono::steady_clock::time_point end) {
    const auto value =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            end - begin).count();
    if (value < 0) {
        throw std::runtime_error(
            "P1F qualification steady-clock duration was negative");
    }
    return static_cast<std::uint64_t>(value);
}

void writeCoreQualificationManifest(
    const std::string& path,
    const std::string& buildConfig,
    const GoldenManifests& manifest,
    const std::vector<ComplexityEvidence>& complexity,
    std::uint64_t runtimeMs,
    std::uint64_t goldenMs,
    std::uint64_t fiveMs,
    std::uint64_t smallOracleMs,
    std::uint64_t resourceMs,
    std::uint64_t determinismMs,
    std::uint64_t metamorphicMs,
    std::uint64_t telemetryMs) {
    std::ostringstream out;
    out
        << "schema=D26M21FQ1\n"
        << "completion=1\n"
        << "kind=core\n"
        << "build_config=" << buildConfig << "\n"
        << "site_policy=" << D26SitePolicyId << "\n"
        << "symbolic_policy=" << D26SymbolicPolicyId << "\n"
        << "fingerprint_schema=" << D26FingerprintSchemaId << "\n"
        << "replay_schema=" << D26ReplaySchemaId << "\n"
        << "constructor_fixtures=" << stats.constructorFixtures << "\n"
        << "successful_builds=" << stats.successfulBuilds << "\n"
        << "failed_expected_builds=" << stats.expectedFailedBuilds << "\n"
        << "five_site_permutations=" << stats.fiveSitePermutations << "\n"
        << "five_site_mismatches=" << stats.fiveSiteMismatches << "\n"
        << "global_oracle_sets=" << stats.globalOracleSets << "\n"
        << "global_tet_site_tests=" << stats.globalTetSiteTests << "\n"
        << "global_oracle_failures=" << stats.globalOracleFailures << "\n"
        << "local_facets_checked=" << stats.localFacetsChecked << "\n"
        << "local_legality_failures=" << stats.localLegalityFailures << "\n"
        << "symbolic_ties_checked=" << stats.symbolicTiesChecked << "\n"
        << "symbolic_tie_failures=" << stats.symbolicTieFailures << "\n"
        << "S3_states_checked=" << stats.s3StatesChecked << "\n"
        << "S3_failures=" << stats.s3Failures << "\n"
        << "hull_faces_checked=" << stats.hullFacesChecked << "\n"
        << "hull_support_violations=" << stats.hullSupportViolations << "\n"
        << "coplanar_hull_edges=" << stats.coplanarHullEdgesChecked << "\n"
        << "hull_symbolic_ties=" << stats.hullSymbolicTiesChecked << "\n"
        << "hull_symbolic_failures=" << stats.hullSymbolicFailures << "\n"
        << "validator_states=" << stats.validatorStates << "\n"
        << "resource_cases=" << stats.resourceCases << "\n"
        << "metamorphic_cases=" << stats.metamorphicCases << "\n"
        << "metamorphic_failures=" << stats.metamorphicFailures << "\n"
        << "fingerprint_mismatches=" << stats.fingerprintMismatches << "\n"
        << "five_digest=" << manifest.fiveDigest << "\n"
        << "cube_digest=" << manifest.cubeDigest << "\n"
        << "interior_digest=" << manifest.interiorDigest << "\n"
        << "five_record_hex=" << bytesToHex(manifest.fiveRecord) << "\n"
        << "cube_record_hex=" << bytesToHex(manifest.cubeRecord) << "\n"
        << "interior_record_hex=" << bytesToHex(manifest.interiorRecord) << "\n"
        << "runtime_ms=" << runtimeMs << "\n"
        << "phase_golden_ms=" << goldenMs << "\n"
        << "phase_five_ms=" << fiveMs << "\n"
        << "phase_small_oracle_ms=" << smallOracleMs << "\n"
        << "phase_resource_ms=" << resourceMs << "\n"
        << "phase_determinism_ms=" << determinismMs << "\n"
        << "phase_metamorphic_ms=" << metamorphicMs << "\n"
        << "phase_telemetry_ms=" << telemetryMs << "\n";

    for (std::size_t i = 0U; i < complexity.size(); ++i) {
        out
            << "complexity_" << i << "_sites=" << complexity[i].sites << "\n"
            << "complexity_" << i << "_finite=" << complexity[i].finite << "\n"
            << "complexity_" << i << "_hull=" << complexity[i].ghost << "\n"
            << "complexity_" << i << "_slots=" << complexity[i].slots << "\n"
            << "complexity_" << i << "_digest=" << complexity[i].digest << "\n";
    }

    atomicWriteText(path, out.str());
}

} // namespace

int main(int argc, char** argv) {
    try {
        std::optional<std::string> replayPath;
        std::optional<std::string> manifestPath;
        std::string buildConfig{"unspecified"};
        bool skipCubeExhaustive = false;

        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--write-replay" && i + 1 < argc) {
                replayPath = argv[++i];
            } else if (arg == "--manifest" && i + 1 < argc) {
                manifestPath = argv[++i];
            } else if (arg == "--build-config" && i + 1 < argc) {
                buildConfig = argv[++i];
            } else if (arg == "--skip-cube-exhaustive") {
                skipCubeExhaustive = true;
            } else {
                throw std::runtime_error(
                    "usage: unit_m21f_delaunay_constructor "
                    "[--write-replay <path>] [--manifest <path>] "
                    "[--build-config <name>] [--skip-cube-exhaustive]");
            }
        }

        const auto totalBegin = std::chrono::steady_clock::now();

        const auto goldenBegin = std::chrono::steady_clock::now();
        testGoldenDerivations();
        const GoldenManifests manifest =
            testGoldenReferenceBuilds();
        const auto goldenEnd = std::chrono::steady_clock::now();

        const auto fiveBegin = std::chrono::steady_clock::now();
        testExhaustiveFive(manifest);
        const auto fiveEnd = std::chrono::steady_clock::now();

        if (!skipCubeExhaustive) {
            testExhaustiveCube(manifest);
        }

        const auto smallBegin = std::chrono::steady_clock::now();
        testSmallNOracles();
        testLowerDimensionalAndOrderFailures();
        const auto smallEnd = std::chrono::steady_clock::now();

        std::string* replayOutput =
            replayPath.has_value() ? &*replayPath : nullptr;
        const auto resourceBegin = std::chrono::steady_clock::now();
        const auto complexity =
            testComplexityAndResource(replayOutput);
        const auto resourceEnd = std::chrono::steady_clock::now();

        const auto determinismBegin = std::chrono::steady_clock::now();
        testInputEnumerationAndDeterminism(manifest);
        const auto determinismEnd = std::chrono::steady_clock::now();

        const auto metamorphicBegin = std::chrono::steady_clock::now();
        testMetamorphic();
        const auto metamorphicEnd = std::chrono::steady_clock::now();

        const auto telemetryBegin = std::chrono::steady_clock::now();
        testTelemetryBaseline(manifest);
        const auto telemetryEnd = std::chrono::steady_clock::now();

        require(
            stats.fiveSiteMismatches == 0U &&
            stats.cubeMismatches == 0U &&
            stats.fingerprintMismatches == 0U &&
            stats.globalOracleFailures == 0U &&
            stats.localLegalityFailures == 0U &&
            stats.symbolicTieFailures == 0U &&
            stats.s3Failures == 0U &&
            stats.hullSupportViolations == 0U &&
            stats.hullSymbolicFailures == 0U &&
            stats.metamorphicFailures == 0U,
            "P1F aggregate qualification failure counters are non-zero");

        const auto totalEnd = std::chrono::steady_clock::now();
        const std::uint64_t totalMs =
            elapsedMilliseconds(totalBegin, totalEnd);
        const std::uint64_t goldenMs =
            elapsedMilliseconds(goldenBegin, goldenEnd);
        const std::uint64_t fiveMs =
            elapsedMilliseconds(fiveBegin, fiveEnd);
        const std::uint64_t smallMs =
            elapsedMilliseconds(smallBegin, smallEnd);
        const std::uint64_t resourceMs =
            elapsedMilliseconds(resourceBegin, resourceEnd);
        const std::uint64_t determinismMs =
            elapsedMilliseconds(determinismBegin, determinismEnd);
        const std::uint64_t metamorphicMs =
            elapsedMilliseconds(metamorphicBegin, metamorphicEnd);
        const std::uint64_t telemetryMs =
            elapsedMilliseconds(telemetryBegin, telemetryEnd);

        if (manifestPath.has_value()) {
            writeCoreQualificationManifest(
                *manifestPath,
                buildConfig,
                manifest,
                complexity,
                totalMs,
                goldenMs,
                fiveMs,
                smallMs,
                resourceMs,
                determinismMs,
                metamorphicMs,
                telemetryMs);
        }

        std::cout
            << "M2.1-F serial constructor core qualification PASS"
            << " checks=" << checks
            << " constructor_fixtures=" << stats.constructorFixtures
            << " successful_builds=" << stats.successfulBuilds
            << " failed_expected_builds=" << stats.expectedFailedBuilds
            << " five_site_permutations=" << stats.fiveSitePermutations
            << " five_site_mismatches=" << stats.fiveSiteMismatches
            << " cube_permutations=" << stats.cubePermutations
            << " cube_mismatches=" << stats.cubeMismatches
            << " global_oracle_sets=" << stats.globalOracleSets
            << " global_tet_site_tests=" << stats.globalTetSiteTests
            << " global_oracle_failures=" << stats.globalOracleFailures
            << " local_facets_checked=" << stats.localFacetsChecked
            << " local_legality_failures=" << stats.localLegalityFailures
            << " symbolic_ties_checked=" << stats.symbolicTiesChecked
            << " symbolic_tie_failures=" << stats.symbolicTieFailures
            << " S3_states_checked=" << stats.s3StatesChecked
            << " S3_failures=" << stats.s3Failures
            << " hull_faces_checked=" << stats.hullFacesChecked
            << " hull_support_violations=" << stats.hullSupportViolations
            << " coplanar_hull_edges=" << stats.coplanarHullEdgesChecked
            << " hull_symbolic_ties=" << stats.hullSymbolicTiesChecked
            << " hull_symbolic_failures=" << stats.hullSymbolicFailures
            << " validator_states=" << stats.validatorStates
            << " resource_cases=" << stats.resourceCases
            << " metamorphic_cases=" << stats.metamorphicCases
            << " metamorphic_failures=" << stats.metamorphicFailures
            << " fingerprint_mismatches=" << stats.fingerprintMismatches
            << " runtime_ms=" << totalMs << '\n';

        std::cout
            << "phase_runtime"
            << " golden_ms=" << goldenMs
            << " five_ms=" << fiveMs
            << " small_oracle_ms=" << smallMs
            << " resource_ms=" << resourceMs
            << " determinism_ms=" << determinismMs
            << " metamorphic_ms=" << metamorphicMs
            << " telemetry_ms=" << telemetryMs
            << '\n';

        std::cout
            << "determinism_manifest"
            << " site_policy=" << D26SitePolicyId
            << " symbolic_policy=" << D26SymbolicPolicyId
            << " fingerprint_schema=" << D26FingerprintSchemaId
            << " replay_schema=" << D26ReplaySchemaId
            << " five_sites=5 five_finite=2 five_hull=6 five_digest="
            << manifest.fiveDigest
            << " cube_sites=8 cube_finite=6 cube_hull=12 cube_digest="
            << manifest.cubeDigest
            << " interior_sites=5 interior_finite=4 interior_hull=4 interior_digest="
            << manifest.interiorDigest
            << '\n';

        for (std::size_t i = 0U; i < complexity.size(); ++i) {
            std::cout
                << "complexity_manifest"
                << " case=" << i
                << " sites=" << complexity[i].sites
                << " finite=" << complexity[i].finite
                << " hull=" << complexity[i].ghost
                << " slots=" << complexity[i].slots
                << " digest=" << complexity[i].digest
                << '\n';
        }

        return 0;
    } catch (const std::exception& error) {
        std::cerr
            << "M2.1-F serial constructor qualification FAIL: "
            << error.what() << '\n';
        return 1;
    }
}
