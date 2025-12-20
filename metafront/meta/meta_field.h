#pragma once
#include <string>
#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>
#include <array>
#include <string_view>
#include <optional>
#include <vector>

namespace meta
{

enum Prop : uint8_t
{
    None = 0,
    PrimaryKey = 1 << 0,
    Setter = 1 << 1,
    Getter = 1 << 2,
    Serializable = 1 << 3,
    Hashable = 1 << 4,
    Private = 1 << 5
};

// Enable bitwise OR for convenience
constexpr Prop operator|(Prop a, Prop b)
{
    return static_cast<Prop>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
}
constexpr Prop operator&(Prop a, Prop b)
{
    return static_cast<Prop>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
}

std::string propsToString(Prop props)
{
    std::vector<std::string> flags;

    if (props & Prop::PrimaryKey)
        flags.push_back("PrimaryKey");
    if (props & Prop::Setter)
        flags.push_back("Setter");
    if (props & Prop::Getter)
        flags.push_back("Getter");
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


// Define type aliases outside the template
template<typename Class, typename T>
using GetterPtr_t = T (Class::*)() const;

template<typename Class, typename T>
using SetterPtr_t = void (Class::*)(const T&);

  // Dictionary for attributes - flexible key-value store
using Attributes = std::unordered_map<std::string, std::string>;

// Common attribute keys as constexpr
inline constexpr const char* CSV_COLUMN = "csv_column";
inline constexpr const char* SQL_COLUMN = "sql_column";
inline constexpr const char* SRC_NAME   = "src_name";

// Helper: detect if member pointer is accessible
template <typename T, typename U, U T::* MemberPtr> struct is_member_accessible
{
  private:
    template <typename X, typename Y>
    static auto test(int) -> decltype(std::declval<X>().*MemberPtr, std::true_type{});

    template <typename, typename> static auto test(...) -> std::false_type;

  public:
    static constexpr bool value = decltype(test<T, U>(0))::value;
};

  
// Field metadata template with member pointer in template parameters
template <typename Class,
          typename T,
          T Class::* MemberPtr = nullptr,
          Prop Props = Prop::None,
          GetterPtr_t<Class, T> GetterPtr = nullptr,
          SetterPtr_t<Class, T> SetterPtr = nullptr>

struct Field
{
    // Constructor: field_type, field_name, member_name, mapping
    Field(const std::string& field_type, 
          const std::string& field_name,
          Attributes attrs = {})
        : fieldType(field_type),
          fieldName(field_name),
          attributes(std::move(attrs))
    {
    }

    using type = T;
    using FieldType = T;
    static constexpr Prop properties = Props;
    static constexpr T Class::* memberPtr = MemberPtr;

    static constexpr GetterPtr_t<Class, T> getterPtr = GetterPtr;
    static constexpr SetterPtr_t<Class, T> setterPtr = SetterPtr;
    
    // Args: field_type (arg 0), field_name (arg 1)
    std::string fieldType;
    std::string fieldName;
    
    // name to apply name conventions
    std::string cleanName;
    
    // Dictionary: flexible annotations (optional)
    Attributes attributes;

  

    // Dictionary-like accessors
    std::optional<std::string> getAttribute(const std::string& key) const 
    { 
      if (attributes.empty()) {
            return std::nullopt;
        }
        
        auto it = attributes.find(key);
        if (it != attributes.end()) {
            return it->second;
        }
        return std::nullopt;
    }
    
    bool hasAttribute(const std::string& key) const 
    {
      if (attributes.empty()) {
            return false;
        }
        return attributes.find(key) != attributes.end();
    }
    
    bool hasAttributes() const 
    {
        return !attributes.empty();
    }
    
    std::optional<std::string> getCsvColumn() const 
    {
        return getAttribute(CSV_COLUMN);
    }
    
    std::optional<std::string> getSqlColumn() const 
    {
        return getAttribute(SQL_COLUMN);
    }
  
    // Getter: get value from object using compile-time member pointer
    T get(const Class& obj) const
    {
        if constexpr (MemberPtr != nullptr)
        {
            return obj.*MemberPtr;
        }
        else
        {
            return T{}; // Default value for nullptr case
        }
    }

    // Setter: set value on object using compile-time member pointer
    void set(Class& obj, const T& value) const
    {
        if constexpr (MemberPtr != nullptr)
        {
            obj.*MemberPtr = value;
        }
        // Do nothing for nullptr case
    }
};

// Primary template (generic types will fail)
template <typename T> struct MetaTuple
{
    static_assert(sizeof(T) == 0, "No metadata tuple defined for this type");
};

} // namespace meta


// template <typename T>
// concept MetaAble = requires {
//     { T::fields } -> std::convertible_to<decltype(T::fields)>;
//     { T::query } -> std::convertible_to<const char*>;
//     { T::table } -> std::convertible_to<const char*>;
// };

