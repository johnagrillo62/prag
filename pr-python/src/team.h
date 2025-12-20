#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct Team {
    Team() = default;

    int team;
    std::string t_code;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Team, int, &::Team::team, nullptr, nullptr, meta::Prop::Serializable>("int", "team", "Team", meta::Mapping("Team", "Team")),
        meta::Field<::Team, std::string, &::Team::t_code, nullptr, nullptr, meta::Prop::Serializable>("std::string", "t_code", "TCode", meta::Mapping("TCode", "TCode"))
    );
    inline static constexpr auto tableName = "TEAM";
    inline static constexpr auto query = "SELECT team, t_code FROM TEAM";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Team> {
    static inline const auto& fields = ::my::ns::Team::fields;
    static constexpr auto tableName = ::my::ns::Team::tableName;
    static constexpr auto query = ::my::ns::Team::query;
};
}  // namespace meta