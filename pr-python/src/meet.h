#include <chrono>
#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct Meet {
    Meet() = default;

    int meet;
    std::string m_name;
    std::chrono::system_clock::time_point start;
    std::chrono::system_clock::time_point age_up;
    std::string course;
    std::string location;
    int max_ind_ent;
    int max_rel_ent;
    int max_ent;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Meet, int, &::Meet::meet, nullptr, nullptr, meta::Prop::Serializable>("int", "meet", "Meet", meta::Mapping("Meet", "Meet")),
        meta::Field<::Meet, std::string, &::Meet::m_name, nullptr, nullptr, meta::Prop::Serializable>("std::string", "m_name", "MName", meta::Mapping("MName", "MName")),
        meta::Field<::Meet, std::chrono::system_clock::time_point, &::Meet::start, nullptr, nullptr, meta::Prop::Serializable>("std::chrono::system_clock::time_point", "start", "Start", meta::Mapping("Start", "Start")),
        meta::Field<::Meet, std::chrono::system_clock::time_point, &::Meet::age_up, nullptr, nullptr, meta::Prop::Serializable>("std::chrono::system_clock::time_point", "age_up", "AgeUp", meta::Mapping("AgeUp", "AgeUp")),
        meta::Field<::Meet, std::string, &::Meet::course, nullptr, nullptr, meta::Prop::Serializable>("std::string", "course", "Course", meta::Mapping("Course", "Course")),
        meta::Field<::Meet, std::string, &::Meet::location, nullptr, nullptr, meta::Prop::Serializable>("std::string", "location", "Location", meta::Mapping("Location", "Location")),
        meta::Field<::Meet, int, &::Meet::max_ind_ent, nullptr, nullptr, meta::Prop::Serializable>("int", "max_ind_ent", "MaxIndEnt", meta::Mapping("MaxIndEnt", "MaxIndEnt")),
        meta::Field<::Meet, int, &::Meet::max_rel_ent, nullptr, nullptr, meta::Prop::Serializable>("int", "max_rel_ent", "MaxRelEnt", meta::Mapping("MaxRelEnt", "MaxRelEnt")),
        meta::Field<::Meet, int, &::Meet::max_ent, nullptr, nullptr, meta::Prop::Serializable>("int", "max_ent", "MaxEnt", meta::Mapping("MaxEnt", "MaxEnt"))
    );
    inline static constexpr auto tableName = "MEET";
    inline static constexpr auto query = "SELECT meet, m_name, start, age_up, course, location, max_ind_ent, max_rel_ent, max_ent FROM MEET";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Meet> {
    static inline const auto& fields = ::my::ns::Meet::fields;
    static constexpr auto tableName = ::my::ns::Meet::tableName;
    static constexpr auto query = ::my::ns::Meet::query;
};
}  // namespace meta