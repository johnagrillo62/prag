#include <tuple>
#include <string>
#include <type_traits>
#include <iostream>

#include </mnt/c/Users/johna/source/repos/meta/meta.h>
using namespace meta;

// ------------------------
// Helper to detect if T is aggregate-initializable with N fields
// ------------------------
template <typename T, size_t N>
struct can_init_with_n {
    struct any_type {
        template <typename U> constexpr operator U(); 
    };

    template <size_t... I>
    static constexpr bool test(std::index_sequence<I...>) {
        return requires { T{(static_cast<void>(I), any_type{})...}; };
    }

    static constexpr bool value = test(std::make_index_sequence<N>{});
};

// ------------------------
// Recursive count_fields up to Max
// ------------------------
template <typename T, size_t N = 20>
struct count_fields_impl {
    static constexpr size_t value =
        can_init_with_n<T, N>::value ? N : count_fields_impl<T, N-1>::value;
};

// Base case
template <typename T>
struct count_fields_impl<T, 0> {
    static constexpr size_t value = 0;
};

template <typename T>
constexpr size_t count_fields = count_fields_impl<T>::value;

// ------------------------
// Example Struct
// ------------------------
struct Car {
    std::string maker;
    std::string model;
    unsigned short year;
    bool electric;
};

// ------------------------
// Field Tuple
// ------------------------
inline const auto CarFields = std::make_tuple(
    field<&Car::maker>("maker"),
    field<&Car::model>("model"),
    field<&Car::year>("year"),
    field<&Car::electric>("electric")
);
// ------------------------
// Validate tuple vs struct
// ------------------------
template <typename Class, typename Tuple>
consteval void validate_tuple_vs_struct() {
    constexpr size_t tupleSize = std::tuple_size_v<Tuple>;
    constexpr size_t structFields = count_fields<Class>;
    static_assert(tupleSize == structFields, 
                  "Field count mismatch between struct and Field tuple");
}

// ------------------------
// Usage
// ------------------------
int main() {
    validate_tuple_vs_struct<Car, decltype(CarFields)>();
    std::cout << "Car field count matches Field tuple!" << std::endl;
}


