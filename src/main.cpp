#include <iostream>
#include <thread>
#include <memory>
#include <chrono>
#include <string>
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
    
    if (cliMode) {
        runCLI(dbPath);
        return 0;
    }
    
    // Process command line arguments for database path
    if (argc > 1) {
        dbPath = argv[1];
    }
    

    try {
        std::cout << "Task Management Application" << std::endl;
        std::cout << "==========================" << std::endl;
        
        // Initialize database
        std::cout << "Initializing database..." << std::endl;
        Database db("tasks.db");
        
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
        
        std::shared_ptr<EmailNotification> emailNotifier;
        try {
            emailNotifier = std::make_shared<EmailNotification>("user@example.com");
            emailNotifier->setNotificationPrefix("[TASK REMINDER]");
            emailNotifier->setSmtpServer("smtp.example.com");
            emailNotifier->setSmtpPort(587);
            emailNotifier->setSenderEmail("reminders@taskapp.com");
        } catch (const NotificationException& e) {
            std::cerr << "Failed to initialize email notifications: " << e.what() << std::endl;
            std::cerr << "Continuing with console notifications only." << std::endl;
        }
        
        // Initialize scheduler
        Scheduler scheduler;
        scheduler.setDefaultReminderMessage("Don't forget about your task!");
        scheduler.setMaxConcurrentTasks(20);
        
        // Create some tasks
        std::cout << "\nCreating tasks..." << std::endl;
        
        auto now = std::chrono::system_clock::now();
        
        // Task 1: Due in 1 hour, reminder 15 minutes before
        auto task1 = Task(1, "Complete project proposal", 15, 
                         now, 
                         now + std::chrono::hours(1));
        
        // Task 2: Due in 2 hours, reminder 30 minutes before
        auto task2 = Task(2, "Send weekly report", 30, 
                         now, 
                         now + std::chrono::hours(2));
        
        // Task 3: Due tomorrow, reminder 1 hour before
        auto task3 = Task(3, "Prepare presentation slides", 60, 
                         now, 
                         now + std::chrono::hours(24));
        
        // Add tasks to database
        auto task1IdResult = db.addTask(task1);
        if (!task1IdResult) {
            handleError(task1IdResult.error());
        } else {
            task1.setId(task1IdResult.value());
            std::cout << "Task 1 added with ID: " << task1.getId() << std::endl;
        }
        
        auto task2IdResult = db.addTask(task2);
        if (!task2IdResult) {
            handleError(task2IdResult.error());
        } else {
            task2.setId(task2IdResult.value());
            std::cout << "Task 2 added with ID: " << task2.getId() << std::endl;
        }
        
        auto task3IdResult = db.addTask(task3);
        if (!task3IdResult) {
            handleError(task3IdResult.error());
        } else {
            task3.setId(task3IdResult.value());
            std::cout << "Task 3 added with ID: " << task3.getId() << std::endl;
        }
        
        // Schedule tasks for notifications
        std::cout << "\nScheduling task reminders..." << std::endl;
        
        // In a real application, reminder times would be in the future
        // For demonstration, we'll use a very short time to see it work
        
        // For demo purposes, make task1 due very soon
        task1.setDueDate(now + std::chrono::seconds(30));
        task1.setReminderTimeBefore(15); // 15 seconds before due
        
        try {
            auto scheduleResult = scheduler.scheduleTask(task1, 
                [&consoleNotifier](const Task& task, const std::string& message) {
                    consoleNotifier->sendNotification(task, message);
                });
                
            if (!scheduleResult) {
                handleError(scheduleResult.error());
            } else {
                std::cout << "Task 1 scheduled for console notification" << std::endl;
            }
            
            if (emailNotifier) {
                auto emailScheduleResult = scheduler.scheduleTask(task2, 
                    [&emailNotifier](const Task& task, const std::string& message) {
                        emailNotifier->sendNotification(task, message);
                    });
                    
                if (!emailScheduleResult) {
                    handleError(emailScheduleResult.error());
                } else {
                    std::cout << "Task 2 scheduled for email notification" << std::endl;
                }
            }
            
        } catch (const TaskSchedulingException& e) {
            std::cerr << "Failed to schedule task: " << e.what() << std::endl;
        }
        
        // Update a task
        std::cout << "\nUpdating Task 3..." << std::endl;
        task3.setDescription("Prepare presentation slides with graphics");
        auto updateResult = db.updateTask(task3);
        if (!updateResult) {
            handleError(updateResult.error());
        } else if (updateResult.value()) {
            std::cout << "Task 3 updated successfully" << std::endl;
        } else {
            std::cout << "Task 3 not found or no changes made" << std::endl;
        }
        
        // Retrieve and display all tasks
        std::cout << "\nRetrieving all tasks:" << std::endl;
        auto tasksResult = db.getAllTasks();
        if (!tasksResult) {
            handleError(tasksResult.error());
        } else {
            const auto& tasks = tasksResult.value();
            std::cout << "Found " << tasks.size() << " tasks:" << std::endl;
            for (const auto& task : tasks) {
                printTask(task);
            }
        }
        
        // Demonstrate task completion
        std::cout << "\nMarking Task 2 as completed..." << std::endl;
        task2.markCompleted();
        auto completeResult = db.updateTask(task2);
        if (!completeResult) {
            handleError(completeResult.error());
        } else if (completeResult.value()) {
            std::cout << "Task 2 marked as completed" << std::endl;
        }
        
        // Check pending tasks
        std::cout << "\nRetrieving pending tasks:" << std::endl;
        auto pendingResult = db.getPendingTasks();
        if (!pendingResult) {
            handleError(pendingResult.error());
        } else {
            const auto& pendingTasks = pendingResult.value();
            std::cout << "Found " << pendingTasks.size() << " pending tasks:" << std::endl;
            for (const auto& task : pendingTasks) {
                printTask(task);
            }
        }
        
        // Demonstrate the scheduler checking for events
        std::cout << "\nRunning scheduler to check for events..." << std::endl;
        std::cout << "Waiting for task reminder to trigger..." << std::endl;
        
        // Wait and periodically check for events
        for (int i = 0; i < 5; i++) {
            std::this_thread::sleep_for(std::chrono::seconds(5));
            std::cout << "Checking for events..." << std::endl;
            
            auto eventsResult = scheduler.checkAndTriggerEvents();
            if (!eventsResult) {
                handleError(eventsResult.error());
                break;
            }
            
            std::cout << "Pending events: " << scheduler.getPendingEventsCount() << std::endl;
        }
        
        // Demonstrate deleting a task
        std::cout << "\nDeleting Task 3..." << std::endl;
        auto deleteResult = db.deleteTask(task3.getId());
        if (!deleteResult) {
            handleError(deleteResult.error());
        } else if (deleteResult.value()) {
            std::cout << "Task 3 deleted successfully" << std::endl;
        } else {
            std::cout << "Task 3 not found or could not be deleted" << std::endl;
        }
        
        // Final task list
        std::cout << "\nFinal task list:" << std::endl;
        auto finalTasksResult = db.getAllTasks();
        if (!finalTasksResult) {
            handleError(finalTasksResult.error());
        } else {
            const auto& tasks = finalTasksResult.value();
            std::cout << "Found " << tasks.size() << " tasks:" << std::endl;
            for (const auto& task : tasks) {
                printTask(task);
            }
        }
        
        std::cout << "\nApplication completed successfully!" << std::endl;
        return 0;
        
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