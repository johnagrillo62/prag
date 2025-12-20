#pragma once

#include <windows.h>

#include <chrono>
#include <concepts>
#include <iostream>
#include <memory>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>
#include <unordered_map>
#include <vector>

//#include "boost/hana.hpp"


namespace bhw
{
inline void removeBlanks(std::string& str)
{
    // Remove all whitespace characters
    std::erase_if(str, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}



inline SQLCHAR* ToSqlChar(const std::string& s) noexcept
{
    return reinterpret_cast<SQLCHAR*>(const_cast<char*>(s.c_str()));
}

auto GetBool(SQLHANDLE stmt, uint16_t col) -> bool;
auto GetChar(SQLHANDLE stmt, uint16_t col) -> uint8_t;
auto GetCurrency(SQLHANDLE stmt, uint16_t col) -> float;
auto GetDateTime(SQLHANDLE stmt, uint16_t col) -> std::tm;
auto GetInt(SQLHANDLE stmt, uint16_t col) -> uint32_t;
auto GetLong(SQLHANDLE stmt, uint16_t col) -> uint64_t;
auto GetShort(SQLHANDLE stmt, uint16_t col) -> uint16_t;
auto GetText(SQLHANDLE stmt, uint16_t col, SQLLEN len) -> std::string;
auto GetTimePoint(SQLHANDLE stmt, uint16_t col) -> std::chrono::system_clock::time_point;
auto GetDouble(SQLHANDLE stmt, uint16_t col) -> double;
auto GetFloat(SQLHANDLE stmt, uint16_t col) -> float;

auto GetCWeek(std::chrono::sys_days date, std::chrono::sys_days season_start) -> int;
auto GetCWeek(std::chrono::system_clock::time_point timepoint) -> int;
auto GetISOWeek(std::chrono::system_clock::time_point timepoint) -> uint64_t;

template <typename... Ts> constexpr void Unused([[maybe_unused]] const Ts&... /*unused*/) noexcept
{
}

auto DatetimeToChrono(const std::string& datetime_str) -> std::chrono::system_clock::time_point;

template <typename Type>
concept Dereferencable = requires(Type var) {
    {
        *var
    }
    -> std::same_as<std::add_lvalue_reference_t<typename std::remove_reference_t<decltype(*var)>>>;
    { static_cast<bool>(var) }; // ensures we can check if (!t)
};

// Always returns a reference to the dereferenced object
template <typename T>
    requires Dereferencable<std::remove_reference_t<T>>
auto MustDeref(T&& ptr, const char* file = __FILE__, int line = __LINE__) -> decltype(auto)
{
    if (!static_cast<bool>(ptr))
    {
        std::cerr << "Null pointer at " << file << ":" << line << "\n";
        std::terminate();
    }

    using DerefType = decltype(*ptr);
    return static_cast<DerefType>(*ptr); // preserves reference type
}

template <typename Factory> class LazyCreator final
{
  public:
    using result_type = decltype(std::declval<const Factory&>()());
    constexpr LazyCreator(Factory&& factory) : factory_(std::move(factory))
    {
    }
    constexpr operator result_type() const
    {
        return factory_();
    }

  private:
    Factory factory_;
};

template <typename Factory> static auto LazyCreate(Factory&& factory) -> LazyCreator<Factory>
{
    return LazyCreator<Factory>(std::forward<Factory>(factory));
}

template <typename T> auto ReadUniquePtr(const SQLHANDLE& stmt) -> std::unique_ptr<T>;



template <typename Container, typename T>
auto FetchRowsUnique(const SQLHANDLE conn, const char* query) -> std::unique_ptr<Container>
{

    auto collection = std::make_unique<Container>(); // Create unique_ptr for the container
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    const auto ret = SQLExecDirectA(stmt, ToSqlChar(query), SQL_NTS);
    if (ret == -1)
    {
        SQLCHAR sqlState[6];  // NOLINT(modernize-avoid-c-arrays, readability-magic-numbers)
        SQLCHAR message[256]; // NOLINT(modernize-avoid-c-arrays, readability-magic-numbers)

        SQLINTEGER nativeError;
        SQLSMALLINT messageLength;
        constexpr SQLSMALLINT hType = SQL_HANDLE_STMT;
        // Get diagnostic record
        SQLGetDiagRecA(
            hType, conn, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
        std::cerr << "SQLState: " << sqlState << "\n"
                  << "NativeError: " << nativeError << "\n"
                  << "Message: " << message << "\n";
    }

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        while (SQLFetch(stmt) == SQL_SUCCESS)
        {
            auto obj = ReadUniquePtr<T>(stmt);
            if constexpr (std::is_same_v<Container, std::vector<std::unique_ptr<T>>>)
            {
                collection->push_back(std::move(obj));
            }
            else if constexpr (std::is_same_v<Container,
                                              std::unordered_map<uint64_t, std::unique_ptr<T>>>)
            {
                collection->operator[](obj->getKey()) = std::move(obj);
            }
        }
    }
    return collection; // Return unique_ptr
}
// Helper function to execute the query and fetch results.
template <typename Container, typename T>
auto FetchRows(const SQLHANDLE conn, const char* query) -> Container
{
    Container collection{};
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    auto ret = SQLExecDirectA(stmt, ToSqlChar(query), SQL_NTS);

    if (ret == -1)
    {
        SQLCHAR sqlState[6];  // NOLINT(modernize-avoid-c-arrays, readability-magic-numbers)
        SQLCHAR message[256]; // NOLINT(modernize-avoid-c-arrays, readability-magic-numbers)
        SQLINTEGER nativeError;
        SQLSMALLINT messageLength;
        constexpr SQLSMALLINT hType = SQL_HANDLE_STMT;
        // Get diagnostic record
        SQLGetDiagRecA(
            hType, conn, 1, sqlState, &nativeError, message, sizeof(message), &messageLength);
        std::cerr << "SQLState: " << sqlState << "\n"
                  << "NativeError: " << nativeError << "\n"
                  << "Message: " << message << "\n";
    }

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        while (SQLFetch(stmt) == SQL_SUCCESS)
        {
            auto obj = ReadUniquePtr<T>(stmt);
            if constexpr (std::is_same_v<Container, std::vector<std::unique_ptr<T>>>)
            {
                collection.push_back(std::move(obj));
            }
            else if constexpr (std::is_same_v<Container,
                                              std::unordered_map<uint64_t, std::unique_ptr<T>>>)
            {
                collection[obj->getKey()] = std::move(obj);
            }
        }
    }
    return collection;
}

template <typename T> auto ReadObj(const SQLHANDLE& stmt) -> T;

// Helper function to execute the query and fetch results.
template <typename Container, typename T>
auto FetchRowsObj(const SQLHANDLE conn, const char* query) -> Container
{
    Container collection{};
    SQLHANDLE stmt{};
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    auto ret = SQLExecDirectA(stmt, ToSqlChar(query), SQL_NTS);
    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        while (SQLFetch(stmt) == SQL_SUCCESS)
        {
            auto obj = ReadObj<T>(stmt);
            if constexpr (std::is_same_v<Container, std::vector<std::unique_ptr<T>>>)
            {
                collection.push_back(std::move(obj));
            }
            else if constexpr (std::is_same_v<Container,
                                              std::unordered_map<uint64_t, std::unique_ptr<T>>>)
            {
                collection[obj->getKey()] = std::move(obj);
            }
        }
    }
    return collection;
}

