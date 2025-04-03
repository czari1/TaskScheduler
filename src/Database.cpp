#include "../include/database/Database.hpp"
#include "../include/database/Exceptions.hpp"
#include "../include/core/Result.hpp"
#include <sqlite3.h>
#include <chrono>
#include <stdexcept>
#include <set>
#include <filesystem>

Database::Database(const std::string& dbPath) {
    this->dbPath = dbPath; // Store the path
    bool fileExists = std::filesystem::exists(dbPath);
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        
        // Try to handle specific errors
        if (rc == SQLITE_BUSY || rc == SQLITE_LOCKED) {
            throw ConnectionException("Database file is locked: " + error);
        } else if (rc == SQLITE_PERM || rc == SQLITE_AUTH) {
            throw ConnectionException("Permission denied on database file: " + error);
        } else if (rc == SQLITE_CORRUPT) {
            throw ConnectionException("Database file is corrupted: " + error);
        } else {
            throw ConnectionException("Cannot open database: " + error);
        }
    }

    if (fileExists) {
        try {
            validateDatabaseSchema();
        } catch (const DatabaseException& e) {
            sqlite3_close(db);
            throw;
        }
    }
    
    // Enable foreign keys and set busy timeout
    sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    sqlite3_busy_timeout(db, 1000); // 1 second timeout
}

Result <void> Database::validateDatabaseSchema() {
    char* errMsg = nullptr;
    sqlite3_stmt* stmt;
    
    // Check if tasks table exists with expected columns
    const char* sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='tasks';";
    
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw QueryException("Failed to check for tasks table");
    }
    
    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    
    if (result != SQLITE_ROW) {
        // Table doesn't exist, schema is incompatible
        throw SchemaException("Database schema incompatible: tasks table not found");
    }
    
    // Check column structure
    sql = "PRAGMA table_info(tasks);";
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        throw QueryException("Failed to check tasks table schema");
    }
    
    // Expected column names in our schema
    std::set<std::string> expectedColumns = {
        "id", 
        "description", 
        "reminder_minutes",  // Changed from reminder_time_before
        "created_at",        // Changed from created_at
        "due_date", 
        "completed"
    };
    std::set<std::string> foundColumns;
    
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char* colName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (colName) {
            foundColumns.insert(colName);
        }
    }
    
    sqlite3_finalize(stmt);
    
    // Check if all expected columns are present
    for (const auto& col : expectedColumns) {
        if (foundColumns.find(col) == foundColumns.end()) {
            throw SchemaException("Database schema incompatible: missing column " + col);
        }
    }
}

Database::~Database(){
    if (db){
        sqlite3_close(db);
    }
}

Result<bool> Database::initializeDatabase() {
    const char* createTableSQL = 
        "CREATE TABLE IF NOT EXISTS tasks ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "description TEXT NOT NULL,"
        "reminder_minutes INTEGER NOT NULL,"  // Changed to match other references
        "created_at INTEGER NOT NULL,"            // Changed to match other references
        "due_date INTEGER NOT NULL,"
        "completed INTEGER DEFAULT 0"
        ");";

    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, createTableSQL, nullptr, nullptr, &errMsg);
    
    if (rc != SQLITE_OK) {
        std::string error(errMsg);
        sqlite3_free(errMsg);
        return Result<bool>(std::error_code(rc, std::generic_category()));
    }
    
    return Result<bool>(true);
}

