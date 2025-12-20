#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

namespace my {
namespace ns {
struct Athlete {
    Athlete() = default;

    int athlete;
    std::optional<int> team1;
    std::optional<std::string> last;
    std::optional<std::string> first;
    std::optional<std::string> initial;
    std::string sex;
    std::chrono::system_clock::time_point birth;
    int age;
    std::string id_no;
    bool inactive;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Athlete, int, &::Athlete::athlete, nullptr, nullptr, meta::Prop::Serializable>("int", "athlete", "Athlete", meta::Mapping("Athlete", "Athlete")),
        meta::Field<::Athlete, std::optional<int>, &::Athlete::team1, nullptr, nullptr, meta::Prop::Serializable>("std::optional<int>", "team1", "Team1", meta::Mapping("Team1", "Team1")),
        meta::Field<::Athlete, std::optional<std::string>, &::Athlete::last, nullptr, nullptr, meta::Prop::Serializable>("std::optional<std::string>", "last", "Last", meta::Mapping("Last", "Last")),
        meta::Field<::Athlete, std::optional<std::string>, &::Athlete::first, nullptr, nullptr, meta::Prop::Serializable>("std::optional<std::string>", "first", "First", meta::Mapping("First", "First")),
        meta::Field<::Athlete, std::optional<std::string>, &::Athlete::initial, nullptr, nullptr, meta::Prop::Serializable>("std::optional<std::string>", "initial", "Initial", meta::Mapping("Initial", "Initial")),
        meta::Field<::Athlete, std::string, &::Athlete::sex, nullptr, nullptr, meta::Prop::Serializable>("std::string", "sex", "Sex", meta::Mapping("Sex", "Sex")),
        meta::Field<::Athlete, std::chrono::system_clock::time_point, &::Athlete::birth, nullptr, nullptr, meta::Prop::Serializable>("std::chrono::system_clock::time_point", "birth", "Birth", meta::Mapping("Birth", "Birth")),
        meta::Field<::Athlete, int, &::Athlete::age, nullptr, nullptr, meta::Prop::Serializable>("int", "age", "Age", meta::Mapping("Age", "Age")),
        meta::Field<::Athlete, std::string, &::Athlete::id_no, nullptr, nullptr, meta::Prop::Serializable>("std::string", "id_no", "ID_NO", meta::Mapping("ID_NO", "ID_NO")),
        meta::Field<::Athlete, bool, &::Athlete::inactive, nullptr, nullptr, meta::Prop::Serializable>("bool", "inactive", "Inactive", meta::Mapping("Inactive", "Inactive"))
    );
    inline static constexpr auto tableName = "ATHLETE";
    inline static constexpr auto query = "SELECT athlete, team1, last, first, initial, sex, birth, age, id_no, inactive FROM ATHLETE";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Athlete> {
    static inline const auto& fields = ::my::ns::Athlete::fields;
    static constexpr auto tableName = ::my::ns::Athlete::tableName;
    static constexpr auto query = ::my::ns::Athlete::query;
};
}  // namespace meta