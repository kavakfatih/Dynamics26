#include "DelaunayPredicates.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>

namespace femcae::meshing::m2 {
namespace {

using predicates::PredicateSign;

template <typename Site, std::size_t N>
void validateSiteIds(
    const std::array<Site, N>& sites,
    const char* context) {
    for (std::size_t i = 0U; i < N; ++i) {
        if (sites[i].id == InvalidPointId) {
            throw std::invalid_argument(
                std::string(context) + " requires non-zero PointId values");
        }
        for (std::size_t j = i + 1U; j < N; ++j) {
            if (sites[i].id == sites[j].id) {
                throw std::invalid_argument(
                    std::string(context) + " requires unique PointId values");
            }
        }
    }
}

PredicateSign negated(PredicateSign sign) noexcept {
    if (sign == PredicateSign::Positive) {
        return PredicateSign::Negative;
    }
    if (sign == PredicateSign::Negative) {
        return PredicateSign::Positive;
    }
    return PredicateSign::Zero;
}

template <std::size_t N>
PredicateSign firstCofactorByPointId(
    const std::array<PointId, N>& ids,
    const std::array<PredicateSign, N>& coefficients) {
    std::array<std::size_t, N> order{};
    for (std::size_t i = 0U; i < N; ++i) {
        order[i] = i;
    }
    std::sort(
        order.begin(),
        order.end(),
        [&ids](std::size_t lhs, std::size_t rhs) {
            return ids[lhs] < ids[rhs];
        });

    for (std::size_t row : order) {
        if (coefficients[row] != PredicateSign::Zero) {
            return coefficients[row];
        }
    }
    return PredicateSign::Zero;
}

enum class Projection : std::uint8_t {
    XY = 0,
    XZ = 1,
    YZ = 2
};

IndexedPoint2 projectPoint(
    const IndexedPoint3& site,
    Projection projection) {
    switch (projection) {
    case Projection::XY:
        return {site.id, {site.point.x, site.point.y}};
    case Projection::XZ:
        return {site.id, {site.point.x, site.point.z}};
    case Projection::YZ:
        return {site.id, {site.point.y, site.point.z}};
    }
    throw std::logic_error("unknown M2 projection");
}

DelaunayConflict conflictFromSign(PredicateSign sign) {
    if (sign == PredicateSign::Positive) {
        return DelaunayConflict::Conflict;
    }
    if (sign == PredicateSign::Negative) {
        return DelaunayConflict::NoConflict;
    }
    throw std::logic_error("resolved Delaunay sign must be non-zero");
}

} // namespace

ResolvedDelaunaySign resolveLiftOnlyInsphere(
    const std::array<IndexedPoint3, 5>& sites) {
    validateSiteIds(sites, "D26LIFT1 InSphere");

    const PredicateSign raw = predicates::insphere(
        sites[0].point,
        sites[1].point,
        sites[2].point,
        sites[3].point,
        sites[4].point).sign;

    if (raw != PredicateSign::Zero) {
        return {raw, raw};
    }

    // Dynamics26 row-major [x y z lift 1] determinantinde lift kolonunun
    // kofaktor isaretleri sirasiyla - + - + - olur.
    const std::array<PredicateSign, 5> coefficients{
        negated(predicates::orient3d(
            sites[1].point, sites[2].point, sites[3].point, sites[4].point).sign),
        predicates::orient3d(
            sites[0].point, sites[2].point, sites[3].point, sites[4].point).sign,
        negated(predicates::orient3d(
            sites[0].point, sites[1].point, sites[3].point, sites[4].point).sign),
        predicates::orient3d(
            sites[0].point, sites[1].point, sites[2].point, sites[4].point).sign,
        negated(predicates::orient3d(
            sites[0].point, sites[1].point, sites[2].point, sites[3].point).sign)};

    const std::array<PointId, 5> ids{
        sites[0].id, sites[1].id, sites[2].id, sites[3].id, sites[4].id};

    const PredicateSign resolved =
        firstCofactorByPointId(ids, coefficients);
    if (resolved == PredicateSign::Zero) {
        throw std::invalid_argument(
            "D26LIFT1 InSphere tie is unresolved because all orientation cofactors are zero");
    }

    return {PredicateSign::Zero, resolved};
}

ResolvedDelaunaySign resolveLiftOnlyIncircle(
    const std::array<IndexedPoint2, 4>& sites) {
    validateSiteIds(sites, "D26LIFT1 InCircle");

    const PredicateSign raw = predicates::incircle(
        sites[0].point,
        sites[1].point,
        sites[2].point,
        sites[3].point).sign;

    if (raw != PredicateSign::Zero) {
        return {raw, raw};
    }

    // Dynamics26 row-major [x y lift 1] determinantinde lift kolonunun
    // kofaktor isaretleri sirasiyla + - + - olur.
    const std::array<PredicateSign, 4> coefficients{
        predicates::orient2d(
            sites[1].point, sites[2].point, sites[3].point).sign,
        negated(predicates::orient2d(
            sites[0].point, sites[2].point, sites[3].point).sign),
        predicates::orient2d(
            sites[0].point, sites[1].point, sites[3].point).sign,
        negated(predicates::orient2d(
            sites[0].point, sites[1].point, sites[2].point).sign)};

    const std::array<PointId, 4> ids{
        sites[0].id, sites[1].id, sites[2].id, sites[3].id};

    const PredicateSign resolved =
        firstCofactorByPointId(ids, coefficients);
    if (resolved == PredicateSign::Zero) {
        throw std::invalid_argument(
            "D26LIFT1 InCircle tie is unresolved because all orientation cofactors are zero");
    }

    return {PredicateSign::Zero, resolved};
}

DelaunayConflict classifyFiniteCellConflict(
    const std::array<IndexedPoint3, 4>& positiveTetrahedron,
    const IndexedPoint3& query) {
    const std::array<IndexedPoint3, 5> sites{
        positiveTetrahedron[0],
        positiveTetrahedron[1],
        positiveTetrahedron[2],
        positiveTetrahedron[3],
        query};
    validateSiteIds(sites, "finite Delaunay conflict");

    const PredicateSign orientation = predicates::orient3d(
        positiveTetrahedron[0].point,
        positiveTetrahedron[1].point,
        positiveTetrahedron[2].point,
        positiveTetrahedron[3].point).sign;
    if (orientation != PredicateSign::Positive) {
        throw std::invalid_argument(
            "finite Delaunay conflict requires a positive stored tetrahedron");
    }

    return conflictFromSign(resolveLiftOnlyInsphere(sites).resolvedSign);
}

ResolvedDelaunaySign classifyProjectedCoplanarCircumcircle(
    const std::array<IndexedPoint3, 3>& facet,
    const IndexedPoint3& query) {
    const std::array<IndexedPoint3, 4> sites{
        facet[0], facet[1], facet[2], query};
    validateSiteIds(sites, "projected hull circumcircle");

    const PredicateSign coplanarity = predicates::orient3d(
        facet[0].point,
        facet[1].point,
        facet[2].point,
        query.point).sign;
    if (coplanarity != PredicateSign::Zero) {
        throw std::invalid_argument(
            "projected hull circumcircle requires an exactly coplanar query");
    }

    constexpr std::array<Projection, 3> projections{
        Projection::XY, Projection::XZ, Projection::YZ};

    for (Projection projection : projections) {
        std::array<IndexedPoint2, 4> projected{
            projectPoint(facet[0], projection),
            projectPoint(facet[1], projection),
            projectPoint(facet[2], projection),
            projectPoint(query, projection)};

        const PredicateSign orientation = predicates::orient2d(
            projected[0].point,
            projected[1].point,
            projected[2].point).sign;
        if (orientation == PredicateSign::Zero) {
            continue;
        }

        if (orientation == PredicateSign::Negative) {
            std::swap(projected[1], projected[2]);
        }
        return resolveLiftOnlyIncircle(projected);
    }

    throw std::invalid_argument(
        "projected hull circumcircle requires a non-collinear facet");
}

DelaunayConflict classifyGhostCellConflict(
    const std::array<IndexedPoint3, 3>& outwardFacet,
    const IndexedPoint3& insideWitness,
    const IndexedPoint3& query) {
    const std::array<IndexedPoint3, 5> sites{
        outwardFacet[0],
        outwardFacet[1],
        outwardFacet[2],
        insideWitness,
        query};
    validateSiteIds(sites, "ghost Delaunay conflict");

    const PredicateSign insideSide = predicates::orient3d(
        outwardFacet[0].point,
        outwardFacet[1].point,
        outwardFacet[2].point,
        insideWitness.point).sign;
    if (insideSide != PredicateSign::Negative) {
        throw std::invalid_argument(
            "ghost Delaunay conflict requires an outward-oriented facet");
    }

    const PredicateSign querySide = predicates::orient3d(
        outwardFacet[0].point,
        outwardFacet[1].point,
        outwardFacet[2].point,
        query.point).sign;

    if (querySide == PredicateSign::Positive) {
        return DelaunayConflict::Conflict;
    }
    if (querySide == PredicateSign::Negative) {
        return DelaunayConflict::NoConflict;
    }

    const ResolvedDelaunaySign circle =
        classifyProjectedCoplanarCircumcircle(outwardFacet, query);
    return conflictFromSign(circle.resolvedSign);
}

} // namespace femcae::meshing::m2