Result<int> Database::addTask(const Task& task){
    if(!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    } 
    
    if(task.getDescription().empty()) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    const char* sql =
    "INSERT INTO tasks (description, reminder_minutes, created_at, due_date, completed) "
    "VALUES (?, ?, ?, ?, 0);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        std::string errorMsg = sqlite3_errmsg(db);
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
    
    try {
        auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
        auto createdAt = std::chrono::system_clock::to_time_t(task.getCreatedAt());

        if (sqlite3_bind_text(stmt, 1, task.getDescription().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
            throw QueryException("Failed to bind description parameter");
        }
        
        if (sqlite3_bind_int(stmt, 2, task.getReminderMinutes()) != SQLITE_OK) {
            throw QueryException("Failed to bind reminder parameter");
        }
        if (sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(createdAt)) != SQLITE_OK) {
            throw QueryException("Failed to bind created at parameter");
        }
        if (sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(dueTime)) != SQLITE_OK) {
            throw QueryException("Failed to bind due date parameter");
        }

        int stepResult = sqlite3_step(stmt);
        int result = -1;

        if (stepResult == SQLITE_DONE) {
            result = static_cast<int>(sqlite3_last_insert_rowid(db));
            sqlite3_finalize(stmt);
            return Result<int>(result);
        } else if (stepResult == SQLITE_CONSTRAINT) {
            sqlite3_finalize(stmt);
            throw ConstraintException("constraint violation while adding task");
        } else {
            sqlite3_finalize(stmt);
            throw QueryException("Failed to execute query: " + std::string(sqlite3_errmsg(db)));
        }

    } catch (const DatabaseException& e) {
        sqlite3_finalize(stmt);

        if (dynamic_cast<const ConstraintException*>(&e)) {
            return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
        }
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result<bool> Database::updateTask(const Task& task) {
    if(!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }
    try {
        const char* sql = 
            "UPDATE tasks SET "
            "description = ?, "
            "reminder_minutes = ?, "
            "due_date = ?, "
            "completed = ? "
            "WHERE id = ?;";

        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare UPDATE statement: " + std::string(sqlite3_errmsg(db)));
        }
        
        auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());

        if (sqlite3_bind_text(stmt, 1, task.getDescription().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 2, task.getReminderMinutes()) != SQLITE_OK ||
            sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(dueTime)) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 4, task.isCompleted() ? 1 : 0) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 5, task.getId()) != SQLITE_OK){
                sqlite3_finalize(stmt);
                throw QueryException("Failed to bind parameters for update");
        }

        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE) {
            if (result == SQLITE_CONSTRAINT) {
                throw ConstraintException("Constraint violation while updating task");
            } else {
                throw QueryException("Failed to update task: " + std::string(sqlite3_errmsg(db)));
            }
        }
        
        
        if (sqlite3_changes(db) == 0) {
            return Result<bool>(false);  
        }
        
        return Result<bool>(true);
    } catch (const DatabaseException& e) {
        if (dynamic_cast<const ConstraintException*>(&e)) {
            return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
        }
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result<bool> Database::deleteTask(int taskId) {
    
    if(!isConnected()){
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }

    if (taskId <= 0) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    try {
        const char* sql = "DELETE FROM tasks WHERE id = ?;";
        
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare delete statement: " + std::string(sqlite3_errmsg(db)));
        }

        if(sqlite3_bind_int(stmt, 1, taskId) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            throw QueryException("Failed to bind task ID parameter");
        }
        int result = sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE) {
            throw QueryException("Failed to delete task: " + std::string(sqlite3_errmsg(db)));
        }

        if (sqlite3_changes(db) == 0) {
            return Result<bool>(false);
        }
        return Result<bool>(true);

    } catch(const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result<std::vector<Task>> Database::getAllTasks() {

    if(!isConnected()){
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }

    try {
        const char* sql = 
        "SELECT id, description, reminder_minutes, created_at, due_date, completed FROM tasks;";
    
        sqlite3_stmt* stmt;
        std::vector<Task> tasks;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare SELECT statement: " + std::string(sqlite3_errmsg(db)));
        }

        int result;

        while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
            tasks.push_back(taskFromStatement(stmt));
        }

        sqlite3_finalize(stmt);

        if (result != SQLITE_DONE) {
            throw QueryException("Error while fetching tasks: " + std::string(sqlite3_errmsg(db)));
        }

        return Result<std::vector<Task>>(tasks);

    } catch(const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result<std::vector<Task>> Database::getPendingTasks() {
    
    if (!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }
    
    try {
        const char* sql = 
        "SELECT id, description, reminder_minutes, created_at, due_date, completed " 
        "FROM tasks WHERE completed = 0;";

        sqlite3_stmt* stmt;
        std::vector<Task> tasks;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare pending tasks query: " + std::string(sqlite3_errmsg(db)));
        }
        
        int result;

        while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
            tasks.push_back(taskFromStatement(stmt));
        }
        
        sqlite3_finalize(stmt);
        
        if (result != SQLITE_DONE) {
            throw QueryException("Error while fetching pending tasks: " + std::string(sqlite3_errmsg(db)));
        }
        
        return Result<std::vector<Task>>(tasks);
    } catch (const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result<std::vector<Task>> Database::getDeletedTasks() {
    if (!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }
    
    // Assuming we're implementing a soft delete pattern where deleted tasks 
    // might be marked with a flag or moved to a separate table
    try {
        // This is a placeholder implementation - adjust based on your actual requirements
        const char* sql = 
        "SELECT id, description, reminder_time_before, created_at, due_date, completed " 
        "FROM deleted_tasks;";  // Assuming there's a deleted_tasks table
        
        sqlite3_stmt* stmt;
        std::vector<Task> tasks;

        // Check if deleted_tasks table exists first
        if (sqlite3_prepare_v2(db, "SELECT name FROM sqlite_master WHERE type='table' AND name='deleted_tasks';", 
                              -1, &stmt, nullptr) != SQLITE_OK) {
            sqlite3_finalize(stmt);
            // Table doesn't exist, return empty result
            return Result<std::vector<Task>>(tasks);
        }
        
        int tableExists = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        
        if (tableExists != SQLITE_ROW) {
            // Table doesn't exist, return empty result
            return Result<std::vector<Task>>(tasks);
        }

        // Table exists, proceed with query
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare deleted tasks query: " + std::string(sqlite3_errmsg(db)));
        }
        
        int result;
        while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
            tasks.push_back(taskFromStatement(stmt));
        }
        
        sqlite3_finalize(stmt);
        
        if (result != SQLITE_DONE) {
            throw QueryException("Error while fetching deleted tasks: " + std::string(sqlite3_errmsg(db)));
        }
        
        return Result<std::vector<Task>>(tasks);
    } catch (const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

bool Database::execute(const std::string& sql) {
    if (!isConnected()) {
        throw ConnectionException("Database connection not established");
    }
    
    char* errMsg = nullptr;
    int result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg); 
    if (result != SQLITE_OK) {
        std::string errorMessage = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        
        switch (result) {
            case SQLITE_BUSY:
                throw ConnectionException("Database is busy: " + errorMessage);
            case SQLITE_CONSTRAINT:
                throw ConstraintException(errorMessage);
            default:
                throw QueryException(errorMessage);
        }
    }
    
    if (errMsg) {
        sqlite3_free(errMsg);
    }
    
    return true;
}

