#pragma once
#include <string>
#include "../include/core/Task.hpp"

class Notification {
public:
    virtual void sendNotification(const Task& task, const std::string& message) = 0;
    virtual ~Notification() = default;

    virtual bool setNotificationPrefix(const std::string& prefix);
    virtual std::string getNotificationPrefix() const;

protected:
    std::string notificationPrefix{"NOTIFICATION: "};
};