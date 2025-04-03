#include "../include/core/CLI.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <map>
#include <algorithm>
#include <regex>

// Global variables for application state
std::shared_ptr<Database> db;
std::shared_ptr<Scheduler> scheduler;
std::shared_ptr<ConsoleNotification> consoleNotifier;
std::shared_ptr<EmailNotification> emailNotifier;
bool running = true;

void runCLI(const std::string& dbPath) {
    
    
    try {
        // Initialize components
        std::cout << "Task Manager CLI" << std::endl;
        std::cout << "================" << std::endl;
        std::cout << "Initializing with database: " << dbPath << std::endl;
        
        db = std::make_shared<Database>(dbPath);
        auto initResult = db->initializeDatabase();
        if (!initResult) {
            TaskApp::handleError(initResult.error());
        }
        
        scheduler = std::make_shared<Scheduler>();
        scheduler->setDefaultReminderMessage("Task reminder: Don't forget about your task!");
        
        consoleNotifier = std::make_shared<ConsoleNotification>();
        consoleNotifier->setNotificationPrefix("[TASK]");
        consoleNotifier->setColorOutput(true);
        consoleNotifier->setVerboseOutput(true);
        
        // Map of commands to their handlers
        std::map<std::string, CommandHandler> commandMap = {
            {"help", [](const std::vector<std::string>&) { printHelp(); }},
            {"add", handleAddTask},
            {"list", handleListTasks},
            {"update", handleUpdateTask},
            {"delete", handleDeleteTask},
            {"complete", handleCompleteTask},
            {"schedule", handleScheduleTask},
            {"check", handleCheckEvents},
            {"email", handleEmailSetup},
            {"exit", handleExit},
            {"quit", handleExit}
        };
        
        printHelp();
        
        // Main command loop
        while (running) {
            std::string input;
            std::cout << "\n> ";
            std::cout.flush(); 
            std::getline(std::cin, input);
            
            if (input.empty()) {
                continue;
            }
            
            auto args = parseArguments(input);
            if (args.empty()) {
                continue;
            }
            
            std::string command = args[0];
            
            auto it = commandMap.find(command);
            if (it != commandMap.end()) {
                try {
                    it->second(args);
                } catch (const TaskAppException& e) {
                    std::cerr << "Error: " << e.what() << std::endl;
                } catch (const std::exception& e) {
                    std::cerr << "Unexpected error: " << e.what() << std::endl;
                }
            } else {
                std::cout << "Unknown command: " << command << std::endl;
                std::cout << "Type 'help' for available commands." << std::endl;
            }
        }
        
    } catch (const ConnectionException& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }
    
}

// Print help information
void printHelp() {
    std::cout << "\nAvailable commands:\n";
    std::cout << "  help                             - Show this help message\n";
    std::cout << "  add <description> <due_date> <reminder_minutes>  - Add a new task\n";
    std::cout << "  list [pending|all]               - List tasks\n";
    std::cout << "  update <id> <description> <due_date> <reminder_minutes> - Update a task\n";
    std::cout << "  delete <id>                      - Delete a task\n";
    std::cout << "  complete <id>                    - Mark a task as completed\n";
    std::cout << "  schedule <id> <notification_type> - Schedule a task for notification\n";
    std::cout << "  check                            - Check and trigger due events\n";
    std::cout << "  email <recipient> <smtp_server> <port> - Configure email notification\n";
    std::cout << "  exit|quit                        - Exit the application\n";
    std::cout << "\nDate format: YYYY-MM-DD HH:MM or +minutes (for relative time from now)\n";
}

namespace TaskApp
{
    void handleError(const std::error_code& error) {
        std::cerr << "Error: " << error.message() << " (code: " << error.value() << ")" << std::endl;
    }
} 

std::vector<std::string> parseArguments(const std::string& input) {
    std::vector<std::string> args;
    std::string current;
    bool inQuotes = false;
    
    for (char c : input) {
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ' ' && !inQuotes) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        args.push_back(current);
    }
    
    return args;
}

