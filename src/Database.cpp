#include "../include/database/Database.hpp"
#include "../include/database/Exceptions.hpp"
#include <sqlite3.h>
#include <chrono>
#include <stdexcept>

Database::Database(const std::string& dbPath) : db(nullptr) {
    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::string errorMsg = db ? sqlite3_errmsg(db) : "Unknown error";
        throw ConnectionException("Failed to open database: " + errorMsg);
    }
}

Database::~Database(){
    if (db){
        sqlite3_close(db);
    }
}

Result<bool> initializeDatabase() {
    try {
        const char* sql =
            "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "description TEXT NOT NULL,"
            "reminder_time_before INTEGER NOT NULL,"
            "created_at INTEGER NOT NULL,"
            "due_date INTEGER NOT NULL,"
            "completed INTEGER DEFAULT 0"
            ");";
        
        if (execute(sql)) {
            return true;
        }
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    } 
    catch (const DatabaseException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
}

Result <int> Database::addTask(const Task& task){
    if(!isConnected()) {
        return std::unexpected(makeErrorCode(DbError::ConnectionFailed));
    } 
    
    const char* sql =
    "INSERT INTO tasks (description, reminder_time_before, created_at, due_date) "
    "VALUES (?, ?, ?, ?);";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK){
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    }
    

    auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
    auto createdAt = std::chrono::system_clock::to_time_t(task.getCreatedAt());
    sqlite3_bind_text(stmt, 1, task.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, task.getReminderTimeBefore());
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(createdAt));
    sqlite3_bind_int64(stmt, 4, static_cast<sqlite3_int64>(dueTime));

    int result = -1;

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        result = static_cast<int>(sqlite3_last_insert_rowid(db));
    }

    sqlite3_finalize(stmt);
    return result;
    
}

Result <bool> Database::updateTask(const Task& task) {
    const char* sql = 
    "UPDATE tasks SET description = ?, reminder_time_before = ?,"
    "due_date = ?, completed = ? WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
    return false;

    auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
    auto createdAt = std::chrono::system_clock::to_time_t(task.getCreatedAt());
    sqlite3_bind_text(stmt, 1, task.getDescription().c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, task.getReminderTimeBefore());
    sqlite3_bind_int64(stmt, 3, static_cast<sqlite3_int64>(dueTime));
    sqlite3_bind_int(stmt, 4, task.isCompleted() ? 1 : 0);
    sqlite3_bind_int(stmt, 5, task.getId());

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;
}

Result <bool> Database::deleteTask(int taskId) {
    const char* sql = "DELETE  FROM tasks WHERE id = ?;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_DONE) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, taskId);

    bool success = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return success;

}

Result <std::vector<Task>> Database::getAllTasks() {
    const char* sql = 
    "SELECT id, description, reminder_time_before, created_at, due_date, completed FROM tasks;";

    sqlite3_stmt* stmt;
    std::vector<Task> tasks;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_DONE) {
        return tasks;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tasks.push_back(taskFromStatement(stmt));
    }
    
    sqlite3_finalize(stmt);
    return tasks;
}

Result <std::vector<Task>> Database::getPendingTasks() {
    const char* sql = 
    "SELECT id, description, reminder_time_before, created_at, due_date, completed" 
    "FROM tasks WHERE completed = 0;";

    sqlite3_stmt* stmt;
    std::vector<Task> tasks;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return tasks;
    }
    
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        tasks.push_back(taskFromStatement(stmt));
    }
    
    sqlite3_finalize(stmt);
    return tasks;
}

Result <std::vector<Task>> Database::getDeletedTasks() {
    return std::vector<Task> ();
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    bool result = (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) == SQLITE_DONE);

    if (result != SQLITE_OK) {
        std::string errorMessage = errMsg ? errMsg : "Unknown error";
        sqlite3_free(errMsg);
        switch (result)
        {
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
    
    return result == SQLITE_OK;

}

Task Database::taskFromStatement(sqlite3_stmt* stmt) {
    int id = sqlite3_column_int(stmt, 0);
    std::string description = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
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
}

bool Database::isConnected() {
    return db != nullptr;
}
