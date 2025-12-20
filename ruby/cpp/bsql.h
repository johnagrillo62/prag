#pragma once

#include <windows.h>

#include <chrono>
#include <iostream>
#include <sql.h>
#include <sqlext.h>
#include <sqltypes.h>
#include <unordered_map>
#include <vector>

namespace bhw
{

template <typename T> auto ReadUniquePtr(const SQLHANDLE& stmt) -> std::unique_ptr<T>;

template <typename Container, typename T>
auto FetchRowsUnique(const SQLHANDLE& conn, const char* query) -> std::unique_ptr<Container>
{

    auto collection = std::make_unique<Container>(); // Create unique_ptr for the container
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    const auto ret =
        SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(query)), SQL_NTS);
    if (ret == -1)
    {

        SQLCHAR sqlState[6];  // NOLINT(modernize-avoid-c-arrays,
                              // readability-magic-numbers)
        SQLCHAR message[256]; // NOLINT(modernize-avoid-c-arrays,
                              // readability-magic-numbers)

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
auto FetchRows(const SQLHANDLE& conn, const char* query) -> Container
{
    Container collection;
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    auto ret =
        SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(query)), SQL_NTS);

    if (ret == -1)
    {
        SQLCHAR sqlState[6];  // NOLINT(modernize-avoid-c-arrays,
                              // readability-magic-numbers)
        SQLCHAR message[256]; // NOLINT(modernize-avoid-c-arrays,
                              // readability-magic-numbers)
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
    const auto ret =
        SQLExecDirectA(stmt, reinterpret_cast<SQLCHAR*>(const_cast<char*>(query.c_str())), SQL_NTS);
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
auto FetchRowsObjUnique(const SQLHANDLE& conn, const char* query)
    -> std::unique_ptr<Container>
{
    auto collection = std::make_unique<Container>();
    SQLHANDLE stmt;
    SQLAllocHandle(SQL_HANDLE_STMT, conn, &stmt);
    const auto ret = SQLExecDirectA(stmt,
                                    reinterpret_cast<SQLCHAR*>(const_cast<char*>(query.c_str())),

                                    SQL_NTS);

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
} // namespace bhw
