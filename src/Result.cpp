#include "Result.hpp"

struct DbErrorCategory : std::error_category {
    const char* name() const noexcept override {
        return "databse_error";
    }

    std::string message(int e) const override {
        switch (static_cast<DbError>(e)) {
        case DbError::ConnectionFailed:
            return "Failed to connect to database";
        case DbError::QueryFailed:
            return "Database query failed";
        case DbError::ConstrainstViolation:
            return "Database constrain violation";
        default:
            return "Unknown database error";

        }
    } 
};

const DbErrorCategory theDbErrorCategory {};

std::error_code makeErrorCode(DbError e) {
    return {static_cast<int>(e), theDbErrorCategory};
}