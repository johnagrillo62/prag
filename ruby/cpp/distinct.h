#pragma once
#include <concepts>
#include <cstdint>
#include <ostream>

template <typename Tag> class DistinctIntType
{
  public:
    using underlying_type = std::int32_t; // big enough for any unsigned type
  private:
    underlying_type value;

  public:
    constexpr underlying_type get() const noexcept
    {
        return value;
    }
    // Default constructor
    constexpr DistinctIntType() noexcept : value(0)
    {
    }

    // Construct from any unsigned integral type
    template <std::unsigned_integral T>
    explicit constexpr DistinctIntType(T val) noexcept : value(static_cast<underlying_type>(val))
    {
    }

    // Construct from signed long
    explicit constexpr DistinctIntType(long val) noexcept : value(val < 0 ? 0 : static_cast<underlying_type>(val))
    {
    }

    // Pre-increment
    constexpr DistinctIntType& operator++() noexcept
    {
        ++value;
        return *this;
    }

    // Post-increment
    constexpr DistinctIntType operator++(int) noexcept
    {
        DistinctIntType temp = *this;
        ++value;
        return temp;
    }

    // Comparisons
    friend constexpr bool operator==(const DistinctIntType& a, const DistinctIntType& b) noexcept
    {
        return a.value == b.value;
    }

    friend constexpr bool operator!=(const DistinctIntType& a, const DistinctIntType& b) noexcept
    {
        return !(a == b);
    }

    friend constexpr bool operator<(const DistinctIntType& a, const DistinctIntType& b) noexcept
    {
        return a.value < b.value;
    }

    // Stream output
    friend std::ostream& operator<<(std::ostream& os, const DistinctIntType& c)
    {
        return os << c.value;
    }
};
