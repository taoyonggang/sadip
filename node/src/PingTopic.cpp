#include "PingTopic.h"
#include "../db/DbBase.h"

using namespace cn::seisys::dds;

PingTopic::PingTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
}

PingTopic::~PingTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void PingTopic::start_writer(bool wait) {
    //创建ping消息发送端，只有发送端，不需要回执接收端

    if (!writer_thread_) {
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&PingTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }
}

void PingTopic::start_reader(bool wait) {

    if (!reader_thread_) {
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&PingTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}

void PingTopic::writer_worker() {

    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize ping writer base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者
        if (!ping_publisher_) {
            ping_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topic_)
            );
        }
    }
    catch (const std::exception& e)
    {
        ERRORLOG("Init ping writer pub failed: {}", e.what());
        return;
    }
    while (is_writer_) {
        try
        {   
            //构建心跳消息，15秒发送一次
            Ping ping_msg;
            uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            ping_msg.set_src_node_id(topic_info_.nodeId_);
            ping_msg.set_to_node_id(topic_info_.monNodeId_);
            ping_msg.set_state(::cn::seisys::dds::NodeState::ONLINE);
            ping_msg.set_created_at(now);
            ping_msg.set_desc("online");
            if (!ping_msg.src_node_id().empty()) {
                if (!send_ping(ping_msg)) {
                    ERRORLOG("Failed to send ping message");
                }
                else
                {
                    INFOLOG("Send ping sucess at:{}",now);
                }
            }
        }
        catch (const std::exception& e)
        {
            ERRORLOG("Ping writer worker error: {}", e.what());
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(15000));
    }
}

void PingTopic::reader_worker() {
    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize ping reader base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（NodePingTopic）只有订阅，不需要回执
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_ping(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("Ping Subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));
    }
    catch (const std::exception& e)
    {
        ERRORLOG("Ping reader worker error: {}", e.what());
    }
    
    while (is_reader_) {

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}

bool PingTopic::send_ping(const Ping& ping_msg) {
    try {
        std::vector<uint8_t> serialized_msg(ping_msg.ByteSize() + 1);
        ping_msg.SerializeToArray(serialized_msg.data(), ping_msg.ByteSize());
        ping_publisher_->put(serialized_msg);

        //构建统计发送端监控数据
        stat_writer_mon_data(ping_msg.ByteSize(), ping_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send ping failed: {}", e.what());
        return false;
    }
}



void PingTopic::handle_ping(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        size_t size = sample.get_payload().size();
        Ping ping_msg;
        ping_msg.ParsePartialFromArray(data.data(), size);
       /* std::lock_guard<std::mutex> lock(queue_mutex_);
        ping_queue_.push(ping_msg);*/

        stat_reader_mon_data(topic_info_.topic_, ping_msg.src_node_id(), ping_msg.ByteSize());
        
        //更新心跳消息入库
        DbBase* db = DbBase::getDbInstance();
        //更新心跳状态
        uint64_t nodeStat = ping_msg.state();
        uint64_t updateTime = ping_msg.created_at() / 1000;
        char pingSql[1024];
        sprintf(pingSql, "insert into node_ping(`node_id`,`node_status_id`,`description`,`updated_at`) values('%s',%llu,'%s',%llu) on duplicate key update \
            node_status_id=%llu,description='%s',updated_at=%llu", ping_msg.src_node_id().c_str(), nodeStat, ping_msg.desc().c_str(), updateTime, nodeStat, ping_msg.desc().c_str(), updateTime);

        int ret = db->excuteSql(pingSql);
        if (ret < 0)
        {
            ERRORLOG("Ping Update Fail:{}", pingSql);
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle ping failed: {}", e.what());
    }
}

//void PingTopic::handle_reply(const zenoh::Sample& sample) {
//    try {
//        std::vector<uint8_t> data = sample.get_payload().as_vector();
//        size_t size = sample.get_payload().size();
//        NodeReply reply_msg;
//        reply_msg.ParsePartialFromArray(data.data(), size);
//        std::lock_guard<std::mutex> lock(queue_mutex_);
//        reply_queue_.push(reply_msg);
//    }
//    catch (const std::exception& e) {
//        ERRORLOG("Handle reply failed: {}", e.what());
//    }
//}