#pragma once
#include <string>
#include <iostream>
#include <limits>
#include <functional>
#include <vector>
#include <memory>
#include <chrono>
#include "../database/Database.hpp"
#include "../include/core/Task.hpp"
#include "../include/core/Scheduler.hpp"
#include "../include/notifications/ConsoleNotification.hpp"
#include "../include/notifications/EmailNotification.hpp"
#include "../include/database/Exceptions.hpp"

// Command handler type
using CommandHandler = std::function<void(const std::vector<std::string>&)>;

namespace TaskApp{
    void printTask(const Task& task);
    void handleError(const std::error_code& error);
}

// Forward declarations
void printHelp();
std::vector<std::string> parseArguments(const std::string& input);
std::string trimString(const std::string& str);
std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeStr);

// Command handlers
void handleAddTask(const std::vector<std::string>& args);
void handleListTasks(const std::vector<std::string>& args);
void handleUpdateTask(const std::vector<std::string>& args);
void handleDeleteTask(const std::vector<std::string>& args);
void handleCompleteTask(const std::vector<std::string>& args);
void handleScheduleTask(const std::vector<std::string>& args);
void handleCheckEvents(const std::vector<std::string>& args);
void handleEmailSetup(const std::vector<std::string>& args);
void handleExit(const std::vector<std::string>& args);

// Main application entry point
void runCLI(const std::string& dbPath);