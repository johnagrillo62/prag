#pragma once
#include <windows.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <coroutine>
#include <cstdint>
#include <exception>
#include <memory>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "immutable.h"
#include "meta_field.h"

//-------------------- GET COLUMN VALUE --------------------
template <typename T> T inline GetColumnValue(SQLHANDLE& stmt, uint16_t col)
{
    static_assert(sizeof(T) == 0, "GetColumnValue<T> not implemented for this type");
    return {};
}

// Unsigned integer types
template <> inline uint64_t GetColumnValue<uint64_t>(SQLHANDLE& stmt, uint16_t col)
{
    uint64_t value{};
    SQLGetData(stmt, col, SQL_C_UBIGINT, &value, sizeof(value), nullptr);
    return value;
}

template <> inline uint32_t GetColumnValue<uint32_t>(SQLHANDLE& stmt, uint16_t col)
{
    SQLINTEGER value{};
    SQLGetData(stmt, col, SQL_C_LONG, &value, sizeof(value), nullptr);
    return value;
}

template <> inline uint16_t GetColumnValue<uint16_t>(SQLHANDLE& stmt, uint16_t col)
{
    SQLSMALLINT value{};
    SQLGetData(stmt, col, SQL_C_SHORT, &value, sizeof(value), nullptr);
    return value;
}

// Boolean
template <> inline bool GetColumnValue<bool>(SQLHANDLE& stmt, uint16_t col)
{
    int boolValue{};
    SQLLEN indPtr{};
    SQLGetData(stmt, col, SQL_C_LONG, &boolValue, 0, &indPtr);
    return boolValue != 0;
}

// Floating point
template <> inline float GetColumnValue<float>(SQLHANDLE& stmt, uint16_t col)
{
    float value{};
    SQLGetData(stmt, col, SQL_C_FLOAT, &value, sizeof(value), nullptr);
    return value;
}

template <> inline double GetColumnValue<double>(SQLHANDLE& stmt, uint16_t col)
{
    double value{};
    SQLGetData(stmt, col, SQL_C_DOUBLE, &value, sizeof(value), nullptr);
    return value;
}

// std::string
template <> inline std::string GetColumnValue<std::string>(SQLHANDLE& stmt, uint16_t col)
{
    constexpr size_t bufferSize = 256;
    std::vector<char> buffer(bufferSize);
    SQLLEN indicator{};
    SQLRETURN ret = SQLGetData(stmt, col, SQL_C_CHAR, buffer.data(), buffer.size(), &indicator);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        return {};

    std::string str;
    if (indicator == SQL_NULL_DATA)
        str = "";
    else if (indicator < static_cast<SQLLEN>(buffer.size()))
        str.assign(buffer.data(), static_cast<size_t>(indicator));
    else
        str.assign(buffer.data());

    std::erase(str, '\0');
    std::erase_if(str, [](char c) { return std::isspace(static_cast<unsigned char>(c)); });
    return str;
}

// ImmutableString wrapper
template <> inline ImmutableString GetColumnValue<ImmutableString>(SQLHANDLE& stmt, uint16_t col)
{
    return ImmutableString(GetColumnValue<std::string>(stmt, col));
}

// std::chrono::system_clock::time_point
template <>
inline std::chrono::system_clock::time_point GetColumnValue<std::chrono::system_clock::time_point>(
    SQLHANDLE& stmt,
    uint16_t col)
{
    SQL_TIMESTAMP_STRUCT timestamp{};
    SQLLEN ind{};
    SQLRETURN ret =
        SQLGetData(stmt, col, SQL_C_TYPE_TIMESTAMP, &timestamp, sizeof(timestamp), &ind);
    if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
        return {};
    if (ind == SQL_NULL_DATA)
        return {};

    std::tm time{};
    time.tm_year = timestamp.year - 1900;
    time.tm_mon = timestamp.month - 1;
    time.tm_mday = timestamp.day;
    time.tm_hour = timestamp.hour;
    time.tm_min = timestamp.minute;
    time.tm_sec = timestamp.second;
    time.tm_isdst = 0;

    std::time_t t = _mkgmtime(&time);
    return std::chrono::system_clock::from_time_t(t);
}
// -------------------- Explicit specializations for immutable Field<T, FieldMutability::Immutable>
// --------------------

// Unsigned integer types
template <>
inline Field<uint64_t, FieldMutability::Immutable> GetColumnValue<
    Field<uint64_t, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<uint64_t, FieldMutability::Immutable>(GetColumnValue<uint64_t>(stmt, col));
}

template <>
inline Field<uint32_t, FieldMutability::Immutable> GetColumnValue<
    Field<uint32_t, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<uint32_t, FieldMutability::Immutable>(GetColumnValue<uint32_t>(stmt, col));
}

template <>
inline Field<uint16_t, FieldMutability::Immutable> GetColumnValue<
    Field<uint16_t, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<uint16_t, FieldMutability::Immutable>(GetColumnValue<uint16_t>(stmt, col));
}

