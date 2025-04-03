#pragma once
#include "Notification.hpp"
#include "../database/Exceptions.hpp"
#include <string>

class ConsoleNotification : public Notification {
public:
    ConsoleNotification();
    ~ConsoleNotification() override = default;
    
    // Core notification method
    void sendNotification(const Task& task, const std::string& message) override;
    
    // Configuration methods
    bool setColorOutput(bool useColor);
    bool setVerboseOutput(bool verbose);
    bool setNotificationSound(bool enable);
    
    // Getters
    bool isColorOutputEnabled() const noexcept;
    bool isVerboseOutputEnabled() const noexcept;
    bool isNotificationSoundEnabled() const noexcept;
    
private:
    bool colorOutput;
    bool verboseOutput;
    bool useSound;
    bool isInitialized;
    
    // Helper methods
    void validateInitialization() const;
    void validateTaskData(const Task& task) const;
    bool tryInitializeConsole();
};