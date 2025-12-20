#include "util.h"

//#define WIN32_LEAN_AND_MEAN
//#include <windows.h> // IWYU pragma: keep

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <ranges>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>
#include <sstream>
#include <vector>
namespace
{

auto stripSpaces(std::string &str)
{
  // Find the first non-space character from the start
  auto start = std::ranges::find_if_not(str, ::isspace);
  // If no non-space character is found, the string is all spaces
  if (start == str.end())
  {
    str.clear();
    return;
  }

  // Find the first non-space character from the end
  auto end = std::ranges::find_if_not(std::ranges::reverse_view(str), ::isspace).base();

  // Erase the leading and trailing spaces
  str = std::string(start, end);
}
} // namesp

auto bhw::GetCWeek(std::chrono::sys_days date, std::chrono::sys_days season_start) -> int
{
    auto diff = date - season_start;
    auto days = std::chrono::duration_cast<std::chrono::days>(diff).count();
    return static_cast<int>(days / 7) + 1; // Week numbers start at 1
}

auto bhw::GetCWeek(std::chrono::system_clock::time_point timepoint) -> int
{
    using namespace std::chrono;
    auto daysSinceEpoch = duration_cast<days>(timepoint.time_since_epoch()).count();
    return static_cast<int>(daysSinceEpoch / 7); // one week = 7 days
}

auto bhw::GetISOWeek(std::chrono::system_clock::time_point timepoint) -> uint64_t
{
    using namespace std::chrono;

    // Convert to just the date (floor to days)
    sys_days const date = floor<days>(timepoint);

    // Get weekday (Monday = 1, Sunday = 7)
    weekday const wkday{date};
    auto const current_weekday = wkday.c_encoding() == 0 ? 7 : wkday.c_encoding();

    // Find Thursday of this week
    sys_days const thisThursday = date + days{4 - current_weekday};

    // Year of that Thursday
    year const isoYear = year_month_day{thisThursday}.year();

    // Get January 4th of that year (always in ISO week 1)
    sys_days const jan4 = sys_days{isoYear / January / 4};
    weekday const jan4_wd{jan4};
    auto const jan4_weekday = jan4_wd.c_encoding() == 0 ? 7 : jan4_wd.c_encoding();

    // Find Monday of ISO week 1
    sys_days const week1_monday = jan4 - days{jan4_weekday - 1};

    // Compute week number
    int const isoWeek = static_cast<int>((date - week1_monday).count() / 7) + 1;
    return isoWeek;
}


auto bhw::DatetimeToChrono(const std::string &datetime_str) -> std::chrono::system_clock::time_point
{
  using namespace std::chrono;

  std::istringstream iss(datetime_str);
  sys_time<std::chrono::seconds> timepoint;

  // Format: 2025-04-11 14:30:00
  iss >> parse("%F %T", timepoint);

  if (iss.fail())
  {
    // throw std::runtime_error("Failed to parse datetime string: " + datetime_str);
  }
  return timepoint;
}

auto bhw::GetText(SQLHANDLE stmt, uint16_t col, SQLLEN len) -> std::string
{
  std::vector<char> buffer(len);
  SQLLEN indicator; // To check if the value is NULL
  SQLGetData(stmt, col, SQL_C_CHAR, buffer.data(), len, &indicator);
  auto str = std::string(buffer.data(), len);
  std::erase_if(str, [](char chr) { return chr == '\0'; });
  stripSpaces(str);
  return str;
}

auto bhw::GetLong(SQLHANDLE stmt, uint16_t col) -> uint64_t
{
  uint64_t value = 0;
  SQLGetData(stmt, col, SQL_C_UBIGINT, &value, sizeof(value), nullptr);
  return value;
}

auto bhw::GetInt(SQLHANDLE stmt, uint16_t col) -> uint32_t
{
  SQLINTEGER value{};
  SQLGetData(stmt, col, SQL_C_LONG, &value, sizeof(value), nullptr);
  return value;
}

