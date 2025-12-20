// Enhanced map walker with recursion support
template <typename FieldType, typename FieldMetaType>
std::string handleMapField(const FieldMetaType& fieldMeta, const FieldType& value, int indent = 0)
{
    using KeyType = typename FieldType::key_type;
    using ValueType = typename FieldType::mapped_type;

    std::ostringstream os;
    std::string ind(indent * 2, ' ');

    os << ind << fieldMeta.memberName << " : " << fieldMeta.typeName << "> {\n";

    for (const auto& [key, val] : value)
    {
        os << ind << "  ";

        // Handle key - could be recursive
        if constexpr (is_primitive<KeyType>)
        {
            if constexpr (std::is_same_v<KeyType, std::string>)
            {
                os << "\"" << key << "\"";
            }
            else
            {
                os << key;
            }
        }
        else if constexpr (is_map<KeyType>)
        {
            os << "map_key {\n";
            // Could recursively handle map keys (rare but possible)
            os << ind << "    <nested_map_key>\n";
            os << ind << "  }";
        }
        else
        {
            os << "<complex_key>";
        }

        os << " => ";

        // Handle value - could be recursive
        if constexpr (is_primitive<ValueType>)
        {
            if constexpr (std::is_same_v<ValueType, std::string>)
            {
                os << "\"" << val << "\"";
            }
            else
            {
                os << val;
            }
        }
        else if constexpr (is_map<ValueType>)
        {
            os << "\n";
            // Recursively handle nested maps
            struct FakeFieldMeta
            {
                std::string memberName = "nested_map";
            };
            FakeFieldMeta fakeMeta;
            os << handleMapField(fakeMeta, val, indent + 2);
            os << ind << "  ";
        }
        else
        {
            os << "<complex_value>";
        }

        os << "\n";
    }

    os << ind << "}\n";
    return os.str();
}
