#pragma once

#include <map>
#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <condition_variable>
#include "LogMonitor.h"
#include "LogTopic.h"
#include "../dds/TopicInfo.h"

class LogMonitorManager
{
private:
    TopicInfo logInfo_;
    static std::unique_ptr<LogTopic> logTopic_;
    static std::mutex logTopicMutex_;
    mutable std::mutex monitorsMutex_;
    
    // 禁用拷贝构造和赋值
    LogMonitorManager(const LogMonitorManager&) = delete;
    LogMonitorManager& operator=(const LogMonitorManager&) = delete;
    
public:
    explicit LogMonitorManager(TopicInfo logInfo);
    ~LogMonitorManager();
    
    // 支持移动语义
    LogMonitorManager(LogMonitorManager&&) noexcept;
    LogMonitorManager& operator=(LogMonitorManager&&) noexcept;
    
    void startMonitoring(const std::string& filePath);
    void stopMonitoring(const std::string& filePath);
    std::string getActiveMonitors();
    std::map<std::string, std::vector<std::string>> consumeAllMessages();

private:
    std::map<std::string, std::unique_ptr<LogMonitor>> _monitors;
    
    // 初始化logTopic_
    void initLogTopic();
};
