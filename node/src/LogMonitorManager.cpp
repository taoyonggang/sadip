#include "LogMonitorManager.h"
#include <mutex>

std::unique_ptr<LogTopic> LogMonitorManager::logTopic_ = nullptr;
std::mutex LogMonitorManager::logTopicMutex_;

LogMonitorManager::LogMonitorManager(TopicInfo logInfo)
    : logInfo_(std::move(logInfo))
{
    logInfo_.needReply_ = false;
    initLogTopic();
}

LogMonitorManager::LogMonitorManager(LogMonitorManager&& other) noexcept
    : logInfo_(std::move(other.logInfo_)),
      _monitors(std::move(other._monitors))
{
}

LogMonitorManager& LogMonitorManager::operator=(LogMonitorManager&& other) noexcept
{
    if (this != &other) {
        logInfo_ = std::move(other.logInfo_);
        _monitors = std::move(other._monitors);
    }
    return *this;
}

void LogMonitorManager::initLogTopic()
{
    std::lock_guard<std::mutex> lock(logTopicMutex_);
    if (!logTopic_) {
        logTopic_ = std::make_unique<LogTopic>(logInfo_);
        logTopic_->start_writer(false);
    }
}

LogMonitorManager::~LogMonitorManager() = default;

void LogMonitorManager::startMonitoring(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(monitorsMutex_);
    if (_monitors.find(filePath) == _monitors.end())
    {
        _monitors[filePath] = std::make_unique<LogMonitor>(filePath);
    }
    _monitors[filePath]->startMonitoring(logTopic_.get());
}

void LogMonitorManager::stopMonitoring(const std::string& filePath)
{
    std::lock_guard<std::mutex> lock(monitorsMutex_);
    auto it = _monitors.find(filePath);
    if (it != _monitors.end())
    {
        it->second->stopMonitoring();
        _monitors.erase(it);
    }
}

std::string LogMonitorManager::getActiveMonitors()
{
    std::lock_guard<std::mutex> lock(monitorsMutex_);
    Poco::JSON::Array arr;
    std::stringstream ss;
    
    for (const auto& [path, monitor] : _monitors)
    {
        arr.add(monitor->getActiveMonitor());
    }
    
    if (!arr.size()) return "";
    Poco::JSON::Stringifier::stringify(arr, ss);
    return ss.str();
}

std::map<std::string, std::vector<std::string>> LogMonitorManager::consumeAllMessages()
{
    std::lock_guard<std::mutex> lock(monitorsMutex_);
    std::map<std::string, std::vector<std::string>> allMessages;
    
    for (const auto& [path, monitor] : _monitors)
    {
        std::vector<std::string> messages;
        while (monitor->logQueue_.size_approx() > 0)
        {
            auto message = monitor->consumeLogQueue();
            messages.insert(messages.end(), message.begin(), message.end());
        }
        if (!messages.empty()) {
            allMessages[path] = std::move(messages);
        }
    }
    
    return allMessages;
}
