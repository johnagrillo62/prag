

template <typename FieldMetaType, typename ObjectType>
std::string toStringField(const FieldMetaType& fieldMeta, const ObjectType& obj, int indent = 0)
{
    using FieldType = std::remove_cv_t<std::remove_reference_t<typename FieldMetaType::type>>;
    const auto& value = obj.*(fieldMeta.memberPtr);

    std::ostringstream os;
    std::string ind(indent * 2, ' ');

    os << ind << fieldMeta.memberName << " : " << typeid(FieldType).name();
}

// ---------------- Convert an obj to a mdb schema ---------------------- //
template <typename ObjectType> std::string toSchema(const ObjectType& obj, int indent = 0)
{
    std::ostringstream os;
    auto& tpl = meta::MetaTuple<ObjectType>::fields;

    std::apply([&](auto&&... fieldMeta) { ((os << toSchemaField(fieldMeta, obj, indent)), ...); },
               tpl);

    return os.str();
}
