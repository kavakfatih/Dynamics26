#include "femcae/meshing/RobustPredicates.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void fail(const std::string& message) {
    throw std::runtime_error(message);
}

void require(bool condition, const std::string& message) {
    if (!condition) {
        fail(message);
    }
}

} // namespace

int main() {
    using namespace femcae::meshing::predicates;

    try {
        PredicateTelemetry telemetry;

        for (int i = 0; i < 1000; ++i) {
            const auto result = orient3d(
                {1.0, 0.0, 0.0},
                {0.0, 1.0, 0.0},
                {0.0, 0.0, 1.0},
                {0.0, 0.0, 0.0},
                &telemetry);
            require(result.sign == PredicateSign::Positive, "ordinary orient3d sign changed");
        }

        for (int i = 0; i < 25; ++i) {
            const auto result = orient3d(
                {0.0, 0.0, 0.0},
                {1.0, 0.0, 0.0},
                {0.0, 1.0, 0.0},
                {1.0, 1.0, 0.0},
                &telemetry);
            require(result.sign == PredicateSign::Zero, "coplanar orient3d is not exact zero");
        }

        bool rejected = false;
        try {
            (void)orient2d(
                {std::numeric_limits<double>::infinity(), 0.0},
                {1.0, 0.0},
                {0.0, 1.0},
                &telemetry);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "invalid input was accepted");

        require(telemetry.calls == 1026U, "telemetry calls mismatch");
        require(telemetry.fastCertified >= 1000U, "ordinary fast path was not certified");
        require(telemetry.exactFallback >= 25U, "exact fallback was not recorded");
        require(telemetry.exactZero >= 25U, "exact-zero telemetry missing");
        require(telemetry.invalidInput == 1U, "invalid-input telemetry mismatch");
        require(
            telemetry.fastCertified + telemetry.exactFallback + telemetry.invalidInput ==
                telemetry.calls,
            "telemetry categories do not partition calls");

        std::cout << "M1.9-D telemetry PASS "
                  << "calls=" << telemetry.calls
                  << " fast=" << telemetry.fastCertified
                  << " exact=" << telemetry.exactFallback
                  << " zero=" << telemetry.exactZero
                  << " invalid=" << telemetry.invalidInput << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "M1.9-D telemetry FAIL: " << error.what() << '\n';
        return 1;
    }
}
