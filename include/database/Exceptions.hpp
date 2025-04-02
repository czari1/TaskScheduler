#pragma once
#include <stdexcept>
#include <string>

// Base exception class for the application
class TaskAppException : public std::runtime_error {
public:
    explicit TaskAppException(const std::string& message) : std::runtime_error(message) {}
};

// Database specific exceptions
class DatabaseException : public TaskAppException {
public:
    explicit DatabaseException(const std::string& message) : TaskAppException("Database error: " + message) {}
};

class ConnectionException : public DatabaseException {
public:
    explicit ConnectionException(const std::string& message) : DatabaseException("Connection failed: " + message) {}
};

class QueryException : public DatabaseException {
public:
    explicit QueryException(const std::string& message) : DatabaseException("Query failed: " + message) {}
};

class ConstraintException : public DatabaseException {
public:
    explicit ConstraintException(const std::string& message) : DatabaseException("Constraint violation: " + message) {}
};

// Task specific exceptions
class TaskException : public TaskAppException {
public:
    explicit TaskException(const std::string& message) : TaskAppException("Task error: " + message) {}
};

class InvalidTaskDataException : public TaskException {
public:
    explicit InvalidTaskDataException(const std::string& message) : TaskException("Invalid data: " + message) {}
};

// Scheduler specific exceptions
class SchedulerException : public TaskAppException {
public:
    explicit SchedulerException(const std::string& message) : TaskAppException("Scheduler error: " + message) {}
};

class TaskSchedulingException : public SchedulerException {
public:
    explicit TaskSchedulingException(const std::string& message) : SchedulerException("Scheduling failed: " + message) {}
};

// Notification specific exceptions
class NotificationException : public TaskAppException {
public:
    explicit NotificationException(const std::string& message) : TaskAppException("Notification error: " + message) {}
};

class EmailDeliveryException : public NotificationException {
public:
    explicit EmailDeliveryException(const std::string& message) : NotificationException("Email delivery failed: " + message) {}
};