#pragma once
#include "Notification.hpp"
#include <string>

class ConsoleNotification : public Notification {
public:
    ConsoleNotification() = default;
    void sendNotification(const Task& task, const std::string& message) override;
    
    bool setColorOutput(bool useColor);
    bool setVerboseOutput(bool verbose);
    
    bool isColorOutputEnabled() const;
    bool isVerboseOutputEnabled() const;
    
private:
    bool colorOutput{true};
    bool verboseOutput{true};
};
