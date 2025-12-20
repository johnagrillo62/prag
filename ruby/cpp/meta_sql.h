#pragma once
#include <windows.h>

#include <chrono>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <iostream>
#include <optional>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>
#include <utility>
#include <vector>
#include <iomanip>
#include <sstream>
#include <string_view>
#include <tuple>

namespace bhw
{

template <typename T>
concept SqlMappable = requires {
    typename T::Init;
    { T::fields } -> std::convertible_to<decltype(T::fields)>;
    { T::from(std::declval<typename T::Init>()) } -> std::same_as<T>;
    { std::declval<T>().getKey() } -> std::convertible_to<uint64_t>;
};


inline void removeBlanks(std::string& str)
{
    // Remove all whitespace characters
    std::erase_if(str, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
}

///
///  SQL 
///
template <typename T> T GetColumnValue(SQLHANDLE stmt, uint16_t col);

// Unsigned integer types
template <> inline uint64_t GetColumnValue<uint64_t>(SQLHANDLE stmt, uint16_t col)
{
    uint64_t value{};
    SQLGetData(stmt, col, SQL_C_UBIGINT, &value, sizeof(value), nullptr);
    return value;
}

template <> inline uint32_t GetColumnValue<uint32_t>(SQLHANDLE stmt, uint16_t col)
{
    SQLINTEGER value{};
    SQLGetData(stmt, col, SQL_C_LONG, &value, sizeof(value), nullptr);
    return value;
}

template <> inline uint16_t GetColumnValue<uint16_t>(SQLHANDLE stmt, uint16_t col)
{
    SQLSMALLINT value{};
    SQLGetData(stmt, col, SQL_C_SHORT, &value, sizeof(value), nullptr);
    return value;
}

// Boolean
template <> inline bool GetColumnValue<bool>(SQLHANDLE stmt, uint16_t col)
{
    int boolValue{};
    SQLLEN indPtr{};
    SQLGetData(stmt, col, SQL_C_LONG, &boolValue, 0, &indPtr);
    return boolValue != 0;
}

// Character (single byte)
template <> inline uint8_t GetColumnValue<uint8_t>(SQLHANDLE stmt, uint16_t col)
{
    SQLCHAR value{};
    SQLGetData(stmt, col, SQL_C_CHAR, &value, sizeof(value), nullptr);
    return static_cast<uint8_t>(value);
}

// Floating point
template <> inline float GetColumnValue<float>(SQLHANDLE stmt, uint16_t col)
{
    float value{};
    SQLGetData(stmt, col, SQL_C_FLOAT, &value, sizeof(value), nullptr);
    return value;
}

template <> inline double GetColumnValue<double>(SQLHANDLE stmt, uint16_t col)
{
    double value{};
    SQLGetData(stmt, col, SQL_C_DOUBLE, &value, sizeof(value), nullptr);
    return value;
}

// std::string

template <> inline std::string GetColumnValue<std::string>(SQLHANDLE stmt, uint16_t col)
{
    constexpr size_t bufferSize = 256;
    std::vector<char> buffer(bufferSize);
    SQLLEN indicator{}; // length of returned data

    // Fetch data from SQL

    if (SQLRETURN ret = SQLGetData(stmt, col, SQL_C_CHAR, buffer.data(), buffer.size(), &indicator);
        ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        return {}; // fetch failed
    }

    std::string str;

    if (indicator == SQL_NULL_DATA)
    {
        str = ""; // NULL -> empty string
    }
    else if (indicator < static_cast<SQLLEN>(buffer.size()))
    {
        str.assign(buffer.data(), static_cast<size_t>(indicator));
    }
    else
    {
        str.assign(buffer.data());
    }

    // Remove trailing nulls using C++20 ranges
    std::erase(str, '\0');

    // Remove leading and trailing spaces
    removeBlanks(str);
    return str;
}

// std::chrono::system_clock::time_point

template <>
inline std::chrono::system_clock::time_point GetColumnValue<std::chrono::system_clock::time_point>(
    SQLHANDLE stmt,
    uint16_t col)
{
    SQL_TIMESTAMP_STRUCT timestampValue{};
    SQLLEN indPtr{};
    SQLRETURN ret = SQLGetData(
        stmt, col, SQL_C_TYPE_TIMESTAMP, &timestampValue, sizeof(timestampValue), &indPtr);

    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        return {};
    if (indPtr == SQL_NULL_DATA)
        return {};

    std::tm time{};
    time.tm_year = timestampValue.year - 1900;
    time.tm_mon = timestampValue.month - 1;
    time.tm_mday = timestampValue.day;
    time.tm_hour = timestampValue.hour;
    time.tm_min = timestampValue.minute;
    time.tm_sec = timestampValue.second;
    time.tm_isdst = 0; // disable DST guessing

    // Use UTC version on Windows
    std::time_t t = _mkgmtime(&time);
    return std::chrono::system_clock::from_time_t(t);
}

template <typename T> class Generator
{
  public:
    struct promise_type
    {
        std::optional<T> current_value;
        std::exception_ptr exception;

        promise_type() noexcept = default;

        Generator get_return_object()
        {
            return Generator{std::coroutine_handle<promise_type>::from_promise(*this)};
        }
        std::suspend_always initial_suspend() noexcept
        {
            return {};
        }
        std::suspend_always final_suspend() noexcept
        {
            return {};
        }

        // Construct T only when yielding
        std::suspend_always yield_value(T value) noexcept(std::is_nothrow_move_constructible_v<T>)
        {
            current_value.emplace(std::move(value));
            return {};
        }

        void return_void() noexcept
        {
        }

        void unhandled_exception()
        {
            exception = std::current_exception();
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    explicit Generator(handle_type h) : coro_(h)
    {
    }
    Generator(const Generator&) = delete;
    Generator& operator=(const Generator&) = delete;

    Generator(Generator&& other) noexcept : coro_(other.coro_)
    {
        other.coro_ = {};
    }
    Generator& operator=(Generator&& other) noexcept
    {
        if (this != &other)
        {
            if (coro_)
                coro_.destroy();
            coro_ = other.coro_;
            other.coro_ = {};
        }
        return *this;
    }

    ~Generator()
    {
        if (coro_)
            coro_.destroy();
    }

    class iterator
    {
      public:
        using value_type = T;
        using reference = const T&;
        using pointer = const T*;
        using difference_type = std::ptrdiff_t;
        using iterator_category = std::input_iterator_tag;

        iterator() : coro_(), done_(true)
        {
        }
        explicit iterator(handle_type h) : coro_(h), done_(!h || h.done())
        {
        }

        iterator& operator++()
        {
            coro_.resume();
            if (coro_.promise().exception)
                std::rethrow_exception(coro_.promise().exception);
            done_ = coro_.done();
            return *this;
        }

        reference operator*() const
        {
            return *coro_.promise().current_value; // safe: set by yield_value
        }

        bool operator==(std::default_sentinel_t) const
        {
            return done_;
        }
        bool operator!=(std::default_sentinel_t) const
        {
            return !done_;
        }

      private:
        handle_type coro_;
        bool done_ = true;
    };

    iterator begin()
    {
        if (coro_)
        {
            coro_.resume();
            if (coro_.promise().exception)
                std::rethrow_exception(coro_.promise().exception);
        }
        return iterator{coro_};
    }
    std::default_sentinel_t end()
    {
        return {};
    }

  private:
    handle_type coro_{};
};

template <SqlMappable T, std::size_t... I>
T ReadObjImplT(const SQLHANDLE& stmt, std::index_sequence<I...>)
{
    // Get the tuple type
    using FieldsTuple = decltype(T::fields);

    // Get Init type
    using InitType = typename T::Init;

    // Construct Init using a fold-expression
    InitType init{(GetColumnValue<typename std::tuple_element<I, FieldsTuple>::type::type>(
        stmt, static_cast<uint16_t>(I) + 1))...};

    return T::from(std::move(init));
}

// Helper that deduces index sequence
template <SqlMappable T> T ReadObjT(const SQLHANDLE& stmt)
{
    return ReadObjImplT<T>(stmt,
                           std::make_index_sequence<std::tuple_size_v<decltype(T::fields)>>{});
}

// --- Coroutine fetch generator ---
template <SqlMappable T> Generator<T> FetchRowsGeneratorT(SQLHANDLE conn, const char* query)
{
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);

    const auto ret =
        SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(query)), SQL_NTS);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
    {
        std::cerr << "SQL query failed\n";
        SQLFreeHandle(SQL_HANDLE_STMT, stmt);
        co_return;
    }

    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        T obj = ReadObjT<T>(stmt);
        co_yield std::move(obj);
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}

} // namespace bhw
