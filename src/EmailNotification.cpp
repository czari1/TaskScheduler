#include "../include/notifications/EmailNotification.hpp"
#include "../include/database/Exceptions.hpp"
#include <iostream>
#include <regex>

EmailNotification::EmailNotification(std::string initialRecipient) {

    if (!setRecipient(initialRecipient)) {
        throw NotificationException("Invalid email recipient: " + initialRecipient);
    } 
}

void EmailNotification::sendNotification(const Task& task, const std::string& message) {
    try {
        // In a real implementation, this would connect to an SMTP server and send an email
        std::cout << "Sending email to " << recipient << "\n";
        std::cout << "Subject: " << notificationPrefix << " Task Reminder\n";
        std::cout << "Task ID: " << task.getId() << "\n";
        std::cout << "Description: " << task.getDescription() << "\n";
        std::cout << "Message: " << message << "\n";
        
        // Simulate network issues sometimes
        if (recipient.find("invalid") != std::string::npos) {
            throw EmailDeliveryException("Failed to deliver email to " + recipient);
        }
    } catch (const EmailDeliveryException& e) {
        throw; // Rethrow specific exception
    } catch (const std::exception& e) {
        throw EmailDeliveryException("Failed to send email notification: " + std::string(e.what()));
    }
}

bool EmailNotification::setRecipient(const std::string& newRecipient) {
    // Simple email validation
    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!std::regex_match(newRecipient, emailRegex)) {
        return false;
    }
    
    recipient = newRecipient;
    return true;
}

bool EmailNotification::setSmtpServer(const std::string& server) {
    if (server.empty()) {
        return false;
    }
    smtpServer = server;
    return true;
}

bool EmailNotification::setSmtpPort(int port) {
    if (port <= 0 || port > 65535) {
        return false;
    }
    smtpPort = port;
    return true;
}

bool EmailNotification::setSenderEmail(const std::string& email) {
    // Simple email validation
    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    if (!std::regex_match(email, emailRegex)) {
        return false;
    }
    
    senderEmail = email;
    return true;
}

std::string EmailNotification::getRecipient() const {
    return recipient;
}

std::string EmailNotification::getSmtpServer() const {
    return smtpServer;
}

int EmailNotification::getSmtpPort() const {
    return smtpPort;
}

std::string EmailNotification::getSenderEmail() const {
    return senderEmail;
}