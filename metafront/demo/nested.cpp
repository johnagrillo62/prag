#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <typeinfo>

// Helper to detect std::map
template <typename T> struct is_std_map : std::false_type
{
};

template <typename K, typename V> struct is_std_map<std::map<K, V>> : std::true_type
{
    using KeyType = K;
    using ValueType = V;
};

template <typename T> void inspectType(const std::string& prefix = "")
{
    if constexpr (is_std_map<T>::value)
    {
        using Key = typename is_std_map<T>::KeyType;
        using Value = typename is_std_map<T>::ValueType;
        std::cout << prefix << "Map<" << typeid(Key).name() << ", ";
        if constexpr (is_std_map<Value>::value)
        {
            std::cout << "nested map>\n";
            inspectType<Value>(prefix + "  "); // recurse with indentation
        }
        else
        {
            std::cout << typeid(Value).name() << ">\n";
        }
    }
    else
    {
        std::cout << prefix << typeid(T).name() << "\n";
    }
}
