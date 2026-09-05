#include "femcae/meshing/RobustPredicates.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

#ifdef __FAST_MATH__
#error "Dynamics26 RobustPredicates cannot be compiled with fast-math semantics"
#endif

namespace femcae::meshing::predicates {
namespace {

class BigInt {
public:
    BigInt() = default;

    static BigInt fromSignedMagnitude(int sign, std::uint64_t magnitude) {
        BigInt result;
        if (magnitude == 0 || sign == 0) {
            return result;
        }
        result.sign_ = sign < 0 ? -1 : 1;
        result.limbs_.push_back(static_cast<std::uint32_t>(magnitude & 0xFFFFFFFFULL));
        const std::uint32_t high = static_cast<std::uint32_t>(magnitude >> 32U);
        if (high != 0U) {
            result.limbs_.push_back(high);
        }
        return result;
    }

    static BigInt one() {
        return fromSignedMagnitude(1, 1);
    }

    [[nodiscard]] int sign() const noexcept {
        return sign_;
    }

    [[nodiscard]] BigInt negated() const {
        BigInt result(*this);
        result.sign_ = -result.sign_;
        return result;
    }

    void shiftLeft(std::size_t bits) {
        if (sign_ == 0 || bits == 0) {
            return;
        }

        const std::size_t wordShift = bits / 32U;
        const unsigned bitShift = static_cast<unsigned>(bits % 32U);
        std::vector<std::uint32_t> shifted(
            limbs_.size() + wordShift + (bitShift == 0U ? 0U : 1U), 0U);

        std::uint64_t carry = 0;
        for (std::size_t i = 0; i < limbs_.size(); ++i) {
            const std::uint64_t value =
                (static_cast<std::uint64_t>(limbs_[i]) << bitShift) | carry;
            shifted[i + wordShift] = static_cast<std::uint32_t>(value & 0xFFFFFFFFULL);
            carry = bitShift == 0U ? 0U : (value >> 32U);
        }
        if (bitShift != 0U) {
            shifted[limbs_.size() + wordShift] = static_cast<std::uint32_t>(carry);
        }

        limbs_ = std::move(shifted);
        normalize();
    }

    friend BigInt operator+(const BigInt& lhs, const BigInt& rhs) {
        if (lhs.sign_ == 0) {
            return rhs;
        }
        if (rhs.sign_ == 0) {
            return lhs;
        }

        if (lhs.sign_ == rhs.sign_) {
            BigInt result;
            result.sign_ = lhs.sign_;
            result.limbs_ = addAbs(lhs.limbs_, rhs.limbs_);
            result.normalize();
            return result;
        }

        const int comparison = compareAbs(lhs.limbs_, rhs.limbs_);
        if (comparison == 0) {
            return {};
        }

        BigInt result;
        if (comparison > 0) {
            result.sign_ = lhs.sign_;
            result.limbs_ = subtractAbs(lhs.limbs_, rhs.limbs_);
        } else {
            result.sign_ = rhs.sign_;
            result.limbs_ = subtractAbs(rhs.limbs_, lhs.limbs_);
        }
        result.normalize();
        return result;
    }

    friend BigInt operator*(const BigInt& lhs, const BigInt& rhs) {
        if (lhs.sign_ == 0 || rhs.sign_ == 0) {
            return {};
        }

        BigInt result;
        result.sign_ = lhs.sign_ * rhs.sign_;
        result.limbs_.assign(lhs.limbs_.size() + rhs.limbs_.size(), 0U);

        for (std::size_t i = 0; i < lhs.limbs_.size(); ++i) {
            std::uint64_t carry = 0;
            for (std::size_t j = 0; j < rhs.limbs_.size(); ++j) {
                const std::size_t index = i + j;
                const std::uint64_t value =
                    static_cast<std::uint64_t>(result.limbs_[index]) +
                    static_cast<std::uint64_t>(lhs.limbs_[i]) *
                        static_cast<std::uint64_t>(rhs.limbs_[j]) +
                    carry;
                result.limbs_[index] =
                    static_cast<std::uint32_t>(value & 0xFFFFFFFFULL);
                carry = value >> 32U;
            }

            std::size_t index = i + rhs.limbs_.size();
            while (carry != 0) {
                if (index == result.limbs_.size()) {
                    result.limbs_.push_back(0U);
                }
                const std::uint64_t value =
                    static_cast<std::uint64_t>(result.limbs_[index]) + carry;
                result.limbs_[index] =
                    static_cast<std::uint32_t>(value & 0xFFFFFFFFULL);
                carry = value >> 32U;
                ++index;
            }
        }

        result.normalize();
        return result;
    }

private:
    static int compareAbs(
        const std::vector<std::uint32_t>& lhs,
        const std::vector<std::uint32_t>& rhs) {
        if (lhs.size() != rhs.size()) {
            return lhs.size() < rhs.size() ? -1 : 1;
        }
        for (std::size_t i = lhs.size(); i > 0; --i) {
            const std::uint32_t a = lhs[i - 1];
            const std::uint32_t b = rhs[i - 1];
            if (a != b) {
                return a < b ? -1 : 1;
            }
        }
        return 0;
    }

