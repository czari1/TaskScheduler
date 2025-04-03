#include "../include/notifications/ConsoleNotification.hpp"
#include "../include/database/Exceptions.hpp"
#include <iostream>
#include <ctime>

void ConsoleNotification::sendNotification(const Task& task, const std::string& message) {
    try {
        // Basic console notification with optional color and verbosity
        if (colorOutput) {
            std::cout << "\033[1;34m" << notificationPrefix << "\033[0m ";
        } else {
            std::cout << notificationPrefix;
        }
        
        std::cout << "Task #" << task.getId() << ": " << task.getDescription() << std::endl;
        
        if (verboseOutput) {
            auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
            std::cout << "Due: " << std::ctime(&dueTime);
            std::cout << "Message: " << message << std::endl;
        }
    } catch (const std::exception& e) {
        throw NotificationException("Failed to send console notification: " + std::string(e.what()));
    }
}

bool ConsoleNotification::setColorOutput(bool useColor) {
    colorOutput = useColor;
    return true;
}

bool ConsoleNotification::setVerboseOutput(bool verbose) {
    verboseOutput = verbose;
    return true;
}

bool ConsoleNotification::isColorOutputEnabled() const {
    return colorOutput;
}

bool ConsoleNotification::isVerboseOutputEnabled() const {
    return verboseOutput;
}