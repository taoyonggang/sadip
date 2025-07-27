#pragma once
#include "ZenohBase.h"
#include <optional>
#include <atomic>
#include <mutex>
#include <queue>
#include "proto/log.pb.h"

using namespace cn::seisys::dds;

class LogTopic : public ZenohBase {
public:
     LogTopic(const TopicInfo& topic_info);
    ~LogTopic() override;

    // 初始化函数在基类中已定义
    // virtual bool init(const TopicInfo& topic_info) override;
    bool init(const TopicInfo& topic_info);

    void start_writer(bool wait = false);
    void start_reader(bool wait = false);
    bool write_log_data(const std::string& filename, const std::string& logStr);

private:
    // 工作线程
    static void writer_worker(void* arg);
    static void reader_worker(void* arg);

    // 创建发布者和订阅者
    bool create_publisher();
    bool create_subscriber();
    bool create_reply_publisher();
    bool create_reply_subscriber();

    // 消息处理
    void handle_message(const zenoh::Sample& sample);
    void handle_reply_message(const zenoh::Sample& sample);
    void handle_reply(const cn::seisys::dds::LogData& logData);

    // Zenoh发布者和订阅者
    std::optional<zenoh::Publisher> publisher_;
    std::optional<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>>  subscriber_;
    std::optional<zenoh::Subscriber<void>>  reply_subscriber_;


    // 状态标志
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    // 消息队列和互斥锁
    std::queue<cn::seisys::dds::LogData> message_queue_;
    std::queue<cn::seisys::dds::LogData> reply_queue_;
    std::mutex queue_mutex_;

    // 监听器状态
    struct {
        std::atomic<int> matched_{ 0 };
        std::atomic<bool> firstConnected_{ false };
    } pub_status_;


    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;
    std::atomic<bool> writer_running_{ false };
    std::atomic<bool> reader_running_{ false };
};

// 内联函数实现（如果需要）
inline void LogTopic::handle_reply(const cn::seisys::dds::LogData& logData) {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    reply_queue_.push(logData);
}