#include "ConfigMap.h"
#include "../../utils/split.h"
#include "../../utils/log/BaseLog.h"

void MapWrapper::addElement(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    map_[key] = value;
}

void MapWrapper::removeElement(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    map_.erase(key);
}

std::string MapWrapper::findElement(const std::string& key) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        return it->second;
    }
    return ""; // 或者根据需求返回一个默认值
}

bool MapWrapper::modifyElement(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = map_.find(key);
    if (it != map_.end()) {
        it->second = value;
        return true;
    }
    return false;
}


void MapWrapper::getNodePartitions(std::string& src, MapWrapper& configMap, std::string& des)
{
    if (src != "")
    {
        std::vector<std::string> partitions;
        utility::string::split(src, partitions, ',');
        for (int i = 0; i < partitions.size(); i++)
        {
            std::string nodePartition = configMap.findElement(partitions[i]);
            if (nodePartition != "")
            {
                if (des == "")
                {
                    des = nodePartition;
                }
                else
                {
                    des = des + "," + nodePartition;
                }
            }
            else
            {
                INFOLOG("Key {} not found!", partitions[i]);
            }
        }
    }
    else
    {
        des = src;
    }
}

// nodeReplayPartitionStr,fileSendReplyPartitionStr
void MapWrapper::getNodeReplayPartition(std::string& src, std::string& group, MapWrapper& configMap, std::string& dec)
{
    if (group == "mgr")
    {
        dec = src;
    }
    else
    {
        std::vector<std::string> replyPartitions;
        utility::string::split(src, replyPartitions, ',');
        for (int i = 0; i < replyPartitions.size(); i++)
        {
            std::string nodeReplyPart = configMap.findElement(replyPartitions[i]);
            if (nodeReplyPart != "")
            {
                if (dec == "")
                {
                    dec = nodeReplyPart;
                }
                else
                {
                    dec = dec + "," + nodeReplyPart;
                }
            }
            else
            {
                INFOLOG("Key {} not found!", replyPartitions[i]);
            }
        }
    }
}
