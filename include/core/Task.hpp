#pragma once
#include <string>
#include <chrono>
using std::string;

class Task {
public:
    Task(int id, 
        const string& description, 
        int reminderTimeBefore,
        const std::chrono::system_clock::time_point createdAt,
        const std::chrono::system_clock::time_point& dueDate);

    ~Task() = default;
    Task(const Task&) = default;
    Task(Task&&) noexcept = default;
    Task& operator=(const Task&) = default;
    Task& operator=(Task&&) noexcept = default;

    int getId() const;
    string getDescription() const;
    std::chrono::system_clock::time_point getDueDate() const;
    std::chrono::system_clock::time_point getCreatedAt() const;
    bool isCompleted() const;
    void markCompleted();
    int getReminderTimeBefore() const;
    std::chrono::system_clock::time_point getReminderTime() const;

    bool setId(int id);
    bool setDescription(const string& description);
    bool setDueDate(const std::chrono::system_clock::time_point& dueDate);
    bool setReminderTimeBefore(int minutes);
    void markIncomplete();

private:
    int id;
    string description;
    std::chrono::system_clock::time_point dueDate;
    std::chrono::system_clock::time_point createdAt;
    int reminderTimeBefore;
    bool completed{false};
};
