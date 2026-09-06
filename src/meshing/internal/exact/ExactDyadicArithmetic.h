#pragma once

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

namespace femcae::meshing::internal::exact {

// Dynamics26 internal exact-arithmetic mechanism.
//
// This header is intentionally under src/ and is not part of the installed/public API.
// It contains arithmetic mechanism only; robust-predicate and M6 quality semantics remain
// owned by their respective callers.
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

    [[nodiscard]] std::size_t bitLength() const noexcept {
        if (limbs_.empty()) {
            return 0U;
        }
        return (limbs_.size() - 1U) * 32U +
               static_cast<std::size_t>(std::bit_width(limbs_.back()));
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

inline DyadicValue decodeBinary64(double value) {
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

inline std::vector<BigInt> exactIntegerCoordinates(
    const std::vector<double>& coordinates) {
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
            integer.shiftLeft(
                static_cast<std::size_t>(value.exponent - commonExponent));
        }
        integers.push_back(std::move(integer));
    }
    return integers;
}

using Matrix = std::vector<std::vector<BigInt>>;

inline BigInt determinant(const Matrix& matrix) {
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

} // namespace femcae::meshing::internal::exact