    static std::vector<std::uint32_t> addAbs(
        const std::vector<std::uint32_t>& lhs,
        const std::vector<std::uint32_t>& rhs) {
        const std::size_t size = std::max(lhs.size(), rhs.size());
        std::vector<std::uint32_t> result(size, 0U);
        std::uint64_t carry = 0;

        for (std::size_t i = 0; i < size; ++i) {
            const std::uint64_t a = i < lhs.size() ? lhs[i] : 0U;
            const std::uint64_t b = i < rhs.size() ? rhs[i] : 0U;
            const std::uint64_t value = a + b + carry;
            result[i] = static_cast<std::uint32_t>(value & 0xFFFFFFFFULL);
            carry = value >> 32U;
        }
        if (carry != 0) {
            result.push_back(static_cast<std::uint32_t>(carry));
        }
        return result;
    }

    static std::vector<std::uint32_t> subtractAbs(
        const std::vector<std::uint32_t>& larger,
        const std::vector<std::uint32_t>& smaller) {
        std::vector<std::uint32_t> result(larger.size(), 0U);
        std::uint64_t borrow = 0;

        for (std::size_t i = 0; i < larger.size(); ++i) {
            const std::uint64_t a = larger[i];
            const std::uint64_t b = (i < smaller.size() ? smaller[i] : 0U) + borrow;
            if (a >= b) {
                result[i] = static_cast<std::uint32_t>(a - b);
                borrow = 0;
            } else {
                result[i] = static_cast<std::uint32_t>((1ULL << 32U) + a - b);
                borrow = 1;
            }
        }
        return result;
    }

    void normalize() {
        while (!limbs_.empty() && limbs_.back() == 0U) {
            limbs_.pop_back();
        }
        if (limbs_.empty()) {
            sign_ = 0;
        }
    }

