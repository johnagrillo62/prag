#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct Entry {
    Entry() = default;

    int meet;
    int athlete;
    std::string ind_or_real;
    int team;
    std::string course;
    int score;
    std::string ex;
    int mt_event;
    std::string misc;
    int entry;
    std::string division;
    int heat;
    int lane;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Entry, int, &::Entry::meet, nullptr, nullptr, meta::Prop::Serializable>("int", "meet", "Meet", meta::Mapping("Meet", "Meet")),
        meta::Field<::Entry, int, &::Entry::athlete, nullptr, nullptr, meta::Prop::Serializable>("int", "athlete", "Athlete", meta::Mapping("Athlete", "Athlete")),
        meta::Field<::Entry, std::string, &::Entry::ind_or_real, nullptr, nullptr, meta::Prop::Serializable>("std::string", "ind_or_real", "I_R", meta::Mapping("I_R", "I_R")),
        meta::Field<::Entry, int, &::Entry::team, nullptr, nullptr, meta::Prop::Serializable>("int", "team", "Team", meta::Mapping("Team", "Team")),
        meta::Field<::Entry, std::string, &::Entry::course, nullptr, nullptr, meta::Prop::Serializable>("std::string", "course", "Course", meta::Mapping("Course", "Course")),
        meta::Field<::Entry, int, &::Entry::score, nullptr, nullptr, meta::Prop::Serializable>("int", "score", "Score", meta::Mapping("Score", "Score")),
        meta::Field<::Entry, std::string, &::Entry::ex, nullptr, nullptr, meta::Prop::Serializable>("std::string", "ex", "Ex", meta::Mapping("Ex", "Ex")),
        meta::Field<::Entry, int, &::Entry::mt_event, nullptr, nullptr, meta::Prop::Serializable>("int", "mt_event", "MtEvent", meta::Mapping("MtEvent", "MtEvent")),
        meta::Field<::Entry, std::string, &::Entry::misc, nullptr, nullptr, meta::Prop::Serializable>("std::string", "misc", "Misc", meta::Mapping("Misc", "Misc")),
        meta::Field<::Entry, int, &::Entry::entry, nullptr, nullptr, meta::Prop::Serializable>("int", "entry", "Entry", meta::Mapping("Entry", "Entry")),
        meta::Field<::Entry, std::string, &::Entry::division, nullptr, nullptr, meta::Prop::Serializable>("std::string", "division", "Division", meta::Mapping("Division", "Division")),
        meta::Field<::Entry, int, &::Entry::heat, nullptr, nullptr, meta::Prop::Serializable>("int", "heat", "HEAT", meta::Mapping("HEAT", "HEAT")),
        meta::Field<::Entry, int, &::Entry::lane, nullptr, nullptr, meta::Prop::Serializable>("int", "lane", "LANE", meta::Mapping("LANE", "LANE"))
    );
    inline static constexpr auto tableName = "ENTRY";
    inline static constexpr auto query = "SELECT meet, athlete, ind_or_real, team, course, score, ex, mt_event, misc, entry, division, heat, lane FROM ENTRY";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Entry> {
    static inline const auto& fields = ::my::ns::Entry::fields;
    static constexpr auto tableName = ::my::ns::Entry::tableName;
    static constexpr auto query = ::my::ns::Entry::query;
};
}  // namespace meta