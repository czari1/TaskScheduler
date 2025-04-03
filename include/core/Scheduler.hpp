#pragma once
#include <map>
#include <functional>
#include <chrono>
#include <vector>
#include "Task.hpp"
#include "Result.hpp"

class Scheduler {
public:
    Scheduler() = default;
    ~Scheduler();

    using Callback = std::function<void(const Task&, const std::string& message)>;

    Result <bool> scheduleTask(const Task& task, Callback callback);
    Result <bool> checkAndTriggerEvents();
    Result <bool> cancelTask(int taskId);

    bool setDefaultReminderMessage(const std::string& message);
    bool setMaxConcurrentTasks(int maxTasks);
    bool setEventCheckInterval(const std::chrono::milliseconds& interval);

    std::string getDefaultReminderMessage() const;
    int getMaxConcurrentTasks() const;
    std::chrono::milliseconds getEventCheckInterval() const;
    size_t getPendingEventsCount() const;


private:
struct Event {
    std::chrono::system_clock::time_point triggerTime;
    Callback callback;
    Task task;
    

    Event(std::chrono::system_clock::time_point time, Callback cb, const Task& t)
        : triggerTime(time), callback(cb), task(t) {}
};

    std::multimap<std::chrono::system_clock::time_point,Event> events;
    std::string defaultReminderMessage{"Task reminder"};
    int maxConcurrentTasks{10};
    std::chrono::milliseconds eventCheckInterval{1000};
    
};