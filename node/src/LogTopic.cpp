#include "LogTopic.h"

LogTopic::LogTopic(const TopicInfo& topic_info) {
}

LogTopic::~LogTopic() {
    writer_running_ = false;
    reader_running_ = false;

    if (writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }

    if (reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}

bool LogTopic::init(const TopicInfo& topic_info) {
    // 首先调用父类的init初始化连接信息
    if (!ZenohBase::init(topic_info)) {
        ERRORLOG("Failed to initialize base ZenohBase");
        return false;
    }

    // 创建发布者
    if (!create_publisher()) {
        ERRORLOG("Failed to create publisher");
        return false;
    }

    // 如果需要订阅，创建订阅者
    {
        if (!create_subscriber()) {
            ERRORLOG("Failed to create subscriber");
            return false;
        }
    }

    // 如果需要回复功能，创建回复发布者和订阅者
    if (topic_info_.needReply_) {
        if (!create_reply_publisher()) {
            ERRORLOG("Failed to create reply publisher");
            return false;
        }

        if (!create_reply_subscriber()) {
            ERRORLOG("Failed to create reply subscriber");
            return false;
        }
    }

    INFOLOG("LogTopic initialized successfully");
    return true;
}

void LogTopic::start_writer(bool wait) {
    if (!writer_thread_) {
        writer_running_ = true;
        writer_thread_ = std::make_unique<std::thread>(writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }
}

void LogTopic::start_reader(bool wait) {
    if (!reader_thread_) {
        reader_running_ = true;
        reader_thread_ = std::make_unique<std::thread>(reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}

void LogTopic::writer_worker(void* arg) {
    auto* topic = static_cast<LogTopic*>(arg);
    while (topic->writer_running_) {
        LogData log_msg;
        bool has_msg = false;
        {
            std::lock_guard<std::mutex> lock(topic->queue_mutex_);
            if (!topic->message_queue_.empty()) {
                log_msg = std::move(topic->message_queue_.front());
                topic->message_queue_.pop();
                has_msg = true;
            }
        }

        if (has_msg && topic->publisher_) {
            try {
                std::vector<uint8_t> serialized_msg(log_msg.ByteSize());
                log_msg.SerializeToArray(serialized_msg.data(), log_msg.ByteSize());
                topic->publisher_->put(serialized_msg);
            }
            catch (const std::exception& e) {
                ERRORLOG("Failed to send log message: {}", e.what());
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void LogTopic::reader_worker(void* arg) {
    auto* topic = static_cast<LogTopic*>(arg);
    while (topic->reader_running_) {
        LogData reply_msg;
        bool has_reply = false;
        {
            std::lock_guard<std::mutex> lock(topic->queue_mutex_);
            if (!topic->reply_queue_.empty()) {
                reply_msg = std::move(topic->reply_queue_.front());
                topic->reply_queue_.pop();
                has_reply = true;
            }
        }

        if (has_reply && topic->reply_publisher_) {
            try {
                std::vector<uint8_t> serialized_msg(reply_msg.ByteSize());
                reply_msg.SerializeToArray(serialized_msg.data(), reply_msg.ByteSize());
                topic->reply_publisher_->put(serialized_msg);
            }
            catch (const std::exception& e) {
                ERRORLOG("Failed to send reply message: {}", e.what());
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool LogTopic::create_publisher() {
    try {
        if (!publisher_) {
            publisher_.emplace(session_->declare_publisher(get_full_key()));
            INFOLOG("Created publisher for topic: {}", get_full_key());
        }
        return true;
    }
    catch (const zenoh::ZException& e) {
        ERRORLOG("Failed to create publisher: {}", e.what());
        return false;
    }
}

bool LogTopic::create_subscriber() {
    try {
        if (!subscriber_) {
            auto callback = [this](const zenoh::Sample& sample) {
                this->handle_message(sample);
                };
            auto on_drop_log = []() {
                DEBUGLOG("Log Subscriber dropped");
                };

            subscriber_.emplace(session_->declare_subscriber(
                topic_info_.topic_,
                std::move(callback),
                std::move(on_drop_log)
            ));

            INFOLOG("Created subscriber for topic: {}", topic_info_.topic_);
        }
        return true;
    }
    catch (const zenoh::ZException& e) {
        ERRORLOG("Failed to create subscriber: {}", e.what());
        return false;
    }
}

bool LogTopic::write_log_data(const std::string& filename, const std::string& logStr) {
    if (!publisher_) {
        ERRORLOG("Publisher not initialized");
        return false;
    }

    try {
        cn::seisys::dds::LogData log_data;
        log_data.set_uuid("");
        log_data.set_src_node_id(topic_info_.nodeId_);
        log_data.set_to_node_id(topic_info_.monNodeId_);
        log_data.set_file_name(filename);
        log_data.set_new_logs(logStr);

        std::vector<uint8_t> serialized_msg(log_data.ByteSize() + 1);
        log_data.SerializeToArray(serialized_msg.data(), log_data.ByteSize());
        publisher_->put(serialized_msg);
        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Failed to write log data: {}", e.what());
        return false;
    }
}

void LogTopic::handle_message(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        size_t size = sample.get_payload().size();
        cn::seisys::dds::LogData logData;
        logData.ParsePartialFromArray(data.data(), size);

        if (topic_info_.needReply_) {
            handle_reply(logData);
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Error processing message: {}", e.what());
    }
}

bool LogTopic::create_reply_publisher() {
    try {
        if (!reply_publisher_) {
            std::string reply_key = topic_info_.topicReply_;
            reply_publisher_.emplace(session_->declare_publisher(reply_key));
            INFOLOG("Created reply publisher for topic: {}", reply_key);
        }
        return true;
    }
    catch (const zenoh::ZException& e) {
        ERRORLOG("Failed to create reply publisher: {}", e.what());
        return false;
    }
}

bool LogTopic::create_reply_subscriber() {
    try {
        if (!reply_subscriber_) {
            std::string reply_key = topic_info_.topicReply_;
            auto callback = [this](const zenoh::Sample& sample) {
                this->handle_reply_message(sample);
                };
            auto on_drop_log = []() {
                DEBUGLOG("Log Subscriber reply dropped");
                };

            subscriber_.emplace(session_->declare_subscriber(
                reply_key,
                std::move(callback),
                std::move(on_drop_log)
            ));
            INFOLOG("Created reply subscriber for topic: {}", reply_key);
        }
        return true;
    }
    catch (const zenoh::ZException& e) {
        ERRORLOG("Failed to create reply subscriber: {}", e.what());
        return false;
    }
}

void LogTopic::handle_reply_message(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        size_t size = sample.get_payload().size();
        cn::seisys::dds::LogData replyData;
        replyData.ParsePartialFromArray(data.data(), size);

        // 将回复消息加入队列
        std::lock_guard<std::mutex> lock(queue_mutex_);
        reply_queue_.push(replyData);
    }
    catch (const std::exception& e) {
        ERRORLOG("Error processing reply message: {}", e.what());
    }
}