#pragma once
#include "../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include "proto/cam.pb.h"
#include "DataInsert.h"



using namespace cn::seisys::dds;

class CamTopic : public ZenohBase {
public:
    explicit CamTopic(const TopicInfo& topic_info);
    ~CamTopic() override;

    void start_writer(bool wait = false);
    void start_reader(bool wait = false);

private:
    void writer_worker();
    void reader_worker();
    bool send_cam(const Cam& node_msg);
    bool send_reply(const CamReply& reply_msg);
    void handle_node(const zenoh::Sample& sample);
    void handle_reply(const zenoh::Sample& sample);
    //void build_down_rte();


private:
    TopicInfo topic_info_;
    DataInsert data_insert_;
    SnowFlake* g_sf_;
    DbBase* db_;
    uint64_t startTime_;
    uint64_t endTime_;

    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    std::shared_ptr<zenoh::Publisher> node_publisher_;
    std::shared_ptr<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    std::optional<zenoh::Subscriber<void>> reply_subscriber_;

    std::queue<Cam> node_queue_;
    std::queue<CamReply> reply_queue_;
    std::mutex queue_mutex_;

    // 架构和操作系统映射
    static const std::map<std::string, int> arch_map_;
    static const std::map<std::string, int> os_map_;


    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;
    std::atomic<bool> writer_running_{ false };
    std::atomic<bool> reader_running_{ false };
};