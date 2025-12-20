/*
================================================================================
MetaField - C++ Reflection Metadata Headers
================================================================================

Author: John Grillo
Date: 2025-09-13
Version: 2.0

Description:
------------
Core reflection metadata structures and utilities for the MetaFront tool.
Provides type-safe field metadata with getter/setter support.

================================================================================
*/

#pragma once

#include <string>
#include <type_traits>
#include <vector>
#include <tuple>
#include <functional>

// -------------------------
// Field Properties Enum
// -------------------------
enum FieldProp : uint32_t {
    None         = 0,
    Serializable = 1 << 0,
    Private      = 1 << 1,
    Protected    = 1 << 2,
    Getter       = 1 << 3,
    Setter       = 1 << 4,
    ReadOnly     = 1 << 5,
    WriteOnly    = 1 << 6,
    Transient    = 1 << 7,
    Key          = 1 << 8,
    Index        = 1 << 9,
    Required     = 1 << 10,
    Unique       = 1 << 11
};

// Bitwise operators for FieldProp
constexpr FieldProp operator|(FieldProp lhs, FieldProp rhs) {
    return static_cast<FieldProp>(
        static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs)
    );
}

constexpr FieldProp operator&(FieldProp lhs, FieldProp rhs) {
    return static_cast<FieldProp>(
        static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs)
    );
}

constexpr bool hasFlag(FieldProp value, FieldProp flag) {
    return (value & flag) == flag;
}

// -------------------------
// Field Properties String Conversion
// -------------------------
inline std::string fieldPropsToString(FieldProp props) {
    std::string result = "FieldProp::";
    
    if (static_cast<uint32_t>(props) == 0) {
        return "FieldProp::None";
    }
    
    bool first = true;
    auto addProp = [&](FieldProp flag, const char* name) {
        if (hasFlag(props, flag)) {
            if (!first) result += " | FieldProp::";
            result += name;
            first = false;
        }
    };
    
    addProp(FieldProp::Serializable, "Serializable");
    addProp(FieldProp::Private, "Private");
    addProp(FieldProp::Protected, "Protected");
    addProp(FieldProp::Getter, "Getter");
    addProp(FieldProp::Setter, "Setter");
    addProp(FieldProp::ReadOnly, "ReadOnly");
    addProp(FieldProp::WriteOnly, "WriteOnly");
    addProp(FieldProp::Transient, "Transient");
    addProp(FieldProp::Key, "Key");
    addProp(FieldProp::Index, "Index");
    addProp(FieldProp::Required, "Required");
    addProp(FieldProp::Unique, "Unique");
    
    return result;
}



namespace meta {


// -------------------------
// Type Traits for Function Pointers
// -------------------------
template<typename T>
struct is_member_function_ptr : std::false_type {};

template<typename R, typename C, typename... Args>
struct is_member_function_ptr<R(C::*)(Args...)> : std::true_type {};

template<typename R, typename C, typename... Args>
struct is_member_function_ptr<R(C::*)(Args...) const> : std::true_type {};

template<typename T>
constexpr bool is_member_function_ptr_v = is_member_function_ptr<T>::value;

// -------------------------
// Field Metadata Template
// -------------------------
template
    typename ClassType,
    typename FieldType,
    auto MemberPtr,
    auto GetterPtr = nullptr,
    auto SetterPtr = nullptr,
    FieldProp Properties = FieldProp::Serializable
>
struct FieldMeta {
    using class_type = ClassType;
    using field_type = FieldType;
    
    static constexpr auto member_ptr = MemberPtr;
    static constexpr auto getter_ptr = GetterPtr;
    static constexpr auto setter_ptr = SetterPtr;
    static constexpr FieldProp properties = Properties;
    
    // Runtime field information
    std::string type_name;
    std::string field_name;
    std::vector<std::string> annotations;
    
    // Constructor for runtime info
    constexpr FieldMeta(const char* type_str, const char* name_str)
        : type_name(type_str), field_name(name_str) {}
    
    template<typename... Args>
    constexpr FieldMeta(const char* type_str, const char* name_str, Args&&... args)
        : type_name(type_str), field_name(name_str), annotations{std::forward<Args>(args)...} {}
    
    // Field access methods
    template<typename T = decltype(MemberPtr)>
    static constexpr bool has_direct_access() {
        return !std::is_same_v<T, std::nullptr_t>;
    }
    
    template<typename T = decltype(GetterPtr)>
    static constexpr bool has_getter() {
        return !std::is_same_v<T, std::nullptr_t>;
    }
    
