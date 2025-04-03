#include <algorithm>
#include "../include/core/Scheduler.hpp"
#include "../include/database/Exceptions.hpp"


Scheduler::~Scheduler() {}

Result <bool> Scheduler::scheduleTask(const Task& task, Callback callback) {

    if (!callback) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    if (events.size() >= static_cast<size_t>(maxConcurrentTasks)) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }
    
    auto now = std::chrono::system_clock::now();
    auto reminderTime = task.getReminderTime();

    if(reminderTime <= now) {
        throw TaskSchedulingException("Reminder time has already passed");
    }

    try {
        Event event{reminderTime, callback, task};

        events.insert({event.triggerTime, event});
        return true;
    } catch (const std::exception& e) {
        throw TaskSchedulingException("Failed to schedule task: " + std::string(e.what()));
    }
}

Result <bool> Scheduler::checkAndTriggerEvents() {
    auto now = std::chrono::system_clock::now();
    auto it =events.begin();

    try {

        while (it != events.end() && it-> first <= now) {
            
            try {
                it -> second.callback(it->second.task, defaultReminderMessage);
                it = events.erase(it);
            } catch (const NotificationException& e) {
                //log this error
                it = events.erase(it);
                continue;
            } catch (const std::exception& e) {
                throw SchedulerException("Fialed to trigger event: " + std::string(e.what()));
            }
        }
        return true;
        
    } catch (const SchedulerException& e) {
        return std::unexpected(makeErrorCode(DbError::QueryFailed));
    } 
}

Result <bool> Scheduler::cancelTask (int taskId) {

    if (taskId <= 0) {
        return std::unexpected(makeErrorCode(DbError::ConstraintViolation));
    }

    try {
        auto it = std::find_if(events.begin(), events.end(), 
            [taskId](const auto& pair) {
                return pair.second.task.getId() == taskId;
            });
            
        if (it == events.end()) {
            return false;
        }

        events.erase(it);
        return true;
    } catch (const std::exception& e) {
        throw SchedulerException("Failed to cancel task: " + std::string(e.what()));
    }  
}

bool Scheduler::setMaxConcurrentTasks(int maxTasks) {
    
    if (maxTasks <= 0) {
        return false;
    }

    if (maxTasks < static_cast<int>(events.size())) {
        return false;
    }
    
    maxConcurrentTasks = maxTasks;
    return true;
}

bool Scheduler::setDefaultReminderMessage(const std::string& message) {

    if(message.empty()) {
        return false;
    }
    defaultReminderMessage = message;
    return true;
}

std::string Scheduler::getDefaultReminderMessage() const {
    return defaultReminderMessage;
}

int Scheduler::getMaxConcurrentTasks() const {
    return maxConcurrentTasks;
}

std::chrono::milliseconds Scheduler::getEventCheckInterval() const {
    return eventCheckInterval;
}
size_t Scheduler::getPendingEventsCount() const {
    return events.size();
}


