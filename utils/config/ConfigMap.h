#ifndef CONFIGMAP_H
#define CONFIGMAP_H

#include <map>
#include <mutex>
#include <string>
#include <vector>

class MapWrapper {
private:
    std::map<std::string, std::string> map_;
    std::mutex mutex_;

public:
    void addElement(const std::string& key, const std::string& value);
    void removeElement(const std::string& key);
    std::string findElement(const std::string& key);
    bool modifyElement(const std::string& key, const std::string& value);


    void getNodePartitions(std::string& src, MapWrapper& configMap, std::string& des);
    void getNodeReplayPartition(std::string& src, std::string& group, MapWrapper& configMap, std::string& dec);
};

#endif