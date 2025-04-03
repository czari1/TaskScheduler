#include "../include/core/Result.hpp"  // Adjust path to match your project structure

namespace {

    struct DbErrorCategory : std::error_category {
        const char* name() const noexcept override {
            return "database_error";  
        }

        std::string message(int e) const override {
            switch (static_cast<DbError>(e)) {
            case DbError::ConnectionFailed:
                return "Failed to connect to database";
            case DbError::QueryFailed:
                return "Database query failed";
            case DbError::ConstraintViolation:  
                return "Database constraint violation";  
            default:
                return "Unknown database error";
            }
        } 
    };
    

    const DbErrorCategory theDbErrorCategory {};
}

std::error_category const& dbErrorCategory() {
    return theDbErrorCategory;
}

std::error_code makeErrorCode(DbError e) {
    return {static_cast<int>(e), dbErrorCategory()};
}