// Trim whitespace from a string
std::string trimString(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Parse date time string in format YYYY-MM-DD HH:MM or +minutes
std::chrono::system_clock::time_point parseDateTime(const std::string& dateTimeStr) {
    // Check for relative time format (+minutes)
    static const std::regex relative_re(R"(\+(\d+))");
    std::smatch match;
    if (std::regex_match(dateTimeStr, match, relative_re)) {
        try {
            int minutes = std::stoi(match[1]);
            if (minutes < 0) {
                throw std::invalid_argument("Relative time cannot be negative");
            }
            return std::chrono::system_clock::now() + std::chrono::minutes(minutes);
        } catch (const std::exception&) {
            throw std::invalid_argument("Invalid relative time format. Use +minutes");
        }
    }

    // Parse absolute date/time (YYYY-MM-DD HH:MM)
    std::tm tm = {};
    std::istringstream ss(dateTimeStr);
    char date_delim1, date_delim2, time_delim;
    int year, month, day, hour, minute;

    // Parse date and time components
    ss >> year >> date_delim1 >> month >> date_delim2 >> day >> hour >> time_delim >> minute;

    if (ss.fail() || date_delim1 != '-' || date_delim2 != '-' || time_delim != ':') {
        throw std::invalid_argument("Invalid date/time format. Use YYYY-MM-DD HH:MM or +minutes");
    }

    // Validate ranges
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm now_tm = *std::localtime(&now);
    int current_year = now_tm.tm_year + 1900;

    if (year < current_year || year > current_year + 10) {
        throw std::invalid_argument("Year must be between " + std::to_string(current_year) + 
                                  " and " + std::to_string(current_year + 10));
    }
    if (month < 1 || month > 12) {
        throw std::invalid_argument("Month must be between 1 and 12");
    }
    if (day < 1 || day > 31) {
        throw std::invalid_argument("Day must be between 1 and 31");
    }
    if (hour < 0 || hour > 23) {
        throw std::invalid_argument("Hour must be between 0 and 23");
    }
    if (minute < 0 || minute > 59) {
        throw std::invalid_argument("Minute must be between 0 and 59");
    }

    // Set tm struct
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = 0;
    tm.tm_isdst = -1; // Let mktime determine daylight saving time

    // Convert to time_t and validate
    auto time = std::mktime(&tm);
    if (time == -1) {
        throw std::invalid_argument("Invalid date/time combination");
    }

    // Check if the date is in the past
    if (std::chrono::system_clock::from_time_t(time) < std::chrono::system_clock::now()) {
        throw std::invalid_argument("Date/time cannot be in the past");
    }

    return std::chrono::system_clock::from_time_t(time);
}

namespace TaskApp {
void printTask(const Task& task) {
    std::cout << "Task #" << task.getId() << ": " << task.getDescription() << std::endl;
    
    auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
    auto createdTime = std::chrono::system_clock::to_time_t(task.getCreatedAt());
    
    std::tm dueTm = *std::localtime(&dueTime);
    std::tm createdTm = *std::localtime(&createdTime);
    
    std::cout << "  Created: " << std::put_time(&createdTm, "%Y-%m-%d %H:%M") << std::endl;
    std::cout << "  Due: " << std::put_time(&dueTm, "%Y-%m-%d %H:%M") << std::endl;
    std::cout << "  Reminder: " << task.getReminderMinutes() << " minutes before due" << std::endl;
    std::cout << "  Status: " << (task.isCompleted() ? "Completed" : "Pending") << std::endl;
}
}
// Handle add task command
void handleAddTask(const std::vector<std::string>& args) {
    if (args.size() != 4) {  // Changed from 4 to 3 since we want dateTimeStr as one argument
        std::cout << "Usage: add \"description\" \"YYYY-MM-DD HH:MM\" reminderMinutes" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  add \"Do the dishes\" \"2025-04-05 15:14\" 30" << std::endl;
        std::cout << "  add \"Take medicine\" \"+60\" 5" << std::endl;
        return;
    }
    
    try {
        std::string description = args[1];  // First arg is "add"
        std::string dateTimeStr = args[2];  // Take the date-time string as is
        int reminderMinutes = std::stoi(args[3]);
        
        auto dueDate = parseDateTime(dateTimeStr);
        auto createdAt = std::chrono::system_clock::now();
        
        Task task(0, description, reminderMinutes, createdAt, dueDate);
        auto result = db->addTask(task);
        
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        std::cout << "Task added successfully with ID: " << result.value() << std::endl;
        
    } catch (const InvalidTaskDataException& e) {
        std::cout << "Error: Task error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle list tasks command
void handleListTasks(const std::vector<std::string>& args) {
    std::string type = "all";
    if (!args.empty()) {
        type = args[0];
    }
    
    Result<std::vector<Task>> tasksResult;
    
    if (type == "pending") {
        tasksResult = db->getPendingTasks();
    } else {
        tasksResult = db->getAllTasks();
    }
    
    if (!tasksResult) {
        TaskApp::handleError(tasksResult.error());
        return;
    }
    
    const auto& tasks = tasksResult.value();
    
    if (tasks.empty()) {
        std::cout << "No tasks found." << std::endl;
        return;
    }
    
    std::cout << "Found " << tasks.size() << " tasks:" << std::endl;
    std::cout << "------------------------------" << std::endl;
    
    for (const auto& task : tasks) {
        TaskApp::printTask(task);
        std::cout << "------------------------------" << std::endl;
    }
}

// Handle update task command
void handleUpdateTask(const std::vector<std::string>& args) {
    if (args.size() <= 1) {  // Show tasks if no ID provided
        auto tasksResult = db->getAllTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        if (tasks.empty()) {
            std::cout << "No tasks available to update." << std::endl;
            return;
        }

        std::cout << "Available tasks to update:" << std::endl;
        std::cout << "------------------------" << std::endl;
        for (const auto& task : tasks) {
            TaskApp::printTask(task);
            std::cout << "------------------------" << std::endl;
        }
        std::cout << "Usage: update <id> \"description\" \"YYYY-MM-DD HH:MM\" reminderMinutes" << std::endl;
        std::cout << "Example: update 1 \"Do dishes\" \"2025-04-04 22:11\" 10" << std::endl;
        return;
    }

    if (args.size() != 5) {  // args[0] is "update", so we need 5 total
        std::cout << "Usage: update <id> \"description\" \"YYYY-MM-DD HH:MM\" reminderMinutes" << std::endl;
        std::cout << "Example: update 1 \"Do dishes\" \"2025-04-04 22:11\" 10" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[1]);
        std::string description = args[2];
        std::string dateTimeStr = args[3];
        int reminderMinutes = std::stoi(args[4]);
        
        // First get the existing task to preserve creation date
        auto tasksResult = db->getAllTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }
        
        const auto& tasks = tasksResult.value();
        auto it = std::find_if(tasks.begin(), tasks.end(), 
                              [taskId](const Task& t) { return t.getId() == taskId; });
        
        if (it == tasks.end()) {
            std::cout << "Task not found with ID: " << taskId << std::endl;
            return;
        }
        
        Task task = *it; // Copy the existing task
        
        // Update the task properties
        if (!task.setDescription(description)) {
            std::cout << "Invalid description" << std::endl;
            return;
        }
        
        auto dueDate = parseDateTime(dateTimeStr);
        if (!task.setDueDate(dueDate)) {
            std::cout << "Invalid due date" << std::endl;
            return;
        }
        
        if (!task.setReminderMinutes(reminderMinutes)) {
            std::cout << "Invalid reminder time" << std::endl;
            return;
        }
        
        auto result = db->updateTask(task);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        if (result.value()) {
            std::cout << "Task updated successfully" << std::endl;
            TaskApp::printTask(task);
        } else {
            std::cout << "Task not found or no changes made" << std::endl;
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: Invalid number format for task ID or reminder minutes" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle delete task command
void handleDeleteTask(const std::vector<std::string>& args) {
    if (args.size() <= 1) {  // Changed from empty() check to size() <= 1
        // First show available tasks
        auto tasksResult = db->getAllTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        if (tasks.empty()) {
            std::cout << "No tasks available to delete." << std::endl;
            return;
        }

        std::cout << "Available tasks to delete:" << std::endl;
        std::cout << "------------------------" << std::endl;
        for (const auto& task : tasks) {
            std::cout << "ID: " << task.getId() << " - " << task.getDescription() << std::endl;
        }
        std::cout << "------------------------" << std::endl;
        std::cout << "Usage: delete <id>" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[1]);  // Changed from args[0] to args[1]
        
        // Get task description before deleting
        auto tasksResult = db->getAllTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        auto it = std::find_if(tasks.begin(), tasks.end(), 
                              [taskId](const Task& t) { return t.getId() == taskId; });
        
        if (it == tasks.end()) {
            std::cout << "Task not found with ID: " << taskId << std::endl;
            return;
        }

        std::string description = it->getDescription();
        
        auto result = db->deleteTask(taskId);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        if (result.value()) {
            std::cout << "Task \"" << description << "\" (ID: " << taskId << ") deleted successfully" << std::endl;
            
            // Also try to cancel any scheduled notifications
            auto cancelResult = scheduler->cancelTask(taskId);
            if (cancelResult && cancelResult.value()) {
                std::cout << "Task notifications cancelled" << std::endl;
            }
        } else {
            std::cout << "Task not found" << std::endl;
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: Invalid task ID. Please provide a number." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle complete task command
void handleCompleteTask(const std::vector<std::string>& args) {
    if (args.size() <= 1) {  // Show tasks if no ID provided (args[0] is "complete")
        auto tasksResult = db->getPendingTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        if (tasks.empty()) {
            std::cout << "No pending tasks available to complete." << std::endl;
            return;
        }

        std::cout << "Available tasks to complete:" << std::endl;
        std::cout << "------------------------" << std::endl;
        for (const auto& task : tasks) {
            TaskApp::printTask(task);
            std::cout << "------------------------" << std::endl;
        }
        std::cout << "Usage: complete <id>" << std::endl;
        std::cout << "Example: complete 1" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[1]);  // Use args[1] since args[0] is "complete"
        
        // Get task before marking as completed
        auto tasksResult = db->getAllTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        auto it = std::find_if(tasks.begin(), tasks.end(), 
                              [taskId](const Task& t) { return t.getId() == taskId; });
        
        if (it == tasks.end()) {
            std::cout << "Task not found with ID: " << taskId << std::endl;
            return;
        }

        if (it->isCompleted()) {
            std::cout << "Task is already completed." << std::endl;
            return;
        }

        Task task = *it;
        task.markCompleted();
        
        auto result = db->updateTask(task);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        if (result.value()) {
            std::cout << "Task \"" << task.getDescription() << "\" (ID: " << taskId 
                     << ") marked as completed" << std::endl;
            
            // Cancel any scheduled notifications
            auto cancelResult = scheduler->cancelTask(taskId);
            if (cancelResult && cancelResult.value()) {
                std::cout << "Task notifications cancelled" << std::endl;
            }
        } else {
            std::cout << "Failed to mark task as completed" << std::endl;
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: Invalid task ID. Please provide a number." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle schedule task command
void handleScheduleTask(const std::vector<std::string>& args) {
    if (args.size() <= 1) {  // Show tasks if no ID provided
        auto tasksResult = db->getPendingTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        if (tasks.empty()) {
            std::cout << "No pending tasks available to schedule." << std::endl;
            return;
        }

        std::cout << "Available tasks to schedule:" << std::endl;
        std::cout << "------------------------" << std::endl;
        for (const auto& task : tasks) {
            TaskApp::printTask(task);
            std::cout << "Notification types: console, email" << std::endl;
            std::cout << "------------------------" << std::endl;
        }
        std::cout << "Usage: schedule <id> <notification_type>" << std::endl;
        std::cout << "Example: schedule 1 console" << std::endl;
        return;
    }

    try {
        int taskId = std::stoi(args[1]);
        std::string notificationType = args.size() > 2 ? args[2] : "console";
        
        auto tasksResult = db->getPendingTasks();
        if (!tasksResult) {
            TaskApp::handleError(tasksResult.error());
            return;
        }

        const auto& tasks = tasksResult.value();
        auto it = std::find_if(tasks.begin(), tasks.end(), 
                              [taskId](const Task& t) { return t.getId() == taskId; });
        
        if (it == tasks.end()) {
            std::cout << "Task not found or already completed. ID: " << taskId << std::endl;
            return;
        }

        Task task = *it;
        
        if (notificationType == "console") {
            // Create callback function from ConsoleNotification
            Scheduler::Callback callback = [notifier = consoleNotifier](const Task& t, const std::string& msg) {
                notifier->sendNotification(t, msg);
            };
            
            auto scheduleResult = scheduler->scheduleTask(task, callback);
            if (!scheduleResult) {
                TaskApp::handleError(scheduleResult.error());
                return;
            }
            
            auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
            std::tm* dueTm = std::localtime(&dueTime);
            
            std::cout << "Task scheduled for console notification" << std::endl;
            std::cout << "Will notify " << task.getReminderMinutes() 
                     << " minutes before due time: " 
                     << std::put_time(dueTm, "%Y-%m-%d %H:%M") << std::endl;
                     
        } else if (notificationType == "email") {
            if (!emailNotifier) {
                std::cout << "Email notifications not configured. Use 'email' command first." << std::endl;
                return;
            }
            
            // Create callback function from EmailNotification
            Scheduler::Callback callback = [notifier = emailNotifier](const Task& t, const std::string& msg) {
                notifier->sendNotification(t, msg);
            };
            
            auto scheduleResult = scheduler->scheduleTask(task, callback);
            if (!scheduleResult) {
                TaskApp::handleError(scheduleResult.error());
                return;
            }
            std::cout << "Task scheduled for email notification" << std::endl;
        } else {
            std::cout << "Unknown notification type. Use 'console' or 'email'." << std::endl;
        }
    } catch (const std::invalid_argument& e) {
        std::cout << "Error: Invalid task ID. Please provide a number." << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle check events command
void handleCheckEvents(const std::vector<std::string>& args) {
    (void)args;
    std::cout << "Checking for events..." << std::endl;
    
    auto pendingTasks = db->getPendingTasks();
    if (!pendingTasks) {
        TaskApp::handleError(pendingTasks.error());
        return;
    }

    if (pendingTasks.value().empty()) {
        std::cout << "No pending tasks to check." << std::endl;
        return;
    }

    std::cout << "Pending events before check: " << scheduler->getPendingEventsCount() << std::endl;
    
    auto result = scheduler->checkAndTriggerEvents();
    if (!result) {
        TaskApp::handleError(result.error());
        return;
    }
    
    if (result.value() > 0) {
        std::cout << "Triggered " << result.value() << " notifications." << std::endl;
    } else {
        std::cout << "No notifications were due." << std::endl;
    }
    
    std::cout << "Events check completed." << std::endl;
    std::cout << "Pending events after check: " << scheduler->getPendingEventsCount() << std::endl;
}

// Handle email setup command
void handleEmailSetup(const std::vector<std::string>& args) {
    if (args.size() != 4) {  // args[0] is "email" command
        std::cout << "Usage: email <recipient> <smtp_server> <port>" << std::endl;
        std::cout << "Example: email user@example.com smtp.gmail.com 587" << std::endl;
        return;
    }
    
    try {
        std::string recipient = args[1];  // Changed from args[0] to args[1]
        std::string smtpServer = args[2]; // Changed from args[1] to args[2]
        int port;
        
        try {
            port = std::stoi(args[3]);    // Changed from args[2] to args[3]
            if (port <= 0 || port > 65535) {
                throw std::invalid_argument("Port must be between 1 and 65535");
            }
        } catch (const std::exception&) {
            std::cout << "Invalid port number. Must be between 1 and 65535." << std::endl;
            return;
        }
        
        // Validate email format using simple regex
        static const std::regex email_regex(R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
        if (!std::regex_match(recipient, email_regex)) {
            std::cout << "Invalid email format. Please use a valid email address." << std::endl;
            return;
        }

        emailNotifier = std::make_shared<EmailNotification>(recipient);
        emailNotifier->setNotificationPrefix("[TASK REMINDER]");
        emailNotifier->setSmtpServer(smtpServer);
        emailNotifier->setSmtpPort(port);
        emailNotifier->setSenderEmail("tasks@taskmanager.app");
        
        std::cout << "Email notifications configured successfully:" << std::endl;
        std::cout << "  Recipient: " << recipient << std::endl;
        std::cout << "  SMTP Server: " << smtpServer << std::endl;
        std::cout << "  Port: " << port << std::endl;
        
    } catch (const NotificationException& e) {
        std::cout << "Failed to configure email: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Error configuring email: " << e.what() << std::endl;
    }
}

// Handle exit command
void handleExit(const std::vector<std::string>& args) {
    (void)args;
    std::cout << "Exiting Task Manager. Goodbye!" << std::endl;
    running = false;
}