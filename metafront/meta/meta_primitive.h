#include <sstream>
#include <string>
#include <type_traits>
#include <typeinfo>

// Extracted primitive handling function
template <typename FieldType, typename FieldMetaType>
std::string handlePrimitiveField(const FieldMetaType& fieldMeta,
                                 const FieldType& value,
                                 int indent = 0)
{
    std::ostringstream os;
    std::string ind(indent * 2, ' ');

    os << ind << fieldMeta.memberName << " : " << fieldMeta.typeName << " = ";

    if constexpr (std::is_same_v<FieldType, bool>)
        os << (value ? "true" : "false");
    else if constexpr (std::is_same_v<FieldType, std::string>)
        os << "\"" << value << "\"";
    else
        os << value;

    os << "\n";
    return os.str();
}
