#pragma once
#include "Notification.hpp"
#include <string>

class EmailNotification : public Notification {
public:
    explicit EmailNotification(std::string recipient);
    void sendNotification(const Task& task, const std::string& message) override;

    bool setRecipient(const std::string& newRecipient);
    bool setSmtpServer(const std::string& server);
    bool setSmtpPort(int port);
    bool setSenderEmail(const std::string& email);

    std::string getRecipient() const;
    std::string getSmtpServer() const;
    int getSmtpPort() const;
    std::string getSenderEmail() const;

private:
    std::string recipient;
    std::string smtpServer{"localhost"};
    int smtpPort{25};
    std::string senderEmail{"notification@example.com"};
};