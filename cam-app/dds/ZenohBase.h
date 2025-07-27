// ZenohBase.h
#pragma once
#include "../utils/pch.h"
#include"../utils/uuid.h"
#include <zenoh.hxx>
#include <chrono>
#include <memory>
#include <string>
#include "../utils/log/BaseLog.h"
#include "TopicInfo.h"
#include "monitor.pb.h"

using namespace zenoh;
using namespace std::chrono;
using namespace cn::seisys::dds;
using namespace std;

class ZenohBase {
protected:
    std::shared_ptr<zenoh::Session> session_{ nullptr };
    //zenoh::Session* session_{ nullptr };
    std::string topic_name_;
    bool is_active_{false};
    TopicInfo topic_info_;
    std::shared_ptr<zenoh::Publisher> write_mon_publisher_;//消息发送、回执发送监控发送端
    std::shared_ptr<zenoh::Publisher> reader_mon_publisher_;//消息接收、回执接收监控发送端
    std::atomic<bool> stop_{ false };
    std::thread monitoring_thread_;


public:
    ZenohBase() = default;
    virtual ~ZenohBase() = default;

    virtual bool init(const TopicInfo& topic_info) {
        topic_info_ = topic_info;
        try {
            // ʹ���µ� Config API
#ifdef ZENOHCXX_ZENOHC
            init_log_from_env_or("debug");
            auto config = zenoh::Config::from_file(topic_info_.configPath_);
#else
            auto config = zenoh::Config::create_default();
#endif
            session_ = std::make_shared<zenoh::Session>(zenoh::Session::open(std::move(config)));
            topic_name_ = topic_info_.topic_;
            startWriterMon(); // 启动监控线程
            return true;
        } catch (const zenoh::ZException& e) {
            ERRORLOG("Failed to create Zenoh session: {}", e.what());
            return false;
        }
    }

    virtual void stop() {
        stopWriterMon(); // 停止监控线程
        is_active_ = false;
        if (session_) {
            session_->close();
        }
    }

    void create_writer_mon_pub()
    {
        // 初始化发布、回执发送监控发送端
        if (!write_mon_publisher_) {
            write_mon_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.monTopicName_)
            );
        }
    }
    

    //发送数据和统计监控信息（）
    void stat_writer_mon_data(size_t length, string toNodeId) {
        if (length > 0) {
            TopicMonInfo monData;
            monData.set_uuid(utility::uuid::generate());
            monData.set_topicname(topic_info_.topic_);
            monData.set_srcnodeid(topic_info_.nodeId_);
            //monData.set_srcnodename(topic_info_.nodeName_);
            //monData.toNodeId(topicInfo_.monNodeId_);
            monData.set_tonodeid(toNodeId);
            monData.set_ncount(1);
            monData.set_srctype(1);
            monData.set_nsize(length);
            monData.set_progarmname(topic_info_.programName_);
            write_mon_msg(&monData);
        }
    };

    //发送回执和统计回执监控信息
    void stat_reply_writer_mon_data(size_t length, string toNodeId) {
        if (length > 0) {
            TopicMonInfo monData;
            monData.set_uuid(utility::uuid::generate());
            monData.set_topicname(topic_info_.topicReply_);
            monData.set_srcnodeid(topic_info_.nodeId_);
            //monData.set_srcnodename(topic_info_.nodeName_);
            //monData.toNodeId(topicInfo_.monNodeId_);
            monData.set_tonodeid(toNodeId);
            monData.set_ncount(1);
            monData.set_srctype(1);
            monData.set_nsize(length);
            monData.set_progarmname(topic_info_.programName_);
            write_reply_mon_msg(&monData);
        }
    };

    // 接收并统计监控信息
    void stat_reader_mon_data(string topicName,string srcNodeId,size_t length) {
        if (length> 0) {
            TopicMonInfo monData;
            monData.set_uuid(utility::uuid::generate());
            monData.set_topicname(topicName);
            monData.set_srcnodeid(srcNodeId);
            //monData.set_srcnodename(srcNodeName);
            //monData.toNodeId(topicInfo_.monNodeId_);
            monData.set_tonodeid(topic_info_.nodeId_);
            monData.set_ncount(1);
            monData.set_srctype(2);
            monData.set_nsize(length);
            monData.set_progarmname(topic_info_.programName_);
            reader_mon_msg(&monData);
        }
    };

    // 接收回执并统计监控信息
    void stat_reply_reader_mon_data( string topicName, string srcNodeId,size_t length) {
        if (length > 0) {
            TopicMonInfo monData;
            monData.set_uuid(utility::uuid::generate());
            monData.set_topicname(topicName);
            monData.set_srcnodeid(srcNodeId);
            //monData.set_srcnodename(srcNodeName);
            //monData.toNodeId(topicInfo_.monNodeId_);
            monData.set_tonodeid(topic_info_.nodeId_);
            monData.set_ncount(1);
            monData.set_srctype(2);
            monData.set_nsize(length);
            monData.set_progarmname(topic_info_.programName_);
            reader_reply_mon_msg(&monData);
        }
    };

