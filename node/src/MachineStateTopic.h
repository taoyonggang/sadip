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

class MachineStateTopic : public ZenohBase {

public:
    explicit MachineStateTopic(const TopicInfo& topic_info);
    ~MachineStateTopic() override;


    void start_writer(bool wait = false);
    void start_reader(bool wait = false);

private:
    void writer_worker();
    void reader_worker();

    int get_system_info(cn::seisys::dds::MachineState& msg);
    bool send_machine_state(const cn::seisys::dds::MachineState& state);
    bool send_reply(const cn::seisys::dds::NodeReply& reply);

    void handle_state_data(const zenoh::Sample& sample);
    void handle_reply_data(const zenoh::Sample& sample);

    // Legacy compatibility listeners - 可以根据需要保留或移除
    //class NodePubListener {
    //public:
    //    NodePubListener() : matched_(0), firstConnected_(false) {}
    //    void on_matched(const zenoh::PublicationInfo& info);
    //    int matched_;
    //    bool firstConnected_;
    //} node_pub_listener_;

    //class NodeReplyPubListener {
    //public:
    //    NodeReplyPubListener() : matched_(0), firstConnected_(false) {}
    //    void on_matched(const zenoh::PublicationInfo& info);
    //    int matched_;
    //    bool firstConnected_;
    //} node_reply_pub_listener_;

private:
    TopicInfo topic_info_;
    std::mutex mon_mutex_;
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    std::shared_ptr<zenoh::Publisher> state_publisher_;
    //std::shared_ptr<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    //std::optional<zenoh::Subscriber<void>> reply_subscriber_;

    std::queue<cn::seisys::dds::MachineState> state_queue_;
    std::queue<cn::seisys::dds::NodeReply> reply_queue_;
    std::mutex queue_mutex_;

    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;
};