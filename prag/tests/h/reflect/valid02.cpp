#include <tuple>
#include <string_view>
#include <vector>
#include <iostream>

// ---------------- Field ----------------
template <typename Class, auto MemberPtr>
struct Field {
    std::string_view manualName;

    constexpr Field(std::string_view n) : manualName(n) {}

    static consteval std::string_view prettyName() {
        constexpr std::string_view pf = __PRETTY_FUNCTION__;
        constexpr std::string_view key1 = "MemberPtr = &Class::"; // GCC
        constexpr std::string_view key2 = "MemberPtr = &";       // Clang

        std::size_t pos = pf.find(key1);
        std::string_view name;

        if (pos != std::string_view::npos) {
            pos += key1.size();
            std::size_t end = pf.find_first_of("];", pos);
            if (end == std::string_view::npos) end = pf.size();
            name = pf.substr(pos, end - pos);
        } else {
            pos = pf.find(key2);
            if (pos != std::string_view::npos) {
                pos += key2.size();
                std::size_t end = pf.find_first_of("];", pos);
                if (end == std::string_view::npos) end = pf.size();
                name = pf.substr(pos, end - pos);
                std::size_t sep = name.find("::");
                if (sep != std::string_view::npos) name = name.substr(sep + 2);
            } else {
                name = {};
            }
        }
        return name;
    }
};

// ---------------- Compile-time field count ----------------
template <typename T, size_t N>
struct can_init_with_n {
    struct any_type { template <typename U> constexpr operator U(); };
    template <size_t... I>
    static constexpr bool test(std::index_sequence<I...>) {
        return requires { T{(static_cast<void>(I), any_type{})...}; };
    }
    static constexpr bool value = test(std::make_index_sequence<N>{});
};

template <typename T, size_t N = 20>
struct count_fields_impl {
    static constexpr size_t value =
        can_init_with_n<T, N>::value ? N : count_fields_impl<T, N-1>::value;
};
template <typename T> struct count_fields_impl<T, 0> { static constexpr size_t value = 0; };
template <typename T> constexpr size_t count_fields = count_fields_impl<T>::value;

template <typename Class, typename Tuple>
consteval void validate_tuple_count() {
    constexpr size_t tuple_size = std::tuple_size_v<Tuple>;
    constexpr size_t struct_count = count_fields<Class>;
    static_assert(tuple_size == struct_count,
                  "Field count mismatch between struct and Field tuple");
}

// ---------------- Runtime field name check ----------------
template <typename Tuple, std::size_t... Is>
void validate_tuple_fields_impl(const Tuple& t, std::index_sequence<Is...>, std::vector<std::string>& errors) {
    (([&] {
        auto pretty = std::get<Is>(t).prettyName();
        auto manual = std::get<Is>(t).manualName;
        if (pretty != manual) {
            errors.push_back("Field name mismatch at index " + std::to_string(Is) +
                             ": prettyName='" + std::string(pretty) +
                             "', manualName='" + std::string(manual) + "'");
        }
    }()), ...);
}

template <typename Tuple>
std::vector<std::string> validate_tuple_fields(const Tuple& t) {
    std::vector<std::string> errors;
    validate_tuple_fields_impl(t, std::make_index_sequence<std::tuple_size_v<Tuple>>{}, errors);
    return errors;
}

// ---------------- Test struct ----------------
struct Car {
    std::string maker;
    std::string model;
    unsigned short year;
    bool electric;
};

const auto CarFields = std::make_tuple(
    Field<Car, &Car::maker>{"maker"},
    Field<Car, &Car::model>{"model"},
    Field<Car, &Car::year>{"year"},
    Field<Car, &Car::electric>{"electric"}
);

int main() {
    // Compile-time check
    validate_tuple_count<Car, decltype(CarFields)>();

    // Runtime check
    auto errors = validate_tuple_fields(CarFields);
    if (errors.empty()) {
        std::cout << "All field names match!\n";
    } else {
        for (auto& e : errors) std::cout << e << "\n";
    }
}
