#include "zenoh_topic_manager.h"

ZenohTopicManager::ZenohTopicManager() {
    domainMap = {
        {"default", "default"},
        {"memory", "memory"},
        {"times", "times"},
        {"s3", "s3"},
        {"dds", "dds"},
        {"file", "file"}
    };

    directionMap = {
        {"up", "up"},
        {"down", "down"},
        {"both", "up/down"}
    };
}

ZenohTopicManager::Topic ZenohTopicManager::createTopic(
    Domain domain,
    const std::string& project,
    const std::string& business,
    const std::string& region,
    const std::string& nodeId,
    const std::string& module,
    const std::string& function,
    Direction direction) {
    return Topic(
        domain,
        project,
        business,
        region,
        nodeId,
        module,
        function,
        direction
    );
}

ZenohTopicManager::Topic ZenohTopicManager::createFileTopic(
    const std::string& project,
    const std::string& business,
    const std::string& region,
    const std::string& nodeId,
    const std::string& module,
    const std::string& function,
    Direction direction) {
    return Topic(
        Domain::FILE,
        project,
        business,
        region,
        nodeId,
        module,
        function,
        direction
    );
}

void ZenohTopicManager::addTopic(const Topic& topic) {
    topics.push_back(topic);
}

std::string ZenohTopicManager::buildKeyExpr(const Topic& topic) {
    std::stringstream ss;
    ss << getDomainString(topic.domain) << "/"
       << topic.project << "/"
       << topic.business << "/"
       << topic.region << "/"
       << topic.nodeId << "/"
       << topic.module << "/"
       << topic.function << "/"
       << getDirectionString(topic.direction);
    
    return ss.str();
}

std::string ZenohTopicManager::getWildcardExpr(const Topic& topic) {
    return buildKeyExpr(topic) + "/**";
}

std::string ZenohTopicManager::getDomainString(Domain domain) {
    switch(domain) {
        case Domain::DEFAULT: return "default";
        case Domain::MEMORY: return "memory";
        case Domain::TIMES: return "times";
        case Domain::S3: return "s3";
        case Domain::DDS: return "dds";
        case Domain::FILE: return "file";
        default: return "default";
    }
}

std::string ZenohTopicManager::getDirectionString(Direction dir) {
    switch(dir) {
        case Direction::UP: return "up";
        case Direction::DOWN: return "down";
        case Direction::BOTH: return "up/down";
        default: return "up/down";
    }
}

ZenohTopicManager::Domain ZenohTopicManager::getDomainEnum(const std::string& str) {
    if (str == "memory") return Domain::MEMORY;
    if (str == "times") return Domain::TIMES;
    if (str == "s3") return Domain::S3;
    if (str == "dds") return Domain::DDS;
    if (str == "file") return Domain::FILE;
    return Domain::DEFAULT;
}

ZenohTopicManager::Direction ZenohTopicManager::getDirectionEnum(const std::string& str) {
    if (str == "up") return Direction::UP;
    if (str == "down") return Direction::DOWN;
    return Direction::BOTH;
}