auto bhw::GetShort(SQLHANDLE stmt, uint16_t col) -> uint16_t
{
  SQLSMALLINT value{};
  SQLGetData(stmt, col, SQL_C_SHORT, &value, sizeof(value), nullptr);
  return value;
}

auto bhw::GetChar(SQLHANDLE stmt, uint16_t col) -> uint8_t
{
  SQLCHAR value{};
  SQLGetData(stmt, col, SQL_C_CHAR, &value, sizeof(value), nullptr);
  return value;
}

auto bhw::GetBool(SQLHANDLE stmt, uint16_t  col) -> bool
{
  int boolValue{};
  SQLLEN indPtr;

  SQLRETURN const ret = SQLGetData(stmt, col, SQL_C_LONG, &boolValue, 0, &indPtr);
  Unused(ret);
  return (boolValue != 0); // Convert integer to boolean
}

auto bhw::GetDateTime(SQLHANDLE stmt, uint16_t col) -> std::tm
{
  std::tm time{};

  SQL_TIMESTAMP_STRUCT timestampValue{};
  SQLLEN indPtr; // Indicator variable for SQL_NULL_DATA

  SQLRETURN const ret = SQLGetData(stmt, col, SQL_C_TYPE_TIMESTAMP, &timestampValue, 0, &indPtr);
  Unused(ret);
  // if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {/
  //	if (indPtr == SQL_NULL_DATA) {
  //			// Handle SQL NULL data (when the value is NULL)//
  //	std::cout << "Column " << col << " contains NULL." << std::endl;
  //	}
  // else {
  // Convert SQL_TIMESTAMP_STRUCT to std::tm
  time.tm_year = timestampValue.year - 1900; // tm_year is years since 1900
  time.tm_mon = timestampValue.month - 1;    // tm_mon is 0-based (0 = January)
  time.tm_mday = timestampValue.day;
  time.tm_hour = timestampValue.hour;
  time.tm_min = timestampValue.minute;
  time.tm_sec = timestampValue.second;
  time.tm_isdst = -1; // Let mktime handle daylight saving time
                      //}
  //}
  // else {
  //		std::cerr << "Error retrieving data from SQL" << std::endl;
  //	}

  return time;
}

auto bhw::GetTimePoint(SQLHANDLE stmt, uint16_t col) -> std::chrono::system_clock::time_point
{
  SQL_TIMESTAMP_STRUCT timestampValue{};
  SQLLEN indPtr{};

  SQLRETURN const ret = SQLGetData(stmt, col, SQL_C_TYPE_TIMESTAMP, &timestampValue, sizeof(timestampValue), &indPtr);

  if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
  {
    return {};
    // std::cerr << "Error retrieving data from SQL" << '\n';
    // throw std::runtime_error("SQLGetData failed");  // NOLINT(clang-diagnostic-error)
  }

  if (indPtr == SQL_NULL_DATA)
  {
    return std::chrono::system_clock::time_point{}; // epoch
  }

  std::tm time{};
  time.tm_year = timestampValue.year - 1900;
  time.tm_mon = timestampValue.month - 1;
  time.tm_mday = timestampValue.day;
  time.tm_hour = timestampValue.hour;
  time.tm_min = timestampValue.minute;
  time.tm_sec = timestampValue.second;
  time.tm_isdst = -1;

  std::time_t const retTime = std::mktime(&time);
  return std::chrono::system_clock::from_time_t(retTime);
}

auto bhw::GetFloat(SQLHANDLE /*stmt*/, uint16_t /*col*/) -> float
{
  float constexpr val{};
  return val;
}
auto bhw::GetDouble(SQLHANDLE /*stmt*/, uint16_t /*col*/) -> double
{
  double constexpr val{};
  return val;
}

auto bhw::GetCurrency(SQLHANDLE /*stmt*/, uint16_t /*col*/) -> float
{
  float constexpr val{};
  return val;
}
