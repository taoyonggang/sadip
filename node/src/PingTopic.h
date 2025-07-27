#pragma once

#include "../../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include "proto/node.pb.h"
#include "../../dds/TopicInfo.h"

using namespace cn::seisys::dds;

class PingTopic : public ZenohBase {
public:
    explicit PingTopic(const TopicInfo& topic_info);
    ~PingTopic() override;

    //bool init(const TopicInfo& topic_info) override;
    void start_writer(bool wait = false);
    void start_reader(bool wait = false);

private:
    void writer_worker();
    void reader_worker();
    bool send_ping(const Ping& ping_msg);
    //bool send_reply(const NodeReply& reply_msg);
    void handle_ping(const zenoh::Sample& sample);
    //void handle_reply(const zenoh::Sample& sample);

private:
    TopicInfo topic_info_;
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    std::shared_ptr<zenoh::Publisher> ping_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;

   /* std::queue<Ping> ping_queue_;
    std::queue<NodeReply> reply_queue_;
    std::mutex queue_mutex_;*/

    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;
};