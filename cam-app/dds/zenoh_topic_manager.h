#ifndef ZENOH_TOPIC_MANAGER_H
#define ZENOH_TOPIC_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <sstream>

class ZenohTopicManager {
public:
    enum class Domain {
        DEFAULT,    // 默认域
        MEMORY,     // 内存数据(易失性)
        TIMES,      // 实时数据(低延迟)
        S3,         // 对象存储(持久化)
        DDS,        // DDS数据(兼容)
        FILE        // 本地文件持久化
    };

    enum class Direction {
        UP,
        DOWN,
        BOTH
    };

    struct Topic {
        Domain domain;            // 功能域
        std::string project;      // 项目
        std::string business;     // 业务域
        std::string region;       // 区域
        std::string nodeId;       // 节点ID
        std::string module;       // 业务模块
        std::string function;     // 功能
        Direction direction;      // 方向

        Topic(Domain d, std::string proj, std::string bus, 
              std::string reg, std::string nId, std::string mod, 
              std::string func, Direction dir)
            : domain(d), project(proj), business(bus),
              region(reg), nodeId(nId), module(mod),
              function(func), direction(dir) {}
    };

    ZenohTopicManager();
    
    // 基础Topic创建接口
    Topic createTopic(Domain domain,
                     const std::string& project,
                     const std::string& business,
                     const std::string& region,
                     const std::string& nodeId,
                     const std::string& module,
                     const std::string& function,
                     Direction direction = Direction::BOTH);

    // 创建文件持久化Topic
    Topic createFileTopic(const std::string& project,
                         const std::string& business,
                         const std::string& region,
                         const std::string& nodeId,
                         const std::string& module,
                         const std::string& function,
                         Direction direction = Direction::BOTH);

    // Topic管理接口
    void addTopic(const Topic& topic);
    std::string buildKeyExpr(const Topic& topic);
    std::string getWildcardExpr(const Topic& topic);
    std::vector<Topic> getTopics() const { return topics; }
    Domain getDomainEnum(const std::string& str);
    Direction getDirectionEnum(const std::string& str);
    std::string getDomainString(Domain domain);
    std::string getDirectionString(Direction dir);

private:
    std::map<std::string, std::string> domainMap;
    std::map<std::string, std::string> directionMap;
    std::vector<Topic> topics;


};

#endif // ZENOH_TOPIC_MANAGER_H