#pragma once
#include "../../utils/pch.h"
#include "../../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include "proto/node.pb.h"
#include "../../dds/TopicInfo.h"
#include "soci/soci.h"
#include "../db/DbBase.h"

using namespace cn::seisys::dds;

class NodeCmdTopic : public ZenohBase {
public:
    explicit NodeCmdTopic(const TopicInfo& topic_info);
    ~NodeCmdTopic() override;

    //bool init();
    void start_writer(int argc, char** argv, bool wait = false);
    void start_reader(int argc, char** argv, bool wait = false);

private:
    void writer_worker();
    void reader_worker();
    
    bool send_cmd(const NodeCmd& cmd_msg);
    bool send_reply(const NodeCmdReply& reply_msg);
    void handle_cmd(const zenoh::Sample& sample);
    void handle_reply(const zenoh::Sample& sample);

    // 辅助方法
    static void fill_cmd_message(NodeCmd& cmd_msg, const soci::row& row);
    static void update_cmd_status(DbBase* db, long long id, int status);
    void process_app_cmd_replies();
    void process_file_cmd_replies();
 /*   int create_replay_sub();
    int create_replay_pub();*/

    // 数据成员
    TopicInfo topic_info_;
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    std::shared_ptr<zenoh::Publisher> cmd_publisher_;
    std::shared_ptr<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    std::optional<zenoh::Subscriber<void>> reply_subscriber_;

    std::queue<NodeCmd> cmd_queue_;
    std::queue<NodeCmdReply> reply_queue_;
    std::mutex queue_mutex_;

    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;

    // 兼容性保留字段
    //class NodePubListener {
    //public:
    //    NodePubListener() : matched_(0), firstConnected_(false) {}
    //    void on_publication_matched(eprosima::fastdds::dds::DataWriter* writer,
    //        const eprosima::fastdds::dds::PublicationMatchedStatus& info);
    //    int matched_;
    //    bool firstConnected_;
    //} nodePubListener_;

    //class NodeReplyPubListener {
    //public:
    //    NodeReplyPubListener() : matched_(0), firstConnected_(false) {}
    //    void on_publication_matched(eprosima::fastdds::dds::DataWriter* writer,
    //        const eprosima::fastdds::dds::PublicationMatchedStatus& info);
    //    int matched_;
    //    bool firstConnected_;
    //} nodeReplyPubListener_;
};