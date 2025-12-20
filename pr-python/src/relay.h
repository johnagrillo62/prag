#include <cstdint>
#include <string>

namespace my {
namespace ns {
struct Relay {
    Relay() = default;

    int relay;
    int meet;
    int lo_hi;
    int team;
    std::string letter;
    int age_range;
    std::string sex;
    int ath1;
    int ath2;
    int ath3;
    int ath4;
    int ath5;
    int ath6;
    int ath7;
    int ath8;
    int distance;
    int stroke;
    std::string relay_age;

    // reflection meta tuples
    inline static const auto fields = std::make_tuple(
        meta::Field<::Relay, int, &::Relay::relay, nullptr, nullptr, meta::Prop::Serializable>("int", "relay", "RELAY", meta::Mapping("RELAY", "RELAY")),
        meta::Field<::Relay, int, &::Relay::meet, nullptr, nullptr, meta::Prop::Serializable>("int", "meet", "MEET", meta::Mapping("MEET", "MEET")),
        meta::Field<::Relay, int, &::Relay::lo_hi, nullptr, nullptr, meta::Prop::Serializable>("int", "lo_hi", "LO_HI", meta::Mapping("LO_HI", "LO_HI")),
        meta::Field<::Relay, int, &::Relay::team, nullptr, nullptr, meta::Prop::Serializable>("int", "team", "TEAM", meta::Mapping("TEAM", "TEAM")),
        meta::Field<::Relay, std::string, &::Relay::letter, nullptr, nullptr, meta::Prop::Serializable>("std::string", "letter", "LETTER", meta::Mapping("LETTER", "LETTER")),
        meta::Field<::Relay, int, &::Relay::age_range, nullptr, nullptr, meta::Prop::Serializable>("int", "age_range", "AGE_RANGE", meta::Mapping("AGE_RANGE", "AGE_RANGE")),
        meta::Field<::Relay, std::string, &::Relay::sex, nullptr, nullptr, meta::Prop::Serializable>("std::string", "sex", "SEX", meta::Mapping("SEX", "SEX")),
        meta::Field<::Relay, int, &::Relay::ath1, nullptr, nullptr, meta::Prop::Serializable>("int", "ath1", "ATH(1)", meta::Mapping("ATH(1)", "ATH(1)")),
        meta::Field<::Relay, int, &::Relay::ath2, nullptr, nullptr, meta::Prop::Serializable>("int", "ath2", "ATH(2)", meta::Mapping("ATH(2)", "ATH(2)")),
        meta::Field<::Relay, int, &::Relay::ath3, nullptr, nullptr, meta::Prop::Serializable>("int", "ath3", "ATH(3)", meta::Mapping("ATH(3)", "ATH(3)")),
        meta::Field<::Relay, int, &::Relay::ath4, nullptr, nullptr, meta::Prop::Serializable>("int", "ath4", "ATH(4)", meta::Mapping("ATH(4)", "ATH(4)")),
        meta::Field<::Relay, int, &::Relay::ath5, nullptr, nullptr, meta::Prop::Serializable>("int", "ath5", "ATH(5)", meta::Mapping("ATH(5)", "ATH(5)")),
        meta::Field<::Relay, int, &::Relay::ath6, nullptr, nullptr, meta::Prop::Serializable>("int", "ath6", "ATH(6)", meta::Mapping("ATH(6)", "ATH(6)")),
        meta::Field<::Relay, int, &::Relay::ath7, nullptr, nullptr, meta::Prop::Serializable>("int", "ath7", "ATH(7)", meta::Mapping("ATH(7)", "ATH(7)")),
        meta::Field<::Relay, int, &::Relay::ath8, nullptr, nullptr, meta::Prop::Serializable>("int", "ath8", "ATH(8)", meta::Mapping("ATH(8)", "ATH(8)")),
        meta::Field<::Relay, int, &::Relay::distance, nullptr, nullptr, meta::Prop::Serializable>("int", "distance", "DISTANCE", meta::Mapping("DISTANCE", "DISTANCE")),
        meta::Field<::Relay, int, &::Relay::stroke, nullptr, nullptr, meta::Prop::Serializable>("int", "stroke", "STROKE", meta::Mapping("STROKE", "STROKE")),
        meta::Field<::Relay, std::string, &::Relay::relay_age, nullptr, nullptr, meta::Prop::Serializable>("std::string", "relay_age", "RELAYAGE", meta::Mapping("RELAYAGE", "RELAYAGE"))
    );
    inline static constexpr auto tableName = "RELAY";
    inline static constexpr auto query = "SELECT relay, meet, lo_hi, team, letter, age_range, sex, ath1, ath2, ath3, ath4, ath5, ath6, ath7, ath8, distance, stroke, relay_age FROM RELAY";
};
} // namespace ns
} // namespace my
namespace meta {
template<> struct MetaTuple<::my::ns::Relay> {
    static inline const auto& fields = ::my::ns::Relay::fields;
    static constexpr auto tableName = ::my::ns::Relay::tableName;
    static constexpr auto query = ::my::ns::Relay::query;
};
}  // namespace meta