// Helper function to execute the query and fetch results.
template <typename Container, typename T>
auto FetchRowsObjUnique(const SQLHANDLE conn, const char* query) -> std::unique_ptr<Container>
{
    auto collection = std::make_unique<Container>();
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    auto ret = SQLExecDirectA(stmt, ToSqlChar(query), SQL_NTS);

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        while (SQLFetch(stmt) == SQL_SUCCESS)
        {
            auto obj = ReadObj<T>(stmt);
            if constexpr (std::is_same_v<Container, std::vector<T>>)
            {
                collection->push_back(std::move(obj));
            }
            else if constexpr (std::is_same_v<Container, std::unordered_map<uint64_t, T>>)
            {
                collection->emplace(obj.getKey(), std::move(obj));
            }
        }
    }

    return collection;
}
/*
template <typename T> std::string serialize(const T& obj)
{
    std::ostringstream oss;

    // Iterate over each (field name, getter function) pair
    boost::hana::for_each(
        boost::hana::accessors<T>(),
        [&](const auto& pair)
        {
            constexpr auto field_name =
                boost::hana::to<const char*>(boost::hana::first(pair)); // compile-time string
            const auto& getter = boost::hana::second(pair);             // getter function

            oss << field_name << " = ";

            const auto& value = getter(obj);

            using FieldType = std::decay_t<decltype(value)>;

            if constexpr (std::is_same_v<FieldType, std::string>)
            {
                oss << value;
            }
            else if constexpr (std::is_same_v<FieldType, std::chrono::system_clock::time_point>)
            {
                oss << std::chrono::duration_cast<std::chrono::milliseconds>(
                           value.time_since_epoch())
                           .count();
            }
            else
            {
                oss << value;
            }

            oss << "\n";
        });

    return oss.str();
}



template <typename T> std::unique_ptr<T> ReadObjTemplateUnique(SQLHANDLE stmt)
{
    auto obj = std::make_unique<T>();
    uint16_t fieldIndex = 1; // ODBC column indices start from 1

    boost::hana::for_each(boost::hana::accessors<T>(),
                          [&](auto pair)
                          {
                              auto setter = boost::hana::second(pair); // Setter function
                              using MemberType = std::remove_cvref_t<decltype(setter(*obj))>;

                              setter(*obj) = GetColumnValue<MemberType>(stmt, fieldIndex);
                              ++fieldIndex;
                          });

    return obj;
}




template <typename T> T ReadObjTemplate(SQLHANDLE stmt)
{
    T obj{};
    uint16_t fieldIndex = 1; // ODBC column indices start from 1

    boost::hana::for_each(boost::hana::accessors<T>(),
                   [&](auto pair)
                   {
                       auto setter = boost::hana::second(pair); // Setter function
                       using MemberType = std::remove_cvref_t<decltype(setter(obj))>;

                       setter(obj) = GetColumnValue<MemberType>(stmt, fieldIndex);
                       ++fieldIndex;
                   });

    return obj;
}


template <typename Container, typename T>
std::unique_ptr<Container> FetchRowsUniqueTemplate(const SQLHANDLE& conn, const char* query)
{
    auto collection = std::make_unique<Container>();
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

    const SQLRETURN ret =
        SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(query)), SQL_NTS);

    if (ret == SQL_ERROR)
    {
        SQLCHAR sqlState[6], message[256];
        SQLINTEGER nativeError;
        SQLSMALLINT messageLength;
        SQLGetDiagRecA(SQL_HANDLE_STMT,
                       conn,
                       1,
                       sqlState,
                       &nativeError,
                       message,
                       sizeof(message),
                       &messageLength);

        std::cerr << "SQLState: " << sqlState << "\n"
                  << "NativeError: " << nativeError << "\n"
                  << "Message: " << message << "\n";

        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        return collection;
    }

    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO)
    {
        while (SQLFetch(stmt) == SQL_SUCCESS)
        {
            auto obj = ReadObjTemplateUnique<T>(stmt);

            if constexpr (std::is_same_v<Container, std::vector<std::unique_ptr<T>>>)
                collection->push_back(std::move(obj));
            else if constexpr (std::is_same_v<Container,
                                              std::unordered_map<uint64_t, std::unique_ptr<T>>>)
                (*collection)[obj->getKey()] = std::move(obj);
        }
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
    return collection;
}
*/

} // namespace bhw