// Boolean
template <>
inline Field<bool, FieldMutability::Immutable> GetColumnValue<
    Field<bool, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<bool, FieldMutability::Immutable>(GetColumnValue<bool>(stmt, col));
}

// Floating point types
template <>
inline Field<float, FieldMutability::Immutable> GetColumnValue<
    Field<float, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<float, FieldMutability::Immutable>(GetColumnValue<float>(stmt, col));
}

template <>
inline Field<double, FieldMutability::Immutable> GetColumnValue<
    Field<double, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<double, FieldMutability::Immutable>(GetColumnValue<double>(stmt, col));
}

// std::string
template <>
inline Field<std::string, FieldMutability::Immutable> GetColumnValue<
    Field<std::string, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<std::string, FieldMutability::Immutable>(GetColumnValue<std::string>(stmt, col));
}

// ImmutableString
template <>
inline Field<ImmutableString, FieldMutability::Immutable> GetColumnValue<
    Field<ImmutableString, FieldMutability::Immutable>>(SQLHANDLE& stmt, uint16_t col)
{
    return Field<ImmutableString, FieldMutability::Immutable>(
        GetColumnValue<ImmutableString>(stmt, col));
}

// std::chrono::system_clock::time_point
template <>
inline Field<std::chrono::system_clock::time_point, FieldMutability::Immutable> GetColumnValue<
    Field<std::chrono::system_clock::time_point, FieldMutability::Immutable>>(SQLHANDLE& stmt,
                                                                              uint16_t col)
{
    return Field<std::chrono::system_clock::time_point, FieldMutability::Immutable>(
        GetColumnValue<std::chrono::system_clock::time_point>(stmt, col));
}

// Field wrapper for meta-fields
template <typename T, FieldMutability M>
inline Field<T, M> GetColumnValue(SQLHANDLE& stmt, uint16_t col)
{
    return {};
    // T raw = GetColumnValue<T>(stmt, col); // fetch raw type
    // return Field<T, M>(std::move(raw));
}

//-------------------- RAW TYPE TRAIT --------------------
template <typename T> struct RawTypeTrait
{
    using type = T;
};
template <typename T> struct RawTypeTrait<ImmutableClass<T>>
{
    using type = T;
};

//-------------------- WRAPPER TYPE --------------------
template <typename T> struct WrapperType
{
    using Raw = typename RawTypeTrait<T>::type;
    using Type = T;

    static std::unique_ptr<T> wrap(Raw&& raw)
    {
        if constexpr (std::is_same_v<T, ImmutableClass<Raw>>)
        {
            return std::make_unique<T>(std::move(raw));
        }
        else
        {
            return std::make_unique<T>(std::move(raw));
        }
    }
};
//-------------------- READ OBJECT --------------------
template <typename T, std::size_t... I> T ReadObjImplT(SQLHANDLE& stmt, std::index_sequence<I...>)
{
    using FieldsTuple = decltype(T::fields);

    return T{
        Field<typename std::tuple_element<I, FieldsTuple>::type::type, // raw type
              static_cast<FieldMutability>(
                  std::tuple_element<I, FieldsTuple>::type::properties) // cast to FieldMutability
              >(GetColumnValue<typename std::tuple_element<I, FieldsTuple>::type::type>(
            stmt, static_cast<uint16_t>(I) + 1))...};
}

// Helper that generates index sequence automatically
template <typename T> T ReadObjT(SQLHANDLE& stmt)
{
    return ReadObjImplT<T>(stmt,
                           std::make_index_sequence<std::tuple_size_v<decltype(T::fields)>>{});
}

//-------------------- GENERATOR --------------------
template <typename T> class Generator
{
  public:
    using YieldType = T;

    struct promise_type
    {
        YieldType current_value;
        std::exception_ptr exception;

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

        std::suspend_always yield_value(YieldType value) noexcept
        {
            current_value = std::move(value);
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
        if (coro_)
            coro_.destroy();
        coro_ = other.coro_;
        other.coro_ = {};
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
        using value_type = YieldType;
        using reference = YieldType&;
        using pointer = YieldType*;
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
            return coro_.promise().current_value;
        }
        pointer operator->() const
        {
            return &coro_.promise().current_value;
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
    handle_type coro_;
};

//-------------------- FETCH ROWS GENERATOR --------------------
template <typename T> Generator<std::unique_ptr<T>> FetchRowsGeneratorT(SQLHANDLE conn)
{
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(T::query)), SQL_NTS);

    using Raw = typename WrapperType<T>::Raw;

    while (SQLFetch(stmt) == SQL_SUCCESS)
    {
        Raw raw = ReadObjT<Raw>(stmt);                 // fetch raw data
        co_yield WrapperType<T>::wrap(std::move(raw)); // wrap mutable/immutable
    }

    SQLFreeHandle(SQL_HANDLE_STMT, stmt);
}