    template<typename T = decltype(SetterPtr)>
    static constexpr bool has_setter() {
        return !std::is_same_v<T, std::nullptr_t>;
    }
    
    // Get field value (via direct access or getter)
    template<typename ObjType>
    static auto get_value(const ObjType& obj) {
        if constexpr (has_direct_access()) {
            return obj.*MemberPtr;
        } else if constexpr (has_getter()) {
            return (obj.*GetterPtr)();
        } else {
            static_assert(has_direct_access() || has_getter(), 
                         "Field must have either direct access or getter");
        }
    }
    
    // Set field value (via direct access or setter)
    template<typename ObjType, typename ValueType>
    static void set_value(ObjType& obj, ValueType&& value) {
        if constexpr (has_direct_access()) {
            obj.*MemberPtr = std::forward<ValueType>(value);
        } else if constexpr (has_setter()) {
            if constexpr (std::is_void_v<std::invoke_result_t<decltype(SetterPtr), ObjType&, ValueType>>) {
                (obj.*SetterPtr)(std::forward<ValueType>(value));
            } else {
                // For fluent interface setters that return reference
                (obj.*SetterPtr)(std::forward<ValueType>(value));
            }
        } else {
            static_assert(has_direct_access() || has_setter(), 
                         "Field must have either direct access or setter");
        }
    }
    
    // Property checks
    static constexpr bool is_serializable() {
        return hasFlag(properties, FieldProp::Serializable);
    }
    
    static constexpr bool is_private() {
        return hasFlag(properties, FieldProp::Private);
    }
    
    static constexpr bool is_protected() {
        return hasFlag(properties, FieldProp::Protected);
    }
    
    static constexpr bool is_read_only() {
        return hasFlag(properties, FieldProp::ReadOnly);
    }
    
    static constexpr bool is_write_only() {
        return hasFlag(properties, FieldProp::WriteOnly);
    }
    
    static constexpr bool is_key() {
        return hasFlag(properties, FieldProp::Key);
    }
    
    static constexpr bool is_required() {
        return hasFlag(properties, FieldProp::Required);
    }
    
    static constexpr bool is_unique() {
        return hasFlag(properties, FieldProp::Unique);
    }
};

// -------------------------
// MetaTuple Base Template
// -------------------------
template<typename T>
struct MetaTuple {
    // This will be specialized for each class by the code generator
    static_assert(std::is_class_v<T>, "MetaTuple can only be used with class types");
    
    // Default implementation - will be overridden by specializations
    static constexpr auto fields = std::tuple<>{};
    static constexpr const char* table_name = "unknown";
};

// -------------------------
// Tuple Utilities
// -------------------------
template<typename T, std::size_t I = 0>
constexpr std::size_t tuple_size_v = std::tuple_size_v<T>;

template<typename ClassType>
constexpr std::size_t get_field_count() {
    return tuple_size_v<decltype(MetaTuple<ClassType>::fields)>;
}

template<std::size_t Index, typename ClassType>
constexpr auto get_field_meta() {
    return std::get<Index>(MetaTuple<ClassType>::fields);
}

template<typename ClassType>
constexpr const char* get_table_name() {
    return MetaTuple<ClassType>::table_name;
}

// -------------------------
// Field Iteration
// -------------------------
template<typename ClassType, typename Func>
constexpr void for_each_field(Func&& func) {
    constexpr auto field_count = get_field_count<ClassType>();
    
    [&]<std::size_t... Is>(std::index_sequence<Is...>) {
        (func(get_field_meta<Is, ClassType>()), ...);
    }(std::make_index_sequence<field_count>{});
}

template<typename ClassType, typename Func>
void for_each_field_value(const ClassType& obj, Func&& func) {
    for_each_field<ClassType>([&](const auto& field_meta) {
        using FieldMeta = std::decay_t<decltype(field_meta)>;
        auto value = FieldMeta::get_value(obj);
        func(field_meta.field_name, value);
    });
}

// -------------------------
// Serialization Helpers
// -------------------------
template<typename ClassType>
constexpr bool has_serializable_fields() {
    bool has_serializable = false;
    for_each_field<ClassType>([&](const auto& field_meta) {
        if (field_meta.is_serializable()) {
            has_serializable = true;
        }
    });
    return has_serializable;
}

template<typename ClassType, typename Func>
void for_each_serializable_field(const ClassType& obj, Func&& func) {
    for_each_field<ClassType>([&](const auto& field_meta) {
        if (field_meta.is_serializable()) {
            using FieldMeta = std::decay_t<decltype(field_meta)>;
            auto value = FieldMeta::get_value(obj);
            func(field_meta.field_name, value);
        }
    });
}

} // namespace meta