Task Database::taskFromStatement(sqlite3_stmt* stmt) {
    try {
        int id = sqlite3_column_int(stmt, 0);

        const char* descText = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));

        if (!descText) {
            throw InvalidTaskDataException("Null description in database record");
        }

        std::string description = descText;

        int reminderTimeBefore = sqlite3_column_int(stmt, 2);
        std::chrono::system_clock::time_point createdAt = 
            std::chrono::system_clock::from_time_t(sqlite3_column_int64(stmt, 3));
        
        std::chrono::system_clock::time_point dueDate = 
            std::chrono::system_clock::from_time_t(sqlite3_column_int64(stmt, 4));
        
        Task task(id, description, reminderTimeBefore, createdAt, dueDate);
        
        if (sqlite3_column_int(stmt, 5) == 1) {
            task.markCompleted();
        }
        
        return task;
    } catch (const std::exception& e) {
        throw InvalidTaskDataException("Failed to create task from database record: " + std::string(e.what()));
    }
}

bool Database::isConnected() {
    return db != nullptr;
}

bool Database::setDatabasePath(const std::string& newPath) {
    if (newPath.empty()) {
        return false;
    }
    
    if (isConnected()) {
        return false;
    }
    
    dbPath = newPath;
    return true;
}

std::string Database::getDatabasePath() const {
    return dbPath;
}