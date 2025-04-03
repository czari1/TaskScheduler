#include "../include/database/Database.hpp"
#include "../include/database/Exceptions.hpp"
#include "../include/core/Result.hpp"
#include <sqlite3.h>
#include <chrono>
#include <stdexcept>

Database::Database(const std::string& dbPath) {
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db);
        throw ConnectionException("Cannot open database: " + error);
    }
    
    // Enable foreign keys and set busy timeout
    sqlite3_exec(db, "PRAGMA foreign_keys = ON", nullptr, nullptr, nullptr);
    sqlite3_busy_timeout(db, 1000); // 1 second timeout
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
        "reminder_minutes INTEGER NOT NULL,"
        "creation_time INTEGER NOT NULL,"
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
    
    return Result<bool>();
}

Result <int> Database::addTask(const Task& task){
    if(!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    } 
    
    if(task.getDescription().empty()) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    const char* sql =
    "INSERT INTO tasks (id, description, reminder_time_before, created_at, due_date) "
    "VALUES (?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        std::string errorMsg = sqlite3_errmsg(db);
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
    
    try {
        auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
        auto createdAt = std::chrono::system_clock::to_time_t(task.getCreatedAt());

        if (sqlite3_bind_int(stmt, 1, task.getId()) != SQLITE_OK) {
            throw QueryException("Failed to bind ID parameter");
        };

        if (sqlite3_bind_text(stmt, 2, task.getDescription().c_str(), -1, SQLITE_TRANSIENT)) {
            throw QueryException("Failed to bind description parameter");
        };
        
        if (sqlite3_bind_int(stmt, 3, task.getReminderTimeBefore()) !=SQLITE_OK) {
            throw QueryException("Failed to bind reminder parameter");
        };
        if (sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(createdAt)) != SQLITE_OK) {
            throw QueryException("Failed to bind created at parameter");
        };
        if (sqlite3_bind_int64(stmt, 5, static_cast<sqlite3_int64>(dueTime)) != SQLITE_OK) {
            throw QueryException("Failed to bind due date parameter");
        };

    int result = -1;
    int stepResult = sqlite3_step(stmt);

    if (stepResult != SQLITE_DONE) {
        result = static_cast<int>(sqlite3_last_insert_rowid(db));
    } else if (stepResult == SQLITE_CONSTRAINT) {
        throw ConstraintException("constraint violation while adding task");
    } else {
        throw QueryException("Failed to excute query: " + std::string(sqlite3_errmsg(db)));
    }

    } catch (const DatabaseException& e) {
    sqlite3_finalize(stmt);

    if (dynamic_cast<const ConstraintException*>(&e)) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }
    return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result <bool> Database::updateTask(const Task& task) {
    if(!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }
    try {
        const char* sql = 
        "UPDATE tasks SET description = ?, reminder_time_before = ?,"
        "due_date = ?, completed = ? WHERE id = ?;";

        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            throw QueryException("Failed to prepare update statement: " + std::string(sqlite3_errmsg(db)));
        }
        
        auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());

        if (sqlite3_bind_text(stmt, 1, task.getDescription().c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 2, task.getReminderTimeBefore()) != SQLITE_OK ||
            sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(dueTime)) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 4, task.isCompleted() ? 1 : 0) != SQLITE_OK ||
            sqlite3_bind_int(stmt, 5, task.getId()) != SQLITE_OK){
                sqlite3_finalize(stmt);
                throw QueryException("Fialed to bind parameters for update");
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
            return false;  
        }
        
        return true;
    } catch (const DatabaseException& e) {
        if (dynamic_cast<const ConstraintException*>(&e)) {
            return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
        }
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result <bool> Database::deleteTask(int taskId) {
    
    if(!isConnected()){
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }

    if (taskId <=0) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    try {
        const char* sql = "DELETE  FROM tasks WHERE id = ?;";
        
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
            return false;
        }
        return true;

    } catch(const std::exception& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }

}

Result <std::vector<Task>> Database::getAllTasks() {

    if(!isConnected()){
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }

    try
    {
        const char* sql = 
        "SELECT id, description, reminder_time_before, created_at, due_date, completed FROM tasks;";
    
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

        return tasks;

    } catch(const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result <std::vector<Task>> Database::getPendingTasks() {
    
    if (!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    }
    
    try {
        const char* sql = 
        "SELECT id, description, reminder_time_before, created_at, due_date, completed " 
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
        
        return tasks;
    } catch (const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result <std::vector<Task>> Database::getDeletedTasks() {
    return std::vector<Task> ();
}

bool Database::execute(const std::string& sql) {

    if (!isConnected()) {
        throw ConnectionException("Database connection not established");
    }
    

    char* errMsg = nullptr;
    bool result = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg); 
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

    try
    {
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
