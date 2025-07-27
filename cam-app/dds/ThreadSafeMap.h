#include <map>
#include <mutex>
#include <utility>
#include <string>
#include <vector>
#include "monitorPubSubTypes.h"

using namespace cn::seisys::dds;
using namespace std;

class ThreadSafeMap {
public:
    using NodeIdPair = std::pair<std::string, std::string>;  // srcNodeId 和 toNodeId 的对
    using Data = TopicMonInfo;

    // 插入或更新数据
    void insertOrUpdate(const NodeIdPair& nodeIds, const Data& data) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = map_.find(nodeIds);
        if (it != map_.end()) {
            // 更新数据
             it->second.nCount(it->second.nCount() + data.nCount());  // 使用 data 和 it->second 更新数据
             it->second.nSize(it->second.nSize() + data.nSize());
        }
        else {
            // 插入新数据
            map_[nodeIds] = data;
        }
    }

    // 获取数据
    Data get(const NodeIdPair& nodeIds) {
        std::lock_guard<std::mutex> lock(mutex_);
        return map_[nodeIds];
    }

    // 清空某个分区的数据
    void erasePartition(const std::string& srcNodeId, const std::string& toNodeId) {
        std::lock_guard<std::mutex> lock(mutex_);
        NodeIdPair nodeIds = std::make_pair(srcNodeId, toNodeId);
        auto it = map_.find(nodeIds);
        if (it != map_.end()) {
            map_.erase(it);
        }
    }

    // 获取所有数据
    std::vector<Data> getAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<Data> allData;
        for (const auto& pair : map_) {
            allData.push_back(pair.second);
        }
        return allData;
    }

    // 清空所有数据
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        map_.clear();
    }

private:
    std::map<NodeIdPair, Data> map_;
    std::mutex mutex_;
};