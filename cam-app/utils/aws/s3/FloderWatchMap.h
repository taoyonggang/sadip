#pragma once

#include <map>
#include <mutex>
#include <thread>

#include "FloderWatch.h"

class FloderWatchMap
{

public:
        bool addWatch(const std::string& key, FloderWatch& data) {
            std::lock_guard<std::mutex> lock(m_mutex);
            // insert方法会返回一个pair,第一个元素是一个迭代器,指向插入的元素,第二个元素是一个bool值,表示是否插入了新的元素
            auto result = m_watchMap.insert(std::make_pair(key, data));
            if (result.second) {
                auto fileWatcher = new FileWatcher(key);
                fileWatcher->start();
                data.isRuning = true;
                data.fileWatcher_ = fileWatcher;
            }
            return result.second; // 如果插入了新的元素,返回true,否则返回false
        }


        void removeWatch(const std::string& key) {
            std::lock_guard<std::mutex> lock(m_mutex);
            FloderWatch fileWatcher = getWatch(key);
            if (fileWatcher.isRuning) {
                fileWatcher.fileWatcher_->stop();
                delete fileWatcher.fileWatcher_;
                fileWatcher.fileWatcher_ = nullptr;
            }
            m_watchMap.erase(key);
        }

        FloderWatch getWatch(const std::string& key) {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_watchMap[key];
        }

        //void updateWatch(const std::string& key, FloderWatch& data) {
        //    std::lock_guard<std::mutex> lock(m_mutex);
        //    m_watchMap[key] = data;
        //}

private:
        std::map<std::string, FloderWatch> m_watchMap;
        std::mutex m_mutex;
};

