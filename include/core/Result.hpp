#pragma once
#include <expected>
#include <string>
#include <system_error>


enum class DbError {
    ConnectionFailed,
    QueryFailed,
    ConstrainstViolation
};

std::error_code makeErrorCode (DbError e);

template<typename T>
using Result = std::expected<T, std::error_code>;

template<>
struct std::is_error_code_enum<DbError> : std::true_type {};
