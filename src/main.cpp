#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include <string>
#include <filesystem>
#include "../include/database/Database.hpp"
#include "../include/core/Task.hpp"
#include "../include/core/Scheduler.hpp"
#include "../include/notifications/ConsoleNotification.hpp"
#include "../include/notifications/EmailNotification.hpp"
#include "../include/database/Exceptions.hpp"
#include "../include/core/CLI.hpp"

// Helper function to print task details
void printTask(const Task& task) {
    std::cout << "Task #" << task.getId() << ": " << task.getDescription() << std::endl;
    
    auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
    std::cout << "Due: " << std::ctime(&dueTime);
    
    auto reminderTime = std::chrono::system_clock::to_time_t(task.getReminderTime());
    std::cout << "Reminder: " << std::ctime(&reminderTime);
    
    std::cout << "Status: " << (task.isCompleted() ? "Completed" : "Pending") << std::endl;
    std::cout << "--------------------------" << std::endl;
}

// Helper function to handle errors
void handleError(const std::error_code& error) {
    std::cerr << "Error: " << error.message() << " (code: " << error.value() << ")" << std::endl;
}



int main(int argc, char* argv[]) {
    std::string dbPath = "tasks.db";
    bool cliMode = false;
    
    // Process command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--cli") {
            cliMode = true;
        } else {
            dbPath = arg;
        }
    }
    
    try {
        std::cout << "Task Management Application" << std::endl;
        std::cout << "==========================" << std::endl;

        std::filesystem::path dbFilePath(dbPath);
        if (!dbFilePath.is_absolute()) {
            dbFilePath = std::filesystem::absolute(dbFilePath);
        }
        
        std::filesystem::create_directories(dbFilePath.parent_path());
        dbPath = dbFilePath.string();
        
        std::cout << "Using database at: " << dbPath << std::endl;
        
        // Initialize database
        std::cout << "Initializing database..." << std::endl;
        Database db(dbPath);
        
        auto initResult = db.initializeDatabase();
        if (!initResult) {
            handleError(initResult.error());
            return 1;
        }
        std::cout << "Database initialized successfully!" << std::endl;
        
        // Create notification handlers
        auto consoleNotifier = std::make_shared<ConsoleNotification>();
        consoleNotifier->setNotificationPrefix("[TASK ALERT]");
        consoleNotifier->setColorOutput(true);
        consoleNotifier->setVerboseOutput(true);
        
        // Initialize scheduler
        Scheduler scheduler;
        scheduler.setDefaultReminderMessage("Don't forget about your task!");
        scheduler.setMaxConcurrentTasks(20);
        
        if (cliMode) {
            runCLI(dbPath);
        } else {
            // Get user's home directory for the example
            std::string homeDir = std::getenv("USERPROFILE");
            std::filesystem::path examplePath = std::filesystem::path(homeDir) / "TaskScheduler" / "tasks.db";
        
            std::cout << "Please use --cli flag to interact with the application." << std::endl;
            std::cout << "Usage examples:" << std::endl;
            std::cout << "    .\\task_scheduler.exe --cli" << std::endl;
            std::cout << "    .\\task_scheduler.exe --cli \"" << examplePath.string() << "\"" << std::endl;
            std::cout << std::endl;
            std::cout << "Current database path: '" << std::filesystem::absolute(dbPath).string() << "'" << std::endl;
        }
        
        return 0;
        
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Filesystem error: " << e.what() << std::endl;
    } catch (const ConnectionException& e) {
        std::cerr << "Database connection error: " << e.what() << std::endl;
    } catch (const DatabaseException& e) {
        std::cerr << "Database error: " << e.what() << std::endl;
    } catch (const TaskException& e) {
        std::cerr << "Task error: " << e.what() << std::endl;
    } catch (const SchedulerException& e) {
        std::cerr << "Scheduler error: " << e.what() << std::endl;
    } catch (const NotificationException& e) {
        std::cerr << "Notification error: " << e.what() << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Unexpected error: " << e.what() << std::endl;
    }
    
    return 1;
}