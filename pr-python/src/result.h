#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct Result {
    Result() = default;

    int meet;
    int athlete;
    std::string ind_or_rel;
    int team;
    int score;
    std::string exhibit;
    int nt;
    int result;
    int age;
    int distance;
    int stroke;
    int mt_event;
    int points;
    int place;
    std::string course;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Result, int, &::Result::meet, nullptr, nullptr, meta::Prop::Serializable>("int", "meet", "MEET", meta::Mapping("MEET", "MEET")),
        meta::Field<::Result, int, &::Result::athlete, nullptr, nullptr, meta::Prop::Serializable>("int", "athlete", "ATHLETE", meta::Mapping("ATHLETE", "ATHLETE")),
        meta::Field<::Result, std::string, &::Result::ind_or_rel, nullptr, nullptr, meta::Prop::Serializable>("std::string", "ind_or_rel", "I_R", meta::Mapping("I_R", "I_R")),
        meta::Field<::Result, int, &::Result::team, nullptr, nullptr, meta::Prop::Serializable>("int", "team", "TEAM", meta::Mapping("TEAM", "TEAM")),
        meta::Field<::Result, int, &::Result::score, nullptr, nullptr, meta::Prop::Serializable>("int", "score", "SCORE", meta::Mapping("SCORE", "SCORE")),
        meta::Field<::Result, std::string, &::Result::exhibit, nullptr, nullptr, meta::Prop::Serializable>("std::string", "exhibit", "EX", meta::Mapping("EX", "EX")),
        meta::Field<::Result, int, &::Result::nt, nullptr, nullptr, meta::Prop::Serializable>("int", "nt", "NT", meta::Mapping("NT", "NT")),
        meta::Field<::Result, int, &::Result::result, nullptr, nullptr, meta::Prop::Serializable>("int", "result", "RESULT", meta::Mapping("RESULT", "RESULT")),
        meta::Field<::Result, int, &::Result::age, nullptr, nullptr, meta::Prop::Serializable>("int", "age", "AGE", meta::Mapping("AGE", "AGE")),
        meta::Field<::Result, int, &::Result::distance, nullptr, nullptr, meta::Prop::Serializable>("int", "distance", "DISTANCE", meta::Mapping("DISTANCE", "DISTANCE")),
        meta::Field<::Result, int, &::Result::stroke, nullptr, nullptr, meta::Prop::Serializable>("int", "stroke", "STROKE", meta::Mapping("STROKE", "STROKE")),
        meta::Field<::Result, int, &::Result::mt_event, nullptr, nullptr, meta::Prop::Serializable>("int", "mt_event", "MTEVENT", meta::Mapping("MTEVENT", "MTEVENT")),
        meta::Field<::Result, int, &::Result::points, nullptr, nullptr, meta::Prop::Serializable>("int", "points", "POINTS", meta::Mapping("POINTS", "POINTS")),
        meta::Field<::Result, int, &::Result::place, nullptr, nullptr, meta::Prop::Serializable>("int", "place", "PLACE", meta::Mapping("PLACE", "PLACE")),
        meta::Field<::Result, std::string, &::Result::course, nullptr, nullptr, meta::Prop::Serializable>("std::string", "course", "COURSE", meta::Mapping("COURSE", "COURSE"))
    );
    inline static constexpr auto tableName = "RESULT";
    inline static constexpr auto query = "SELECT meet, athlete, ind_or_rel, team, score, exhibit, nt, result, age, distance, stroke, mt_event, points, place, course FROM RESULT";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Result> {
    static inline const auto& fields = ::my::ns::Result::fields;
    static constexpr auto tableName = ::my::ns::Result::tableName;
    static constexpr auto query = ::my::ns::Result::query;
};
}  // namespace meta