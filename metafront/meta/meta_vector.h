// Extracted vector handling function
template <typename FieldType, typename FieldMetaType>
std::string handleVectorField(const FieldMetaType& fieldMeta,
                              const FieldType& value,
                              int indent = 0)
{
    using ElemType = typename FieldType::value_type;
    std::ostringstream os;
    std::string ind(indent * 2, ' ');

    os << ind << fieldMeta.memberName << " " << fieldMeta.typeName << "[\n";

    for (const auto& elem : value)
    {
        if constexpr (is_primitive<ElemType>)
        {
            if constexpr (std::is_same_v<ElemType, std::string>)
                os << ind << "  \"" << elem << "\"\n";
            else
                os << ind << "  " << elem << "\n";
        }
        // future: could recurse if ElemType has MetaTuple
    }

    os << ind << "]\n";
    return os.str();
}
