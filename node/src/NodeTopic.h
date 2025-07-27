#pragma once

#include "../../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include "../../dds/TopicInfo.h"
#include <map>
#include "proto/node.pb.h"
#include "proto/file.pb.h"


using namespace cn::seisys::dds;

class NodeTopic : public ZenohBase {
public:
    explicit NodeTopic(const TopicInfo& topic_info);
    ~NodeTopic() override;

    void start_writer(bool wait = false);
    void start_reader(bool wait = false);

private:
    void writer_worker();
    void reader_worker();
    bool send_node(const Node& node_msg);
    bool send_reply(const NodeReply& reply_msg);
    void handle_node(const zenoh::Sample& sample);
    void handle_reply(const zenoh::Sample& sample);
    
    // 辅助函数
    bool update_config(const Node& node, const std::string& description, int nodeReplyStatus);
    void process_config(const Node& node, NodeReply& reply);
    
private:
    TopicInfo topic_info_;
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };
    std::atomic<bool> is_reply_reader_{ false };

    std::shared_ptr<zenoh::Publisher> node_publisher_;
    std::shared_ptr<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    std::optional<zenoh::Subscriber<void>> reply_subscriber_;

    std::queue<Node> node_queue_;
    std::queue<NodeReply> reply_queue_;
    std::mutex queue_mutex_;

    // 架构和操作系统映射
    static const std::map<std::string, int> arch_map_;
    static const std::map<std::string, int> os_map_;


    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;
    std::unique_ptr<std::thread>reply_reader_thread_;
    std::atomic<bool> writer_running_{ false };
    std::atomic<bool> reader_running_{ false };
    std::atomic<bool> reply_reader_running_{ false };
};