#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct MtEvent {
    MtEvent() = default;

    int meet;
    int mt_ev;
    int lo_hi;
    std::string course;
    int mt_event;
    int distance;
    int stroke;
    std::string sex;
    std::string ind_or_rel;
    std::string division;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::MtEvent, int, &::MtEvent::meet, nullptr, nullptr, meta::Prop::Serializable>("int", "meet", "Meet", meta::Mapping("Meet", "Meet")),
        meta::Field<::MtEvent, int, &::MtEvent::mt_ev, nullptr, nullptr, meta::Prop::Serializable>("int", "mt_ev", "MtEv", meta::Mapping("MtEv", "MtEv")),
        meta::Field<::MtEvent, int, &::MtEvent::lo_hi, nullptr, nullptr, meta::Prop::Serializable>("int", "lo_hi", "Lo_Hi", meta::Mapping("Lo_Hi", "Lo_Hi")),
        meta::Field<::MtEvent, std::string, &::MtEvent::course, nullptr, nullptr, meta::Prop::Serializable>("std::string", "course", "Course", meta::Mapping("Course", "Course")),
        meta::Field<::MtEvent, int, &::MtEvent::mt_event, nullptr, nullptr, meta::Prop::Serializable>("int", "mt_event", "MtEvent", meta::Mapping("MtEvent", "MtEvent")),
        meta::Field<::MtEvent, int, &::MtEvent::distance, nullptr, nullptr, meta::Prop::Serializable>("int", "distance", "Distance", meta::Mapping("Distance", "Distance")),
        meta::Field<::MtEvent, int, &::MtEvent::stroke, nullptr, nullptr, meta::Prop::Serializable>("int", "stroke", "Stroke", meta::Mapping("Stroke", "Stroke")),
        meta::Field<::MtEvent, std::string, &::MtEvent::sex, nullptr, nullptr, meta::Prop::Serializable>("std::string", "sex", "Sex", meta::Mapping("Sex", "Sex")),
        meta::Field<::MtEvent, std::string, &::MtEvent::ind_or_rel, nullptr, nullptr, meta::Prop::Serializable>("std::string", "ind_or_rel", "I_R", meta::Mapping("I_R", "I_R")),
        meta::Field<::MtEvent, std::string, &::MtEvent::division, nullptr, nullptr, meta::Prop::Serializable>("std::string", "division", "Division", meta::Mapping("Division", "Division"))
    );
    inline static constexpr auto tableName = "MTEVENT";
    inline static constexpr auto query = "SELECT meet, mt_ev, lo_hi, course, mt_event, distance, stroke, sex, ind_or_rel, division FROM MTEVENT";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::MtEvent> {
    static inline const auto& fields = ::my::ns::MtEvent::fields;
    static constexpr auto tableName = ::my::ns::MtEvent::tableName;
    static constexpr auto query = ::my::ns::MtEvent::query;
};
}  // namespace meta