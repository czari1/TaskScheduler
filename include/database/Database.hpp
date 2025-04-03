#pragma once
#include <sqlite3.h>
#include <vector>
#include "../core/Task.hpp"
#include "../core/Result.hpp"

class Database {
public:
    Database(const std::string& dbPath);
    ~Database();

    Result<bool> initializeDatabase();
    Result<void> validateDatabaseSchema();

    Result<int> addTask(const Task& task);
    Result<bool> updateTask(const Task& task);
    Result <bool> deleteTask(int taskId);
    Result <std::vector<Task>> getAllTasks();
    Result <std::vector<Task>> getPendingTasks();
    Result <std::vector<Task>> getDeletedTasks();

    bool setDatabasePath(const std::string& newPath);
    std::string getDatabasePath() const;

private:
    sqlite3* db;
    std::string dbPath;

    bool execute(const std::string& sql);
    Task taskFromStatement(sqlite3_stmt* stmt);
    bool isConnected();

};