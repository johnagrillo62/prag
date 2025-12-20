#include <iostream>
#include <map>
#include <vector>

// Extract actual assigned data from an existing instance
template <typename T> void extract_assigned_data(const T& instance)
{
    std::cout << "=== Extracting Assigned Data ===" << std::endl;

    constexpr auto& fields = MetaTuple<T>::fields;

    std::apply([&instance](auto&&... field_metas)
               { ((extract_real_field_value(instance, field_metas)), ...); },
               fields);
}

template <typename Instance, typename FieldMeta>
void extract_real_field_value(const Instance& instance, const FieldMeta& field_meta)
{
    using FieldType = typename FieldMeta::FieldType;

    // Get the actual value from the instance
    auto actual_value = instance.*(field_meta.ptr);

    std::cout << field_meta.name << " (" << CleanTypeMapper<FieldType, Language::RUST>::map()
              << "): ";

    print_field_value(actual_value);
    std::cout << std::endl;
}
