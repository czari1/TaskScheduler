#include "../include/core/Task.hpp"

Task::Task(int id, 
           const string& description, 
           int reminderTimeBefore,
           const std::chrono::system_clock::time_point createdAt,
           const std::chrono::system_clock::time_point& dueDate)
    : id(id), 
      description(description), 
      reminderTimeBefore(reminderTimeBefore),
      createdAt(createdAt),
      dueDate(dueDate),
      completed(false) {
}

int Task::getId() const {
    return id;
}

string Task::getDescription() const {
    return description;
}

std::chrono::system_clock::time_point Task::getDueDate() const {
    return dueDate;
}

std::chrono::system_clock::time_point Task::getCreatedAt() const {
    return createdAt;
}

bool Task::isCompleted() const {
    return completed;
}

void Task::markCompleted() {
    completed = true;
}

void Task::markIncomplete() {
    completed = false;
}

std::chrono::system_clock::time_point Task::getReminderTime() const {
    // Calculate reminder time based on minutes before due date
    return dueDate - std::chrono::minutes(reminderTimeBefore);
}

int Task::getReminderTimeBefore() const {
    return reminderTimeBefore;
}

bool Task::setId(int newId) {
    if (newId <= 0) {
        return false;
    }
    id = newId;
    return true;
}

bool Task::setDescription(const string& newDescription) {
    if (newDescription.empty()) {
        return false;
    }
    description = newDescription;
    return true;
}

bool Task::setDueDate(const std::chrono::system_clock::time_point& newDueDate) {
    // Validate that due date is in the future
    if (newDueDate <= std::chrono::system_clock::now()) {
        return false;
    }
    dueDate = newDueDate;
    return true;
}

bool Task::setReminderTimeBefore(int minutes) {
    if (minutes < 0) {
        return false;
    }
    reminderTimeBefore = minutes;
    return true;
}