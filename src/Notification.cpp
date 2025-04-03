#include "../include/notifications/Notification.hpp"
#include "../include//database/Exceptions.hpp"

bool Notification::setNotificationPrefix(const std::string& prefix) {
    if (prefix.empty()) {
        return false;
    }
    notificationPrefix = prefix;
    return true;
}

std::string Notification::getNotificationPrefix() const {
    return notificationPrefix;
}