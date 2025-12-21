
#include "../meta/meta.h"

struct Attrs
{
};

struct Attrs2
{
};

struct A
{
    int x;
    int y;

    // Clean syntax without macros using field helper
    constexpr static const auto fields =
        std::make_tuple(meta::field<&A::x>("id"), meta::field<&A::y>("Y", Attrs{}, Attrs2));
};

// Example with Props attributes
struct B
{
    int value;
    std::string name;

    constexpr static const auto fields =
        std::make_tuple(meta::field<&B::value>("value", meta::Props{meta::Prop::PrimaryKey}),
                        meta::field<&B::name>("name", meta::Props{meta::Prop::Serializable}));
};

// Example with getter/setter (still need explicit Field syntax for these)
struct C
{
  private:
    int m_data;

  public:
    int getData() const
    {
        return m_data;
    }
    void setData(int val)
    {
        m_data = val;
    }

    constexpr static const auto fields = std::make_tuple(
        meta::Field<C, nullptr, meta::Getter<&C::getData>, meta::Setter<&C::setData>>(
            "data",
            meta::Getter<&C::getData>{},
            meta::Setter<&C::setData>{}));
};

int main()
{
    // Test the fields
    A a{42, 100};

    auto& field1 = std::get<0>(A::fields);
    auto& field2 = std::get<1>(A::fields);

    std::cout << "Field 1 name: " << field1.fieldName << ", value: " << field1.getValue(a)
              << std::endl;
    std::cout << "Field 2 name: " << field2.fieldName << ", value: " << field2.getValue(a)
              << std::endl;

    return 0;
}
