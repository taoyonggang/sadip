#include "../../utils/pch.h"
//系统头文件
#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <boost/filesystem.hpp>
//自编头文件
#include "../../utils/ini/INIReader.h"
#include "../utils/log/BaseLog.h"
#include "../db/DbBase.h"
#include "../utils/FileSplit.h"
#include "../../utils/msgQueue.h"
#include "../utils/uuid.h"
#include "NodeTopic.h"
#include "../utils/SnowFlake.h"

using namespace cn::seisys::dds;

SnowFlake* g_sf = new SnowFlake(1, 3);
// 静态成员初始化
const std::map<std::string, int> NodeTopic::arch_map_ = {
    {"archType_UNKNOWN", 0},
    {"amd64", 1},
    {"x86", 2},
    {"arm64v8", 3}
};

const std::map<std::string, int> NodeTopic::os_map_ = {
    {"OSType_UNKNOWN", 0},
    {"windows", 1},
    {"linux", 2}
};

NodeTopic::NodeTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
}

NodeTopic::~NodeTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void NodeTopic::start_writer(bool wait) {
    if (!writer_thread_) {
        writer_running_ = true;
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&NodeTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }
}

void NodeTopic::start_reader(bool wait) {
    if (!reader_thread_) {
        reader_running_ = true;
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&NodeTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}


void NodeTopic::writer_worker() {
    DbBase* db = DbBase::getDbInstance();
    //创建数据下发发送端，回执数据接收端
    try {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize node writer base ZenohBase");
            return ;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者
        if (!node_publisher_) {
            node_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topic_)
            );
        }

        // 回执数据接收端创建，并入库回执数据
         // 初始化回复订阅者
        zenoh::KeyExpr reply_key(topic_info_.topicReply_);
        auto on_reply = [this](const zenoh::Sample& sample) {
            handle_reply(sample);
            };

        auto on_drop_reply = []() {
            DEBUGLOG("Node msg reply subscriber dropped");
            };

        reply_subscriber_.emplace(session_->declare_subscriber(
            reply_key,
            std::move(on_reply),
            std::move(on_drop_reply)
        ));
    }
    catch (const std::exception& e) {
        ERRORLOG("Init node writer failed: {}", e.what());
        return;
    }
    
    while (is_writer_) {
        try {
            char task_sql[1024];
            sprintf(task_sql, "select `uuid`,`to_node_id`,`to_node_name`,`src_node_id`,`to_config_path`,"
                "`src_config_path`,`config`,`config_hash` from node_msg where comm_status_id=1 and to_node_id='%s'",
                topic_info_.partition_.c_str());

            soci::rowset<soci::row> rows = db->select(task_sql);
            
            for (const soci::row& row : rows) {
                cn::seisys::dds::Node node_msg;
                // 填充节点消息
                node_msg.set_uuid(row.get<std::string>(0));
                node_msg.set_to_node_id(row.get<std::string>(1));
                node_msg.set_to_node_name(row.get<std::string>(2));
                node_msg.set_src_node_id(row.get<std::string>(3));
                node_msg.set_to_config_path(row.get<std::string>(4));
                node_msg.set_src_config_path(row.get<std::string>(5));
                
                std::string conf = row.get<std::string>(6);
                std::vector<unsigned char> config_data(conf.begin(), conf.end());
                node_msg.set_config(config_data.data(), config_data.size());
                node_msg.set_config_hash(row.get<std::string>(7));

                // 设置架构和操作系统类型
                auto arch_it = arch_map_.find(utility::node::nodeInfo_.archType_);
                auto os_it = os_map_.find(utility::node::nodeInfo_.osType_);
                
                node_msg.set_arch(static_cast<cn::seisys::dds::ArchType>(
                    arch_it != arch_map_.end() ? arch_it->second : 0));
                node_msg.set_os(static_cast<cn::seisys::dds::OSType>(
                    os_it != os_map_.end() ? os_it->second : 0));
                
                node_msg.set_domain(utility::node::nodeInfo_.domain_);
                node_msg.set_to_group(utility::node::nodeInfo_.group_);

                // 管理节点自我配置处理
                /*if (node_msg.src_node_id() == node_msg.to_node_id()) {
                    update_config(node_msg, 
                        utility::node::nodeInfo_.nodeName_ + " config update success!",
                        2);
                    continue;
                }
                */

                if (!send_node(node_msg)) {
                    ERRORLOG("send node message to {} failed!", node_msg.to_node_id());
                }
                else
                {
                    INFOLOG("Send node message to {} sucess!", node_msg.to_node_id());
                }
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("Writer worker error: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}

void NodeTopic::reader_worker() {
    //创建数据接收端、回执发送端连接 
    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize node msg reader base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（NodeTopic）
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_node(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("Node msg subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));

        //回执发送者创建（NodeReplyTopic）
        if (!reply_publisher_) {
            reply_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topicReply_)
            );
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Init node reader failed: {}", e.what());
        return;
    }
 

     while (is_reader_) {
         std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
}


bool NodeTopic::send_node(const cn::seisys::dds::Node& node_msg) {
    try {
        std::vector<uint8_t> serialized_msg(node_msg.ByteSize() + 1);
        node_msg.SerializeToArray(serialized_msg.data(), node_msg.ByteSize());
        node_publisher_->put(serialized_msg);
        
        //构建统计发送端监控数据
        stat_writer_mon_data(node_msg.ByteSize(),node_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send node failed: {}", e.what());
        return false;
    }
}


// 其他方法（send_reply, handle_node, handle_reply）的实现与 PingTopic 类似，
// 只需将消息类型改为 Node 和 NodeReply

void NodeTopic::process_config(const cn::seisys::dds::Node& node, cn::seisys::dds::NodeReply& reply) {
    std::string configPath = node.to_config_path();
    std::string tmpDir = configPath + "_tmp" + "/";
    
    utility::file::mkdir(tmpDir);
    
    boost::filesystem::path filePath(configPath);
    filePath.make_preferred();
    
    // 备份当前配置
    vector<string> fileName;
    utility::string::split(filePath.string(), fileName, boost::filesystem::path::preferred_separator);
    string copyFilename = tmpDir + fileName.back();
    utility::file::move(configPath, copyFilename);

    // 写入新配置
    utility::file::write_file(configPath.c_str(), node.config().data(), node.config().size());

    // 验证配置
    INIReader reader(configPath);
    string newNodeId = reader.GetString("base", "node_id", "");
    string newNodeName = reader.GetString("base", "name", "");
    DEBUGLOG("new node name: {}", newNodeName.c_str());
  
    
    reply.set_uuid(utility::uuid::generate());
    reply.set_to_node_id(node.src_node_id());
    reply.set_src_node_id(node.to_node_id());
    reply.set_reply_uuid(node.uuid());
    reply.set_to_group(node.to_group());
    
    if (newNodeId == utility::node::nodeInfo_.nodeId_) {
        reply.set_desc(utility::node::nodeInfo_.nodeName_ + " config update success!");
        reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
    } else {
        reply.set_desc("Because nodeId unmatch, " + utility::node::nodeInfo_.nodeName_ + " config update fail!");
        utility::file::move(copyFilename, configPath);
        reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
    }

    uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    reply.set_created_at(now);
    reply.set_updated_at(now);
}



bool NodeTopic::send_reply(const NodeReply& reply_msg) {
    try {
        std::vector<uint8_t> serialized_msg(reply_msg.ByteSize());
        reply_msg.SerializeToArray(serialized_msg.data(), reply_msg.ByteSize());
        reply_publisher_->put(serialized_msg);

        //构建回执发送监控数据
        stat_reply_writer_mon_data(reply_msg.ByteSize(), reply_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send reply failed: {}", e.what());
        return false;
    }
}

void NodeTopic::handle_node(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        Node node_msg;
        node_msg.ParseFromArray(data.data(), data.size());

        // 构建数据接收监控数据
        stat_reader_mon_data(topic_info_.topic_, node_msg.src_node_id(),node_msg.ByteSize());

        if (topic_info_.needReply_)
        {
            if (!node_msg.uuid().empty()) {
                cn::seisys::dds::NodeReply reply;
                process_config(node_msg, reply);

                if (!send_reply(reply)) {
                    ERRORLOG("Failed to send reply for node {}", node_msg.uuid());
                }
            }
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle node data failed: {}", e.what());
    }
}

void NodeTopic::handle_reply(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        NodeReply reply;
        reply.ParseFromArray(data.data(), data.size());

        // 构建回执接收监控数据
        stat_reply_reader_mon_data(topic_info_.topicReply_, reply.src_node_id(),reply.ByteSize());

        DbBase* db = DbBase::getDbInstance();
        try {
            //处理回执--入库
            if (!reply.uuid().empty())
            {
                int ret = 1;
                char updateReplyStat[1024];
                uint64_t id = g_sf->nextId();
                string nodeReplyUuid = reply.uuid();
                string nodeReplyToNodeId = reply.to_node_id();
                string nodeReplyToNodeName = reply.to_node_name();
                string nodeReplyToGroup = reply.to_group();
                string nodeReplyReplyUuid = reply.reply_uuid();
                string description = reply.desc();
                string groupId = reply.to_group();
                int nodeReplyStatus = reply.status();

                // 更新node_msg表状态
                sprintf(updateReplyStat, "update node_msg set comm_status_id=%d where uuid='%s'", nodeReplyStatus, nodeReplyReplyUuid.c_str());
                ret = db->excuteSql(updateReplyStat);
                if (ret < 0)
                {
                    ERRORLOG("Node Update Fail:{}", updateReplyStat);
                }

                // 更新nodeReply表
                char replyInfo[1024];
                sprintf(replyInfo, "insert into node_msg_reply (uuid,to_node_id,to_node_name,reply_uuid,description,\
                 to_group,comm_status_id) values ('%s','%s','%s','%s','%s','%s',%d)",  nodeReplyUuid.c_str(), \
                    nodeReplyToNodeId.c_str(), nodeReplyToNodeName.c_str(), nodeReplyReplyUuid.c_str(), description.c_str(), \
                    groupId.c_str(), nodeReplyStatus);
                ret = db->excuteSql(replyInfo);
                if (ret < 0)
                {
                    ERRORLOG("Node Reply Update Fail:{}", replyInfo);
                }
            }

        }
        catch (const std::exception& e)
        {
            ERRORLOG("Insert reply to DB failed: {}", e.what());
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle reply failed: {}", e.what());
    }
}