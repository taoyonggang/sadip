#pragma once
#include <Poco/FileStream.h>
#include <Poco/Thread.h>
#include <Poco/Mutex.h>
#include <Poco/Condition.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Stringifier.h>
#include <queue>
#include <string>
#include <vector>
#include <concurrentqueue.h>
#include "LogTopic.h"

class LogMonitor
{
public:
    LogMonitor(const std::string& filePath);
    void startMonitoring(LogTopic *logTopic);
    void stopMonitoring();
    static void run(const void* arg);
    std::string getActiveMonitor();
    std::vector<std::string> consumeLogQueue();
    std::string consumeLogQueueWithJson();

private:
    std::string _filePath;
    Poco::Thread _thread;
    const int _maxQueueSize = 5000;
    bool _stop;
    Poco::Mutex _mutex; // Mutex for thread safety

public:
    //std::queue<std::string> _logQueue;
    LogTopic* logTopic_;
    moodycamel::ConcurrentQueue<std::string> logQueue_;

};
