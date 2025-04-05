#pragma once
#include <variant>
#include <string>
#include <system_error>
#include <optional>
#include <utility>

enum class DbError {
    ConnectionFailed,
    QueryFailed,
    ConstraintViolation
};

std::error_code makeErrorCode(DbError e);

// Specialization for std::error_code enum
namespace std {
    template<>
    struct is_error_code_enum<DbError> : true_type {};
}

// Custom Result class that functions like std::expected
template<typename T>
class Result {
private:
    // Renamed from 'value' to 'storage' to avoid naming conflict
    std::variant<T, std::error_code> storage;

public:
    // Default constructor for containers like std::vector
    Result() : storage(T{}) {}
    
    // Constructor for value case
    Result(const T& v) : storage(v) {}
    Result(T&& v) : storage(std::move(v)) {}
    
    // Constructor for error case
    Result(const std::error_code& error) : storage(error) {}
    
    // Check if result contains a value
    bool has_value() const { 
        return std::holds_alternative<T>(storage); 
    }
    
    // Get the value (will throw if no value)
    const T& value() const& {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<T>(storage);
    }
    
    T& value() & {
        if (!has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<T>(storage);
    }
    
    // Get the error (will throw if no error)
    const std::error_code& error() const {
        if (has_value()) {
            throw std::bad_variant_access();
        }
        return std::get<std::error_code>(storage);
    }
    
    // Convenience operator for checking success
    explicit operator bool() const {
        return has_value();
    }
};

// For void results
template<>
class Result<void> {
private:
    std::optional<std::error_code> error_val;

public:
    // Constructor for value (void) case
    Result() : error_val(std::nullopt) {}
    
    // Constructor for error case
    Result(const std::error_code& error) : error_val(error) {}
    
    // Check if result is success
    bool has_value() const { 
        return !error_val.has_value(); 
    }
    
    // Get the error (will throw if no error)
    const std::error_code& error() const {
        if (!error_val.has_value()) {
            throw std::runtime_error("No error in Result<void>");
        }
        return *error_val;
    }
    
    // Convenience operator for checking success
    explicit operator bool() const {
        return has_value();
    }
};

// Helper function for creating error results
template<typename T>
Result<T> make_unexpected(std::error_code ec) {
    return Result<T>(ec);
}