    int sign_{0};
    std::vector<std::uint32_t> limbs_;
};

struct DyadicValue {
    int sign{0};
    std::uint64_t significand{0};
    int exponent{0};
};

DyadicValue decodeBinary64(double value) {
    if (!std::isfinite(value)) {
        throw std::invalid_argument("robust predicate coordinates must be finite");
    }

    const std::uint64_t bits = std::bit_cast<std::uint64_t>(value);
    const int sign = (bits >> 63U) == 0U ? 1 : -1;
    const std::uint64_t exponentBits = (bits >> 52U) & 0x7FFULL;
    const std::uint64_t fractionBits = bits & ((1ULL << 52U) - 1ULL);

    if (exponentBits == 0U) {
        if (fractionBits == 0U) {
            return {};
        }
        return {sign, fractionBits, -1074};
    }

    const std::uint64_t significand = (1ULL << 52U) | fractionBits;
    const int exponent = static_cast<int>(exponentBits) - 1023 - 52;
    return {sign, significand, exponent};
}

std::vector<BigInt> exactIntegerCoordinates(const std::vector<double>& coordinates) {
    std::vector<DyadicValue> dyadics;
    dyadics.reserve(coordinates.size());

    bool haveNonZero = false;
    int commonExponent = 0;
    for (double coordinate : coordinates) {
        const DyadicValue value = decodeBinary64(coordinate);
        dyadics.push_back(value);
        if (value.significand != 0U) {
            if (!haveNonZero || value.exponent < commonExponent) {
                commonExponent = value.exponent;
                haveNonZero = true;
            }
        }
    }

    std::vector<BigInt> integers;
    integers.reserve(dyadics.size());
    for (const DyadicValue& value : dyadics) {
        BigInt integer =
            BigInt::fromSignedMagnitude(value.sign, value.significand);
        if (value.significand != 0U) {
            integer.shiftLeft(static_cast<std::size_t>(value.exponent - commonExponent));
        }
        integers.push_back(std::move(integer));
    }
    return integers;
}

using Matrix = std::vector<std::vector<BigInt>>;

BigInt determinant(const Matrix& matrix) {
    const std::size_t size = matrix.size();
    if (size == 0U) {
        throw std::logic_error("exact determinant matrix must be non-empty");
    }
    for (const auto& row : matrix) {
        if (row.size() != size) {
            throw std::logic_error("exact determinant matrix must be square");
        }
    }

    if (size == 1U) {
        return matrix[0][0];
    }

    BigInt result;
    for (std::size_t column = 0; column < size; ++column) {
        Matrix minor;
        minor.reserve(size - 1U);
        for (std::size_t row = 1; row < size; ++row) {
            std::vector<BigInt> minorRow;
            minorRow.reserve(size - 1U);
            for (std::size_t sourceColumn = 0; sourceColumn < size; ++sourceColumn) {
                if (sourceColumn != column) {
                    minorRow.push_back(matrix[row][sourceColumn]);
                }
            }
            minor.push_back(std::move(minorRow));
        }

        BigInt term = matrix[0][column] * determinant(minor);
        if ((column & 1U) != 0U) {
            term = term.negated();
        }
        result = result + term;
    }
    return result;
}

PredicateSign predicateSign(int sign) noexcept {
    if (sign < 0) {
        return PredicateSign::Negative;
    }
    if (sign > 0) {
        return PredicateSign::Positive;
    }
    return PredicateSign::Zero;
}

PredicateEvaluation exactEvaluation(const Matrix& matrix) {
    return {predicateSign(determinant(matrix).sign()), PredicateEvaluationPath::ExactDyadic};
}

} // namespace

PredicateEvaluation orient2d(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c) {
    const std::vector<BigInt> p = exactIntegerCoordinates({
        a.x, a.y, b.x, b.y, c.x, c.y});

    Matrix matrix{
        {p[0], p[1], BigInt::one()},
        {p[2], p[3], BigInt::one()},
        {p[4], p[5], BigInt::one()}};
    return exactEvaluation(matrix);
}

PredicateEvaluation orient3d(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d) {
    const std::vector<BigInt> p = exactIntegerCoordinates({
        a.x, a.y, a.z,
        b.x, b.y, b.z,
        c.x, c.y, c.z,
        d.x, d.y, d.z});

    Matrix matrix{
        {p[0], p[1], p[2], BigInt::one()},
        {p[3], p[4], p[5], BigInt::one()},
        {p[6], p[7], p[8], BigInt::one()},
        {p[9], p[10], p[11], BigInt::one()}};
    return exactEvaluation(matrix);
}

PredicateEvaluation incircle(
    const geometry::Vec2& a,
    const geometry::Vec2& b,
    const geometry::Vec2& c,
    const geometry::Vec2& d) {
    const std::vector<BigInt> p = exactIntegerCoordinates({
        a.x, a.y, b.x, b.y, c.x, c.y, d.x, d.y});

    Matrix matrix;
    matrix.reserve(4U);
    for (std::size_t i = 0; i < 4U; ++i) {
        const BigInt& x = p[2U * i];
        const BigInt& y = p[2U * i + 1U];
        const BigInt lift = x * x + y * y;
        matrix.push_back({x, y, lift, BigInt::one()});
    }
    return exactEvaluation(matrix);
}

PredicateEvaluation insphere(
    const geometry::Vec3& a,
    const geometry::Vec3& b,
    const geometry::Vec3& c,
    const geometry::Vec3& d,
    const geometry::Vec3& e) {
    const std::vector<BigInt> p = exactIntegerCoordinates({
        a.x, a.y, a.z,
        b.x, b.y, b.z,
        c.x, c.y, c.z,
        d.x, d.y, d.z,
        e.x, e.y, e.z});

    Matrix matrix;
    matrix.reserve(5U);
    for (std::size_t i = 0; i < 5U; ++i) {
        const BigInt& x = p[3U * i];
        const BigInt& y = p[3U * i + 1U];
        const BigInt& z = p[3U * i + 2U];
        const BigInt lift = x * x + y * y + z * z;
        matrix.push_back({x, y, z, lift, BigInt::one()});
    }
    return exactEvaluation(matrix);
}

} // namespace femcae::meshing::predicates