protected:

    std::string get_full_key() const {
        return topic_name_;
    }

    virtual int write_mon_msg(TopicMonInfo* data) {
        if (!needMonitor_) {
            WARNLOG("needMonitor_ is false,can't init monitor publisher.");
            return 1;
        }
        DEBUGLOG("Base::writer_mon_msg()");
        if (data == nullptr) {
            ERRORLOG("data == nullptr");
            return 1;
        }
        else {
            monPubInfo_.set_uuid(((TopicMonInfo*)data)->uuid());
            monPubInfo_.set_domain(topic_info_.domain_);
            monPubInfo_.set_topicname(((TopicMonInfo*)data)->topicname());
            monPubInfo_.set_cycle(topic_info_.monSendSleep_);
            monPubInfo_.set_tonodeid(((TopicMonInfo*)data)->tonodeid());
            //monPubInfo_.set_tonodename(((TopicMonInfo*)data)->tonodename());
            monPubInfo_.set_srcnodeid(((TopicMonInfo*)data)->srcnodeid());
            //monPubInfo_.set_srcnodename(((TopicMonInfo*)data)->srcnodename());
            monPubInfo_.set_updatetime(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
            monPubInfo_.set_nsize(monPubInfo_.nsize() + ((TopicMonInfo*)data)->nsize());
            monPubInfo_.set_ncount(monPubInfo_.ncount() + 1);
            monPubInfo_.set_srctype(((TopicMonInfo*)data)->srctype());
            monPubInfo_.set_progarmname(((TopicMonInfo*)data)->progarmname());
        }
        return 0;
    }

    virtual int write_reply_mon_msg(TopicMonInfo* data) {
        if (!needMonitor_) {
            WARNLOG("needMonitor_ is false,can't init monitor publisher.");
            return 1;
        }
        DEBUGLOG("Base::writer_mon_reply_msg()");
        if (data == nullptr) {
            ERRORLOG("data == nullptr");
            return 1;
        }
        else {
            monReplyPubInfo_.set_uuid(((TopicMonInfo*)data)->uuid());
            monReplyPubInfo_.set_domain(topic_info_.domain_);
            monReplyPubInfo_.set_topicname(((TopicMonInfo*)data)->topicname());
            monReplyPubInfo_.set_cycle(topic_info_.monSendSleep_);
            monReplyPubInfo_.set_tonodeid(((TopicMonInfo*)data)->tonodeid());
            //monReplyPubInfo_.set_tonodename(((TopicMonInfo*)data)->tonodename());
            monReplyPubInfo_.set_srcnodeid(((TopicMonInfo*)data)->srcnodeid());
            //monReplyPubInfo_.set_srcnodename(((TopicMonInfo*)data)->srcnodename());
            monReplyPubInfo_.set_updatetime(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
            monReplyPubInfo_.set_nsize(monReplyPubInfo_.nsize() + ((TopicMonInfo*)data)->nsize());
            monReplyPubInfo_.set_ncount(monReplyPubInfo_.ncount() + 1);
            monReplyPubInfo_.set_srctype(((TopicMonInfo*)data)->srctype());
            monReplyPubInfo_.set_progarmname(((TopicMonInfo*)data)->progarmname());
            //monReplyPubInfo_.srcType(monPubInfo_.srcType());
        }
        return 0;
    }

    virtual int reader_mon_msg(TopicMonInfo* data) {
        if (!needMonitor_) {
            WARNLOG("needMonitor_ is false,can't init monitor publisher.");
            return 1;
        }
        DEBUGLOG("Base::reader_mon_msg()");
        if (data == nullptr) {
            ERRORLOG("data == nullptr");
            return 1;
        }
        else {
            monReaderInfo_.set_uuid(((TopicMonInfo*)data)->uuid());
            monReaderInfo_.set_domain(topic_info_.domain_);
            monReaderInfo_.set_topicname(((TopicMonInfo*)data)->topicname());
            monReaderInfo_.set_cycle(topic_info_.monSendSleep_);
            monReaderInfo_.set_tonodeid(((TopicMonInfo*)data)->tonodeid());
            //monReaderInfo_.set_tonodename(((TopicMonInfo*)data)->tonodename());
            monReaderInfo_.set_srcnodeid(((TopicMonInfo*)data)->srcnodeid());
            //monReaderInfo_.set_srcnodename(((TopicMonInfo*)data)->srcnodename());
            monReaderInfo_.set_updatetime(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
            monReaderInfo_.set_nsize(monReaderInfo_.nsize() + ((TopicMonInfo*)data)->nsize());
            monReaderInfo_.set_ncount(monReaderInfo_.ncount() + 1);
            monReaderInfo_.set_srctype(((TopicMonInfo*)data)->srctype());
            monReaderInfo_.set_progarmname(((TopicMonInfo*)data)->progarmname());
        }
        return 0;
    }

    virtual int reader_reply_mon_msg(TopicMonInfo* data) {
        if (!needMonitor_) {
            WARNLOG("needMonitor_ is false,can't init monitor publisher.");
            return 1;
        }
        DEBUGLOG("Base::reader_mon_reply_msg()");
        if (data == nullptr) {
            ERRORLOG("data == nullptr");
            return 1;
        }
        else {
            monReplyReaderInfo_.set_uuid(((TopicMonInfo*)data)->uuid());
            monReplyReaderInfo_.set_domain(topic_info_.domain_);
            monReplyReaderInfo_.set_topicname(((TopicMonInfo*)data)->topicname());
            monReplyReaderInfo_.set_cycle(topic_info_.monSendSleep_);
            monReplyReaderInfo_.set_tonodeid(((TopicMonInfo*)data)->tonodeid());
            //monReplyReaderInfo_.set_tonodename(((TopicMonInfo*)data)->tonodename());
            monReplyReaderInfo_.set_srcnodeid(((TopicMonInfo*)data)->srcnodeid());
            //monReplyReaderInfo_.set_srcnodename(((TopicMonInfo*)data)->srcnodename());
            monReplyReaderInfo_.set_updatetime(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
            monReplyReaderInfo_.set_nsize(monReplyReaderInfo_.nsize() + ((TopicMonInfo*)data)->nsize());
            monReplyReaderInfo_.set_ncount(monReplyReaderInfo_.ncount() + 1);
            monReplyReaderInfo_.set_srctype(((TopicMonInfo*)data)->srctype());
            monReplyReaderInfo_.set_progarmname(((TopicMonInfo*)data)->progarmname());
        }
        return 0;
    }

private:
    TopicMonInfo monPubInfo_;//发送端监控消息体
    TopicMonInfo monReplyPubInfo_;//回执发送端监控消息体
    TopicMonInfo monReaderInfo_;//接收端监控消息体
    TopicMonInfo monReplyReaderInfo_;//回执接收端监控消息体
    bool needMonitor_ = true;

    void startWriterMon() {
        stop_ = false;
        monitoring_thread_ = std::thread([this] {
            while (needMonitor_&& !stop_) {
                try
                {
                    std::this_thread::sleep_for(std::chrono::seconds(topic_info_.monSendSleep_)); // 根据实际需求在配置中调整睡眠时间
                    // 发送端、回执发送端监控消息发送
                    if (write_mon_publisher_ != nullptr) {
                        if (monPubInfo_.ncount() > 0) {
                            // TopicMonInfo消息体序列化
                            
                            std::vector<uint8_t> serialized_msg(monPubInfo_.ByteSize() + 1);
                            monPubInfo_.SerializeToArray(serialized_msg.data(), monPubInfo_.ByteSize());
                            write_mon_publisher_->put(serialized_msg);
                            DEBUGLOG("send mon pub info success,mon topic name:{},mon srctype:{}, data size:{}",monPubInfo_.topicname(), monPubInfo_.srctype(), monPubInfo_.ByteSize());
                            /*INFOLOG("send writer mon info :{}", monPubInfo_.DebugString());*/
                            monPubInfo_.set_ncount(0);
                            monPubInfo_.set_nsize(0);
                        }

                        if (monReplyPubInfo_.ncount() > 0) {
                            std::vector<uint8_t> serialized_reply_msg(monReplyPubInfo_.ByteSize() + 1);
                            monReplyPubInfo_.SerializeToArray(serialized_reply_msg.data(), monReplyPubInfo_.ByteSize());
                            write_mon_publisher_->put(serialized_reply_msg);
                            DEBUGLOG("send mon reply pub info success,mon topic name:{},mon srctype:{}, data size:{}", monReplyPubInfo_.topicname(), monReplyPubInfo_.srctype(), monReplyPubInfo_.ByteSize());
                            monReplyPubInfo_.set_ncount(0);
                            monReplyPubInfo_.set_nsize(0);
                        }

                        if (monReaderInfo_.ncount() > 0) {
                            // TopicMonInfo消息体序列化
                            std::vector<uint8_t> serialized_msg(monReaderInfo_.ByteSize() + 1);
                            monReaderInfo_.SerializeToArray(serialized_msg.data(), monReaderInfo_.ByteSize());
                            write_mon_publisher_->put(serialized_msg);
                            DEBUGLOG("send mon reader info success,mon topic name:{},mon srctype:{}, data size:{}", monReaderInfo_.topicname(), monReaderInfo_.srctype(), monReaderInfo_.ByteSize());
                            monReaderInfo_.set_ncount(0);
                            monReaderInfo_.set_nsize(0);
                        }

                        if (monReplyReaderInfo_.ncount() > 0) {
                            std::vector<uint8_t> serialized_reply_msg(monReplyReaderInfo_.ByteSize() + 1);
                            monReplyReaderInfo_.SerializeToArray(serialized_reply_msg.data(), monReplyReaderInfo_.ByteSize());
                            write_mon_publisher_->put(serialized_reply_msg);
                            DEBUGLOG("send mon reply reader info success,mon topic name:{},mon srctype:{}, data size:{}", monReplyReaderInfo_.topicname(), monReplyReaderInfo_.srctype(), monReplyReaderInfo_.ByteSize());
                            monReplyReaderInfo_.set_ncount(0);
                            monReplyReaderInfo_.set_nsize(0);
                        }

                    }
                }
                catch (const std::exception& e)
                {
                    ERRORLOG("Writer monitoring thread error: {},writer topic name:{},reply writer topic name:{}", e.what(), monPubInfo_.topicname(), monReplyPubInfo_.topicname());
                }
               
            }
        });
    }

    void stopWriterMon() {
        stop_ = true;
        if (monitoring_thread_.joinable()) {
            monitoring_thread_.join();
        }
        DEBUGLOG("Writer monitoring thread stopped.");
    }

};