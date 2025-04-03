#include "../include/notifications/ConsoleNotification.hpp"
#include <iostream>
#include <iomanip>
#include <windows.h>
#include <stdexcept>
#include <memory>

namespace {
    // Windows console color codes
    constexpr int DEFAULT = 7;
    constexpr int RED = FOREGROUND_RED | FOREGROUND_INTENSITY;
    constexpr int GREEN = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr int YELLOW = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    constexpr int BLUE = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    
    class ConsoleHandle {
    private:
        HANDLE handle;
        WORD originalAttributes;
        
    public:
        ConsoleHandle() : handle(GetStdHandle(STD_OUTPUT_HANDLE)) {
            if (handle == INVALID_HANDLE_VALUE) {
                throw NotificationException("Failed to get console handle");
            }
            CONSOLE_SCREEN_BUFFER_INFO info;
            if (!GetConsoleScreenBufferInfo(handle, &info)) {
                throw NotificationException("Failed to get console information");
            }
            originalAttributes = info.wAttributes;
        }
        
        ~ConsoleHandle() {
            SetConsoleTextAttribute(handle, originalAttributes);
        }
        
        void setColor(int color) {
            SetConsoleTextAttribute(handle, static_cast<WORD>(color));
        }
    };
}

ConsoleNotification::ConsoleNotification() 
    : colorOutput(true)
    , verboseOutput(true)
    , useSound(true)
    , isInitialized(false) {
    if (!tryInitializeConsole()) {
        throw NotificationException("Failed to initialize console notification system");
    }
    isInitialized = true;
}

void ConsoleNotification::sendNotification(const Task& task, const std::string& message) {
    try {
        validateInitialization();
        validateTaskData(task);
        
        std::unique_ptr<ConsoleHandle> console;
        if (colorOutput) {
            console = std::make_unique<ConsoleHandle>();
        }
        
        // Get current time once and store it
        std::time_t currentTime = std::time(nullptr);
        std::tm* timeinfo = std::localtime(&currentTime);
        
        // Print notification header
        if (colorOutput) console->setColor(YELLOW);
        std::cout << "\n" << std::string(50, '=') << "\n";
        std::cout << getNotificationPrefix() << " [" 
                 << std::put_time(timeinfo, "%Y-%m-%d %H:%M:%S")
                 << "]\n";
        std::cout << std::string(50, '=') << "\n";

        // Print task details
        if (colorOutput) console->setColor(GREEN);
        std::cout << "Task #" << task.getId() << ": " << task.getDescription() << "\n";

        if (verboseOutput) {
            if (colorOutput) console->setColor(BLUE);
            auto dueTime = std::chrono::system_clock::to_time_t(task.getDueDate());
            std::cout << "Due: " << std::put_time(std::localtime(&dueTime), "%Y-%m-%d %H:%M:%S") << "\n";
            std::cout << "Reminder: " << task.getReminderMinutes() << " minutes before due\n";
            std::cout << "Status: " << (task.isCompleted() ? "Completed" : "Pending") << "\n";
        }

        // Print message
        if (colorOutput) console->setColor(RED);
        std::cout << "\nMessage: " << message << "\n";
        
        // Print footer
        if (colorOutput) console->setColor(YELLOW);
        std::cout << std::string(50, '=') << "\n" << std::endl;

        // Play sound if enabled
        if (useSound) {
            MessageBeep(MB_ICONINFORMATION);
        }
        
    } catch (const std::exception& e) {
        throw NotificationException("Failed to send console notification: " + std::string(e.what()));
    }
}

bool ConsoleNotification::setColorOutput(bool useColor) {
    validateInitialization();
    colorOutput = useColor;
    return true;
}

bool ConsoleNotification::setVerboseOutput(bool verbose) {
    validateInitialization();
    verboseOutput = verbose;
    return true;
}

bool ConsoleNotification::setNotificationSound(bool enable) {
    validateInitialization();
    useSound = enable;
    return true;
}

bool ConsoleNotification::isColorOutputEnabled() const noexcept {
    return colorOutput;
}

bool ConsoleNotification::isVerboseOutputEnabled() const noexcept {
    return verboseOutput;
}

bool ConsoleNotification::isNotificationSoundEnabled() const noexcept {
    return useSound;
}

void ConsoleNotification::validateInitialization() const {
    if (!isInitialized) {
        throw NotificationException("Console notification system not initialized");
    }
}

void ConsoleNotification::validateTaskData(const Task& task) const {
    if (task.getDescription().empty()) {
        throw NotificationException("Task description is empty");
    }
    if (task.getId() < 0) {
        throw NotificationException("Invalid task ID");
    }
}

bool ConsoleNotification::tryInitializeConsole() {
    try {
        // Test console handle and color support
        ConsoleHandle testHandle;
        return true;
    } catch (...) {
        return false;
    }
}