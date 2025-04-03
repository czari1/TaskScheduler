#include "../include/core/CLI.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <iomanip>
#include <map>
#include <algorithm>

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
    auto now = std::chrono::system_clock::now();
    
    // Check for relative time format (e.g., +30 for 30 minutes from now)
    if (dateTimeStr[0] == '+') {
        try {
            int minutes = std::stoi(dateTimeStr.substr(1));
            return now + std::chrono::minutes(minutes);
        } catch (const std::exception&) {
            throw InvalidTaskDataException("Invalid relative time format. Use +minutes (e.g., +30)");
        }
    }
    
    // Parse absolute time format (YYYY-MM-DD HH:MM)
    std::tm tm = {};
    std::istringstream ss(dateTimeStr);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M");
    
    if (ss.fail()) {
        throw InvalidTaskDataException("Invalid date-time format. Use YYYY-MM-DD HH:MM");
    }
    
    auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm));
    
    // Ensure the time is in the future
    if (timePoint <= now) {
        throw InvalidTaskDataException("Due date must be in the future");
    }
    
    return timePoint;
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
    std::cout << "  Reminder: " << task.getReminderTimeBefore() << " minutes before due" << std::endl;
    std::cout << "  Status: " << (task.isCompleted() ? "Completed" : "Pending") << std::endl;
}
}
// Handle add task command
void handleAddTask(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: add <description> <due_date> <reminder_minutes>" << std::endl;
        std::cout << "Example: add \"Submit report\" \"2025-04-10 15:00\" 30" << std::endl;
        std::cout << "         add \"Call client\" +60" << std::endl;
        return;
    }
    
    std::string description = args[0];
    std::string dueDateStr = args[1];
    
    int reminderMinutes;
    try {
        reminderMinutes = std::stoi(args[2]);
        if (reminderMinutes < 0) {
            std::cout << "Reminder time must be positive" << std::endl;
            return;
        }
    } catch (const std::exception&) {
        std::cout << "Invalid reminder time. Please specify minutes as a number." << std::endl;
        return;
    }
    
    try {
        auto now = std::chrono::system_clock::now();
        auto dueDate = parseDateTime(dueDateStr);
        
        // Create task with temporary ID (will be replaced by database)
        Task task(0, description, reminderMinutes, now, dueDate);
        
        auto result = db->addTask(task);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        std::cout << "Task added with ID: " << result.value() << std::endl;
    } catch (const InvalidTaskDataException& e) {
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
    if (args.size() < 4) {
        std::cout << "Usage: update <id> <description> <due_date> <reminder_minutes>" << std::endl;
        std::cout << "Example: update 1 \"Updated report\" \"2025-04-15 16:00\" 45" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[0]);
        std::string description = args[1];
        std::string dueDateStr = args[2];
        int reminderMinutes = std::stoi(args[3]);
        
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
        
        auto dueDate = parseDateTime(dueDateStr);
        if (!task.setDueDate(dueDate)) {
            std::cout << "Invalid due date" << std::endl;
            return;
        }
        
        if (!task.setReminderTimeBefore(reminderMinutes)) {
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
        } else {
            std::cout << "Task not found or no changes made" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle delete task command
void handleDeleteTask(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: delete <id>" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[0]);
        
        auto result = db->deleteTask(taskId);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        if (result.value()) {
            std::cout << "Task deleted successfully" << std::endl;
            
            // Also try to cancel any scheduled notifications
            auto cancelResult = scheduler->cancelTask(taskId);
            if (cancelResult && cancelResult.value()) {
                std::cout << "Task notifications cancelled" << std::endl;
            }
        } else {
            std::cout << "Task not found" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle complete task command
void handleCompleteTask(const std::vector<std::string>& args) {
    if (args.empty()) {
        std::cout << "Usage: complete <id>" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[0]);
        
        // First get the task
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
        task.markCompleted();
        
        auto result = db->updateTask(task);
        if (!result) {
            TaskApp::handleError(result.error());
            return;
        }
        
        if (result.value()) {
            std::cout << "Task marked as completed" << std::endl;
            
            // Also try to cancel any scheduled notifications
            auto cancelResult = scheduler->cancelTask(taskId);
            if (cancelResult && cancelResult.value()) {
                std::cout << "Task notifications cancelled" << std::endl;
            }
        } else {
            std::cout << "Task not found or no changes made" << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle schedule task command
void handleScheduleTask(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        std::cout << "Usage: schedule <id> <notification_type>" << std::endl;
        std::cout << "       notification_type: console or email" << std::endl;
        return;
    }
    
    try {
        int taskId = std::stoi(args[0]);
        std::string notificationType = args[1];
        
        // First get the task
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
        
        if (task.isCompleted()) {
            std::cout << "Cannot schedule notifications for completed tasks" << std::endl;
            return;
        }
        
        if (notificationType == "console") {
            auto scheduleResult = scheduler->scheduleTask(task, 
                [](const Task& t, const std::string& message) {
                    consoleNotifier->sendNotification(t, message);
                });
                
            if (!scheduleResult) {
                TaskApp::handleError(scheduleResult.error());
                return;
            }
            
            std::cout << "Task scheduled for console notification" << std::endl;
        } else if (notificationType == "email") {
            if (!emailNotifier) {
                std::cout << "Email notifications not configured. Use 'email' command first." << std::endl;
                return;
            }
            
            auto scheduleResult = scheduler->scheduleTask(task, 
                [](const Task& t, const std::string& message) {
                    emailNotifier->sendNotification(t, message);
                });
                
            if (!scheduleResult) {
                TaskApp::handleError(scheduleResult.error());
                return;
            }
            
            std::cout << "Task scheduled for email notification" << std::endl;
        } else {
            std::cout << "Unknown notification type. Use 'console' or 'email'." << std::endl;
        }
    } catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }
}

// Handle check events command
void handleCheckEvents(const std::vector<std::string>& args) {
    (void) args;
    std::cout << "Checking for events..." << std::endl;
    std::cout << "Pending events before check: " << scheduler->getPendingEventsCount() << std::endl;
    
    auto result = scheduler->checkAndTriggerEvents();
    if (!result) {
        TaskApp::handleError(result.error());
        return;
    }
    
    std::cout << "Events check completed." << std::endl;
    std::cout << "Pending events after check: " << scheduler->getPendingEventsCount() << std::endl;
}

// Handle email setup command
void handleEmailSetup(const std::vector<std::string>& args) {
    if (args.size() < 3) {
        std::cout << "Usage: email <recipient> <smtp_server> <port>" << std::endl;
        std::cout << "Example: email user@example.com smtp.example.com 587" << std::endl;
        return;
    }
    
    std::string recipient = args[0];
    std::string smtpServer = args[1];
    
    int port;
    try {
        port = std::stoi(args[2]);
    } catch (const std::exception&) {
        std::cout << "Invalid port number" << std::endl;
        return;
    }
    
    try {
        emailNotifier = std::make_shared<EmailNotification>(recipient);
        emailNotifier->setNotificationPrefix("[TASK REMINDER]");
        emailNotifier->setSmtpServer(smtpServer);
        emailNotifier->setSmtpPort(port);
        emailNotifier->setSenderEmail("tasks@taskmanager.app");
        
        std::cout << "Email notifications configured for " << recipient << std::endl;
    } catch (const NotificationException& e) {
        std::cout << "Failed to configure email: " << e.what() << std::endl;
    }
}

// Handle exit command
void handleExit(const std::vector<std::string>& args) {
    (void)args;
    std::cout << "Exiting Task Manager. Goodbye!" << std::endl;
    running = false;
}