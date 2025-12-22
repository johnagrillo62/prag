#include <tuple>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include "meta.h"

using namespace meta;

// ==================== RUNTIME FIELD NAME VALIDATION ====================
template <typename FieldTuple, std::size_t... Is>
void validate_field_names_impl(const FieldTuple& t, std::index_sequence<Is...>, std::vector<std::string>& errors)
{
    (([&]{
        const auto& f = std::get<Is>(t);
        // Compute pretty name from type deduction at compile time
        auto pretty = f.getTypeName();          // type_name<T>() from your Field
        auto manual = f.fieldName;             // manual name from tuple

        if (pretty != manual)
        {
            errors.push_back("Field name mismatch at index " + std::to_string(Is) +
                             ": prettyName='" + std::string(pretty) +
                             "', manualName='" + std::string(manual) + "'");
        }
    }()), ...);
}

template <typename FieldTuple>
std::vector<std::string> validate_field_names(const FieldTuple& t)
{
    std::vector<std::string> errors;
    validate_field_names_impl(t, std::make_index_sequence<std::tuple_size_v<FieldTuple>>{}, errors);
    return errors;
}

// ==================== EXAMPLE STRUCT ====================
struct Car {
    std::string maker;
    std::string model;
    unsigned short year;
    bool electric;
};

// ==================== FIELD TUPLE ====================
inline const auto CarFields = std::make_tuple(
    MakeField<&Car::maker>("maker"),
    MakeField<&Car::model>("model"),
    MakeField<&Car::year>("year"),
    MakeField<&Car::electric>("electric")
);

int main()
{
    // --- Check field count ---
    constexpr size_t tupleCount = std::tuple_size_v<decltype(CarFields)>;
    constexpr size_t structCount = 4; // or use your count_fields<Car> implementation
    static_assert(tupleCount == structCount, "Field count mismatch!");

    std::cout << "Car field count matches Field tuple!\n";

    // --- Validate field names at runtime ---
    auto errors = validate_field_names(CarFields);

    if (errors.empty())
        std::cout << "All field names match!\n";
    else
        for (auto& e : errors)
            std::cout << e << "\n";

    return 0;
}


