#pragma once
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <variant>
#include <vector>
#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>
#include <array>

// ============================================================================
// TYPE TRAITS (anonymous namespace)
// ============================================================================
namespace
{
template <typename T> struct is_vector : std::false_type {};
template <typename T> struct is_vector<std::vector<T>> : std::true_type {};
template <typename T> inline constexpr bool is_vector_v = is_vector<T>::value;

template <typename T> struct is_tuple : std::false_type {};
template <typename... Args> struct is_tuple<std::tuple<Args...>> : std::true_type {};
template <typename T> inline constexpr bool is_tuple_v = is_tuple<T>::value;

template <typename T> struct is_map : std::false_type {};
template <typename K, typename V> struct is_map<std::map<K, V>> : std::true_type {};
template <typename T> inline constexpr bool is_map_v = is_map<T>::value;

template <typename T> struct is_optional : std::false_type {};
template <typename U> struct is_optional<std::optional<U>> : std::true_type {};
template <typename T> inline constexpr bool is_optional_v = is_optional<T>::value;

template <typename T> struct is_string : std::false_type {};
template <> struct is_string<std::string> : std::true_type {};
template <typename T> inline constexpr bool is_string_v = is_string<T>::value;

template <typename T> struct is_variant : std::false_type {};
template <typename... Args> struct is_variant<std::variant<Args...>> : std::true_type {};
template <typename T> inline constexpr bool is_variant_v = is_variant<T>::value;
}

