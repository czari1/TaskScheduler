#include "../include/core/Task.hpp"
#include "../include/database/Exceptions.hpp"

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
        
        if (id < 0) {
            throw InvalidTaskDataException("Task ID cannot be negative");
        }
        
        if (description.empty()) {
            throw InvalidTaskDataException("Task description cannot be empty");
        }
        
        if (reminderTimeBefore < 0) {
            throw InvalidTaskDataException("Reminder time before due date cannot be negative");
        }
        
        if (dueDate <= createdAt) {
            throw InvalidTaskDataException("Due date must be after creation date");
        }
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

    if (newDueDate <= createdAt) {
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