namespace meta
{

// ============================================================================
// CONCEPTS
// ============================================================================
template <typename T>
concept String = std::is_same_v<T, std::string>;

template <typename T>
concept Arithmetic = std::is_arithmetic_v<T>;

template <typename T>
concept Vector = is_vector_v<T>;

template <typename T>
concept Map = is_map_v<T>;

template <typename T>
concept Optional = is_optional_v<T>;

template <typename T>
concept Variant = is_variant_v<T>;

template <typename T>
concept Tuple = is_tuple_v<T>;

template <typename T>
concept Object = !Arithmetic<T> && !String<T> && !Vector<T> && !Map<T> && !Optional<T> && !Variant<T>;

// ============================================================================
// TYPE NAME EXTRACTION
// ============================================================================
template <typename T>
constexpr std::string_view type_name() {
#if defined(__GNUC__) || defined(__clang__)
    constexpr std::string_view func = __PRETTY_FUNCTION__;
    constexpr std::string_view prefix = "T = ";
    constexpr std::string_view suffix = "]";
    
    constexpr auto start = func.find(prefix) + prefix.size();
    constexpr auto end = func.rfind(suffix);
    
    return func.substr(start, end - start);
    
#elif defined(_MSC_VER)
    constexpr std::string_view func = __FUNCSIG__;
    constexpr std::string_view prefix = "type_name<";
    constexpr std::string_view suffix = ">(void)";
    
    constexpr auto start = func.find(prefix) + prefix.size();
    constexpr auto end = func.find(suffix);
    
    return func.substr(start, end - start);
    
#else
    return "unknown";
#endif
}

// ============================================================================
// PROPS
// ============================================================================
enum Prop : uint8_t
{
    None = 0,
    PrimaryKey = 1 << 0,
    HasSetter = 1 << 1,
    HasGetter = 1 << 2,
    Serializable = 1 << 3,
    Hashable = 1 << 4,
    Private = 1 << 5
};

constexpr Prop operator|(Prop a, Prop b)
{
    return static_cast<Prop>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}

constexpr Prop operator&(Prop a, Prop b)
{
    return static_cast<Prop>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

inline std::string propsToString(Prop props)
{
    std::vector<std::string> flags;

    if (props & Prop::PrimaryKey)
        flags.push_back("PrimaryKey");
    if (props & Prop::HasSetter)
        flags.push_back("HasSetter");
    if (props & Prop::HasGetter)
        flags.push_back("HasGetter");
    if (props & Prop::Serializable)
        flags.push_back("Serializable");
    if (props & Prop::Hashable)
        flags.push_back("Hashable");
    if (props & Prop::Private)
        flags.push_back("Private");

    if (flags.empty())
        return "None";

    std::string result = "meta::Prop::" + flags[0];
    for (size_t i = 1; i < flags.size(); ++i)
    {
        result += " | meta::Prop::" + flags[i];
    }
    return result;
}

struct Props {
    Prop value;
    constexpr Props(Prop p = Prop::None) : value(p) {}
};


// ============================================================================
// ACCESSORS ATTRIBUTE (combines getter + setter)
// ============================================================================
template <auto GetterPtr, auto SetterPtr = nullptr>
struct Accessors {
    static constexpr auto getter = GetterPtr;
    static constexpr auto setter = SetterPtr;
};
  

template <auto FuncPtr>
struct Getter {
    static constexpr auto ptr = FuncPtr;
};

template <auto FuncPtr>
struct Setter {
    static constexpr auto ptr = FuncPtr;
};

// ============================================================================
// MEMBER POINTER TRAITS
// ============================================================================
template <typename>
struct member_pointer_traits;

template <typename Class, typename T>
struct member_pointer_traits<T Class::*> {
    using class_type = Class;
    using member_type = T;
};

template <typename T, typename U, U T::* MemberPtr> 
struct is_member_accessible
{
  private:
    template <typename X, typename Y>
    static auto test(int) -> decltype(std::declval<X>().*MemberPtr, std::true_type{});

    template <typename, typename> 
    static auto test(...) -> std::false_type;

  public:
    static constexpr bool value = decltype(test<T, U>(0))::value;
};

// ============================================================================
// FIELD
// ============================================================================
template <typename Class,
          auto MemberPtr,
          typename... Attrs>
struct Field
{
public:


private:


  
    static constexpr bool has_member_ptr = (MemberPtr != nullptr);
    
    // Helper to check if we have a Getter attribute
    template <typename Attr>
    struct is_getter : std::false_type {};
    
    template <auto Ptr>
    struct is_getter<Getter<Ptr>> : std::true_type {};
    
    // Helper to check if we have a Setter attribute
    template <typename Attr>
    struct is_setter : std::false_type {};
    
    template <auto Ptr>
    struct is_setter<Setter<Ptr>> : std::true_type {};
    
    // Check if any attribute is a Getter
    static constexpr bool has_getter = (is_getter<Attrs>::value || ...);
    
    // Extract getter pointer
    template <typename... As>
    static constexpr auto extractGetterPtr() {
        if constexpr (sizeof...(As) == 0) {
            return nullptr;
        } else {
            return extractGetterPtrImpl<As...>();
        }
    }
    
    template <typename First, typename... Rest>
    static constexpr auto extractGetterPtrImpl() {
        if constexpr (is_getter<First>::value) {
            return First::ptr;
        } else if constexpr (sizeof...(Rest) > 0) {
            return extractGetterPtrImpl<Rest...>();
        } else {
            return nullptr;
        }
    }
    
    // Extract setter pointer
    template <typename... As>
    static constexpr auto extractSetterPtr() {
        if constexpr (sizeof...(As) == 0) {
            return nullptr;
        } else {
            return extractSetterPtrImpl<As...>();
        }
    }
    
    template <typename First, typename... Rest>
    static constexpr auto extractSetterPtrImpl() {
        if constexpr (is_setter<First>::value) {
            return First::ptr;
        } else if constexpr (sizeof...(Rest) > 0) {
            return extractSetterPtrImpl<Rest...>();
        } else {
            return nullptr;
        }
    }
public:    
    static constexpr auto getterPtr = extractGetterPtr<Attrs...>();
    static constexpr auto setterPtr = extractSetterPtr<Attrs...>();
private:    
    using DeducedFromMember = typename std::conditional_t<has_member_ptr, member_pointer_traits<decltype(MemberPtr)>, member_pointer_traits<int Class::*>>::member_type;
    using DeducedFromGetter = std::remove_cvref_t<typename std::conditional_t<has_getter && (getterPtr != nullptr), std::invoke_result<decltype(getterPtr), const Class&>, std::type_identity<int>>::type>;
    
public:
    using T = std::conditional_t<has_member_ptr, DeducedFromMember, DeducedFromGetter>;
    using type = T;
    using FieldType = T;
    
    // Ensure field has a way to access data
    static_assert(
        has_member_ptr || has_getter,
        "Field must have either a member pointer or a Getter attribute - cannot create field with nullptr member and no getter!"
    );
    
    static constexpr auto memberPtr = MemberPtr;
    
    const char* fieldName;
    std::tuple<Attrs...> attributes;
    
    // Constructor - just name + variadic attributes
    template <typename... Args>
    constexpr Field(const char* name, Args&&... args)
        : fieldName(name), attributes(std::forward<Args>(args)...) {}
    
    // Get type name via __PRETTY_FUNCTION__
    static constexpr auto getTypeName() {
        return type_name<T>();
    }
    
    // Attribute access
    template <typename AttrType>
    static constexpr bool has() {
        return (std::is_same_v<AttrType, Attrs> || ...);
    }
    
    template <typename AttrType>
    constexpr const auto& get() const {
        static_assert(has<AttrType>(), "Field does not have this attribute type");
        return std::get<AttrType>(attributes);
    }
    
    // Convenience: get properties (returns Prop::None if no Props attribute)
    constexpr Prop getProps() const {
        if constexpr (has<Props>()) {
            return get<Props>().value;
        }
        return Prop::None;
    }
    
    // Field value access
    T getValue(const Class& obj) const {
        if constexpr (has_member_ptr) {
            return obj.*memberPtr;
        } else if constexpr (getterPtr != nullptr) {
            // Handle member function pointers vs free functions
            if constexpr (std::is_member_function_pointer_v<decltype(getterPtr)>) {
                return (obj.*getterPtr)();
            } else {
                return getterPtr(obj);
            }
        } else {
            return T{};
        }
    }

    void setValue(Class& obj, const T& value) const {
        if constexpr (has_member_ptr) {
            obj.*memberPtr = value;
        } else if constexpr (setterPtr != nullptr) {
            // Handle member function pointers vs free functions
            if constexpr (std::is_member_function_pointer_v<decltype(setterPtr)>) {
                (obj.*setterPtr)(value);
            } else {
                setterPtr(obj, value);
            }
        }
    }
};


  
// ============================================================================
// META TUPLE
// ============================================================================
template <typename T> 
struct MetaTuple
{
    static_assert(sizeof(T) == 0, "No metadata tuple defined for this type");
};

} // namespace meta

