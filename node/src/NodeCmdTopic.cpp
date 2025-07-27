#include "NodeCmdTopic.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "../utils/log/BaseLog.h"
#include "../../utils/msgQueue.h"
#include "../../utils/split.h"
#include <chrono>
#include <boost/filesystem.hpp>


NodeCmdTopic::NodeCmdTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
}

NodeCmdTopic::~NodeCmdTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void NodeCmdTopic::start_writer(int argc, char** argv, bool wait) {
    if (!writer_thread_) {
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&NodeCmdTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }
}

void NodeCmdTopic::start_reader(int argc, char** argv, bool wait) {
    if (!reader_thread_)
    {
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&NodeCmdTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}

void NodeCmdTopic::writer_worker() {
    //auto* topic =  (NodeCmdTopic*)(arg);
    DbBase* db = DbBase::getDbInstance();
   
    //创建数据下发发送端，回执数据接收端
    try {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize cmd writer base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者(NodeCmdTopic)
        if (!cmd_publisher_) {
            cmd_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topic_)
            );
        }

        // 回执数据接收端创建(NodeCmdReply)，并入库回执数据
        zenoh::KeyExpr reply_key(topic_info_.topicReply_);
        auto on_reply = [this](const zenoh::Sample& sample) {
            handle_reply(sample);
            };

        auto on_drop_reply = []() {
            DEBUGLOG("Node cmd reply Subscriber dropped");
            };

        reply_subscriber_.emplace(session_->declare_subscriber(
            reply_key,
            std::move(on_reply),
            std::move(on_drop_reply)
        ));
    }
    catch (const std::exception& e) {
        ERRORLOG("Init node cmd writer failed: {}", e.what());
        return;
    }

    while (is_writer_) {
        try {
            char task_sql[1024];
            sprintf(task_sql, "select `uuid`,`to_node_id`,`to_node_name`,`src_node_id`,"
                "`src_node_name`,`to_location`,`to_group`,`cmd`,`paras`,`created_at`,"
                "`updated_at`,`cmd_type_id`,`id`,`paras1`,`paras2`,`podman_cmd_id` "
                "from node_cmd where comm_status_id=1");

            soci::rowset<soci::row> rows = db->select(task_sql);
            
            for (const soci::row& row : rows) {
                NodeCmd cmd_msg;
                fill_cmd_message(cmd_msg, row);
                
                if (send_cmd(cmd_msg)) {
                    update_cmd_status(db, row.get<long long>(12), 2);
                    INFOLOG("NodeCmd: send to to_node_id:{} cmd_type:{} success at: {}",
                        cmd_msg.to_node_id(), cmd_msg.cmd_type(),
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count());
                }
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("Node cmd writer worker error: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }
}

void NodeCmdTopic::reader_worker() {
    //auto* topic = (NodeCmdTopic*)(arg);
    
    try {
        //创建数据接收端、回执发送端连接  
   // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize cmd reader base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（NodeCmdTopic）
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_cmd(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("Node cmd Subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));

        //回执发送者创建（NodeCmdReplyTopic）
        if (!reply_publisher_) {
            reply_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topicReply_)
            );
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Init cmd reader failed: {}", e.what());
    }

    while (is_reader_) {
        if (topic_info_.needReply_) {
            process_app_cmd_replies();
            process_file_cmd_replies();
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool NodeCmdTopic::send_cmd(const NodeCmd& cmd_msg) {
    try {
        std::vector<uint8_t> serialized_msg(cmd_msg.ByteSizeLong() + 1);
        cmd_msg.SerializePartialToArray(serialized_msg.data(), cmd_msg.ByteSizeLong());
        cmd_publisher_->put(serialized_msg);

        //构建统计发送端监控数据
        stat_writer_mon_data(cmd_msg.ByteSize(), cmd_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send command failed: {}", e.what());
        return false;
    }
}

bool NodeCmdTopic::send_reply(const NodeCmdReply& reply_msg) {
    try {
        std::vector<uint8_t> serialized_msg(reply_msg.ByteSizeLong() + 1);
        reply_msg.SerializePartialToArray(serialized_msg.data(), reply_msg.ByteSizeLong());
        reply_publisher_->put(serialized_msg);

        //构建回执发送监控数据
        stat_reply_writer_mon_data(reply_msg.ByteSize(), reply_msg.to_node_id());

        INFOLOG("Reply sent successfully: {}", reply_msg.uuid());
        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send reply failed: {}", e.what());
        return false;
    }
}

void NodeCmdTopic::handle_cmd(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        NodeCmd cmd_msg;
        cmd_msg.ParsePartialFromArray(data.data(), data.size());

        
        
        if (topic_info_.needReply_ && topic_info_.nodeId_==cmd_msg.to_node_id()) {
            if (cmd_msg.cmd_type() == ::CmdType::LS ||
                cmd_msg.cmd_type() == ::CmdType::MKDIR ||
                cmd_msg.cmd_type() == ::CmdType::DELDIR ||
                cmd_msg.cmd_type() == ::CmdType::DEL ||
                cmd_msg.cmd_type() == ::CmdType::SENDFILE ||
                cmd_msg.cmd_type() == ::CmdType::ZIP ||
                cmd_msg.cmd_type() == ::CmdType::DZIP ||
                cmd_msg.cmd_type() == ::CmdType::GETS3FILE ||
                cmd_msg.cmd_type() == ::CmdType::PUTS3FILE ||
                cmd_msg.cmd_type() == ::CmdType::LSS3BUCKETS ||
                cmd_msg.cmd_type() == ::CmdType::DELS3FILE ||
                cmd_msg.cmd_type() == ::CmdType::ADDS3WATCH ||
                cmd_msg.cmd_type() == ::CmdType::RMS3WATCH ||
                cmd_msg.cmd_type() == ::CmdType::LSS3WATCH ||
                cmd_msg.cmd_type() == ::CmdType::MKFILE ||
                cmd_msg.cmd_type() == ::CmdType::CPFILE)
            {
                // 入队文件相关cmd命令
                stat_reader_mon_data(topic_info_.topic_, cmd_msg.src_node_id(), cmd_msg.ByteSize());
                utility::msg::fileDataQueueIn_.enqueue(cmd_msg);
            }
            else
            {
                //入队app相关命令
                stat_reader_mon_data(topic_info_.topic_, cmd_msg.src_node_id(), cmd_msg.ByteSize());
                utility::msg::mainCmdQueueIn_.enqueue(cmd_msg);
            }
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle command failed: {}", e.what());
    }
}

void NodeCmdTopic::handle_reply(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        NodeCmdReply reply_msg;
        reply_msg.ParsePartialFromArray(data.data(), data.size());
        

        // 构建回执接收监控数据
        stat_reply_reader_mon_data(topic_info_.topicReply_, reply_msg.src_node_id(), reply_msg.ByteSize());

        /*cmd回执数据入库*/
        DbBase* db = DbBase::getDbInstance();
        try {
            int ret = 1;
            std::string uuid = reply_msg.uuid();
            int cmdType = reply_msg.cmd_type();
            std::string srcNodeId = reply_msg.src_node_id();
            std::string srcNodeName = reply_msg.src_node_name();
            std::string toNodeId = reply_msg.to_node_id();
            std::string toNodeName = reply_msg.to_node_name();
            std::string cmdReplyUuid = reply_msg.cmd_reply_uuid();
            std::string description = reply_msg.desc();
            std::string podmanDesc = reply_msg.desc_str();
            /* uint64_t createdAt = nodeCmdReply.createdAt();
             uint64_t updatedAt = nodeCmdReply.updatedAt();*/
            int status = reply_msg.status();
            std::string resStr = "";
            size_t res_size = 0;
            size_t des_size = 0;
            //FileInfos result = reply_msg.result();
            if (reply_msg.result_size()>0)
            {
                rapidjson::Document document;
                document.SetObject();
                rapidjson::Value strValue("", document.GetAllocator());
                rapidjson::Value arrayPlugin(rapidjson::kArrayType);
                rapidjson::Document::AllocatorType& allocator = document.GetAllocator();
                rapidjson::Value object1(rapidjson::kObjectType);
                rapidjson::Value alarmArray(rapidjson::kArrayType);
                rapidjson::Value alarmArrayFile(rapidjson::kArrayType);

                for (int i = 0; i < reply_msg.result_size(); i++)
                {
                    //char strParamCode[128] = { 0 };
                    //sprintf(strParamCode, "param%d", i + 1);
                    string pathName = reply_msg.result(i).file_path_name();
                    size_t size = reply_msg.result(i).file_path_name().size();   //result[i].filePathName().size();
                    pathName = utility::string::subreplace(pathName, "\\", "/");
                    //pathName = utility::string::subreplace(pathName, "./", "");
                    auto strParamCode = new char(size);
                    //sprintf(strParamCode, "%s", pathName);
                    //std::vector<std::string> filePaths;
                    //utility::string::split(result[i].filePathName(), filePaths, '\\');

                    rapidjson::Value objectTemp(rapidjson::kObjectType);
                    rapidjson::Value valueParamCode(pathName.c_str(), document.GetAllocator());
                    objectTemp.AddMember("filePathName", valueParamCode, allocator);
                    objectTemp.AddMember("isDir", reply_msg.result(i).is_dir(), allocator);
                    objectTemp.AddMember("fileSize", reply_msg.result(i).file_size(), allocator);
                    objectTemp.AddMember("timestamp", reply_msg.result(i).timestamp(), allocator);

                    if (reply_msg.result(i).is_dir() == true)
                    {
                        alarmArray.PushBack(objectTemp, allocator);
                    }
                    else
                    {
                        alarmArrayFile.PushBack(objectTemp, allocator);
                    }
                    //delete strParamCode;
                }

                for (rapidjson::SizeType i = 0; i < alarmArrayFile.Size(); i++) {
                    rapidjson::Value value(alarmArrayFile[i], document.GetAllocator());
                    alarmArray.PushBack(value, document.GetAllocator());
                }
                document.AddMember("FileInfos", alarmArray, allocator);
                //document.AddMember("FileInfos", alarmArrayFile, allocator);
                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                document.Accept(writer);

                resStr = buffer.GetString();

                //description json结构整理
                rapidjson::Document documentDes;
                documentDes.SetObject();
                rapidjson::Value strValue_1(uuid.c_str(), documentDes.GetAllocator());
                rapidjson::Value intValue(cmdType);

                rapidjson::Value& sourceArray = document["FileInfos"];
                rapidjson::Value targetObject(rapidjson::kObjectType);
                rapidjson::Value targetArray(rapidjson::kArrayType);
                for (size_t i = 0; i < sourceArray.Size(); i++) {
                    targetArray.PushBack(sourceArray[i], documentDes.GetAllocator());
                }
                targetObject.AddMember("data", targetArray, documentDes.GetAllocator());
                documentDes.AddMember("uuid", strValue_1, documentDes.GetAllocator());
                documentDes.AddMember("cmdType", intValue, documentDes.GetAllocator());
                documentDes.AddMember("result", targetObject, documentDes.GetAllocator());

                //判断data下是否为null，若是转换为空数组
                if (documentDes["result"]["data"].IsNull())
                {
                    documentDes["result"]["data"].SetArray();
                }

                rapidjson::StringBuffer desBuffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer_1(desBuffer);
                documentDes.Accept(writer_1);

                description = desBuffer.GetString();

            }
            else
            {

                rapidjson::Document document;
                document.SetObject();
                rapidjson::Value intValue(cmdType);
                rapidjson::Value strValue(uuid.c_str(), document.GetAllocator());
                //解析des中的json串
                rapidjson::Document documentStr;
                documentStr.Parse(description.c_str());
                if (!documentStr.IsArray()) {
                    ERRORLOG("Failed to parse JSON array.description str: {}", description);
                }
                rapidjson::Value targetObject(rapidjson::kObjectType);
                rapidjson::Value newArray(rapidjson::kArrayType);
                newArray.CopyFrom(documentStr, document.GetAllocator());

                targetObject.AddMember("data", newArray, document.GetAllocator());
                document.AddMember("uuid", strValue, document.GetAllocator());
                document.AddMember("cmdType", intValue, document.GetAllocator());
                document.AddMember("result", targetObject, document.GetAllocator());

                //判断data下是否为null，若是转换为空数组
                if (document["result"]["data"].IsNull())
                {
                    document["result"]["data"].SetArray();
                }

                rapidjson::StringBuffer buffer;
                rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
                document.Accept(writer);
                description = buffer.GetString();

                // 如果处理命令是新建目录的ls或空目录ls命令,result创建空json串
                if (cmdType == 8 || cmdType == 3)
                {
                    rapidjson::Document documentRes;
                    documentRes.SetObject();
                    rapidjson::Value nullArray(rapidjson::kArrayType);
                    documentRes.AddMember("FileInfos", nullArray, documentRes.GetAllocator());
                    rapidjson::StringBuffer bufferRes;
                    rapidjson::Writer<rapidjson::StringBuffer> writerRes(bufferRes);
                    documentRes.Accept(writerRes);

                    resStr = bufferRes.GetString();
                }
            }
            res_size = resStr.length();
            des_size = description.length();

            //node_cmd_reply表更新回执
            auto cmdReplySql = new char[4096 + res_size + des_size];
            sprintf(cmdReplySql, "insert into node_cmd_reply(uuid,src_node_id,src_node_name,to_node_id,to_node_name, cmd_reply_uuid,description,\
                result,comm_status_id,cmd_type_id,podman_desc) values ('%s','%s','%s','%s','%s','%s','%s','%s',%d,%d,'%s')", \
                uuid.c_str(), srcNodeId.c_str(), srcNodeName.c_str(), toNodeId.c_str(), toNodeName.c_str(), cmdReplyUuid.c_str(), description.c_str(), resStr.c_str(), \
                status, cmdType, podmanDesc.c_str());

            ret = db->excuteSql(cmdReplySql);
            if (ret < 0)
            {
                ERRORLOG("Node Reply Update Fail:{}", cmdReplySql);
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("Insert cmdReply to DB failed: {}", e.what());
        }


    }
    catch (const std::exception& e) {
        ERRORLOG("Handle reply failed: {}", e.what());
    }
}

void NodeCmdTopic::fill_cmd_message(NodeCmd& cmd_msg, const soci::row& row) {
    //如果查出的数据为空，则赋值""或0
    if (row.get_indicator(0) == soci::i_null)
    {
        cmd_msg.set_uuid("");
    }
    else
    {
        cmd_msg.set_uuid(row.get<std::string>(0));
    }
    if (row.get_indicator(1) == soci::i_null)
    {
        cmd_msg.set_to_node_id("");
    }
    else
    {
        cmd_msg.set_to_node_id(row.get<std::string>(1));
    }
    if (row.get_indicator(3) == soci::i_null)
    {
        cmd_msg.set_src_node_id("");
    }
    else
    {
        cmd_msg.set_src_node_id(row.get<std::string>(3));
    }
    if (row.get_indicator(4) == soci::i_null)
    {
        cmd_msg.set_src_node_name("");
    }
    else
    {
        cmd_msg.set_src_node_name(row.get<std::string>(4));
    }
    if (row.get_indicator(6) == soci::i_null)
    {
        cmd_msg.set_to_group("");
    }
    else
    {
        cmd_msg.set_to_group(row.get<std::string>(6));
    }
    if (row.get_indicator(7) == soci::i_null)
    {
        cmd_msg.set_cmd("");
    }
    else
    {
        cmd_msg.set_cmd(row.get<std::string>(7));
    }
    if (row.get_indicator(8) == soci::i_null)
    {
        cmd_msg.set_paras("");
    }
    else
    {
        cmd_msg.set_paras(row.get<std::string>(8));
    }
    if (row.get_indicator(13) == soci::i_null)
    {
        cmd_msg.set_paras_1("");
    }
    else
    {
        cmd_msg.set_paras_1(row.get<std::string>(13));
    }
    if (row.get_indicator(14) == soci::i_null)
    {
        cmd_msg.set_paras_2("");
    }
    else
    {
        cmd_msg.set_paras_2(row.get<std::string>(14));
    }
    if (row.get_indicator(11) == soci::i_null)
    {
        cmd_msg.set_cmd_type(static_cast<cn::seisys::dds::CmdType>(0));
    }
    else
    {
        cmd_msg.set_cmd_type(static_cast<cn::seisys::dds::CmdType>(row.get<long long>(11)));
    }
    if (row.get_indicator(15) == soci::i_null)
    {
        cmd_msg.set_podman_cmd(static_cast<cn::seisys::dds::PodmanCmd>(0));
    }
    else
    {
        cmd_msg.set_podman_cmd(static_cast<cn::seisys::dds::PodmanCmd>(row.get<long long>(15)));
    }





    //cmd_msg.set_uuid(row.get<std::string>(0));
    //cmd_msg.set_to_node_id(row.get<std::string>(1));
    //cmd_msg.set_src_node_id(row.get<std::string>(3));
    //cmd_msg.set_src_node_name(row.get<std::string>(4));
    //cmd_msg.set_to_group(row.get<std::string>(6));
    //cmd_msg.set_cmd(row.get<std::string>(7));
    //cmd_msg.set_paras(row.get<std::string>(8));
    //cmd_msg.set_paras_1(row.get<std::string>(13));
    //cmd_msg.set_paras_2(row.get<std::string>(14));
    //cmd_msg.set_cmd_type(static_cast<cn::seisys::dds::CmdType>(row.get<long long>(11)));
    //cmd_msg.set_podman_cmd(static_cast<cn::seisys::dds::PodmanCmd>(row.get<long long>(15)));
}

void NodeCmdTopic::update_cmd_status(DbBase* db, long long id, int status) {
    char update_sql[512];
    sprintf(update_sql, "update node_cmd set comm_status_id=%d where id=%lld", status, id);
    if (db->excuteSql(update_sql) < 0) {
        ERRORLOG("Failed to update command status: {}", update_sql);
    }
}

void NodeCmdTopic::process_app_cmd_replies() {
    NodeCmdReply reply;
    if (utility::msg::mainCmdReplyQueueOut_.try_dequeue(reply)) {
        if (!reply.uuid().empty()) {
            send_reply(reply);
        }
    }
}

void NodeCmdTopic::process_file_cmd_replies() {
    NodeCmdReply reply;
    if (utility::msg::fileDataReplyQueueOut_.try_dequeue(reply)) {
        if (!reply.uuid().empty()) {
            send_reply(reply);
        }
    }
}

//int NodeCmdTopic::create_replay_pub() {
//    try {
//        if (!reply_publisher_) {
//            reply_publisher_ = std::make_shared<zenoh::Publisher>(
//                session_->declare_publisher(topic_info_.topicReply_)
//            );
//        }
//        return 0;
//    }
//    catch (const std::exception& e) {
//        ERRORLOG("Failed to create reply publisher: {}", e.what());
//        return -1;
//    }
//}

//int NodeCmdTopic::create_replay_sub() {
//    try {
//        if (topic_info_.needReply_) {
//            zenoh::KeyExpr reply_key(topic_info_.topicReply_);
//
//            auto on_reply = [this](const zenoh::Sample& sample) {
//                handle_reply(sample);
//                };
//
//            auto on_drop_reply = []() {
//                DEBUGLOG("Reply Subscriber dropped");
//                };
//
//            reply_subscriber_.emplace(session_->declare_subscriber(
//                reply_key,
//                std::move(on_reply),
//                std::move(on_drop_reply)
//            ));
//        }
//        return 0;
//    }
//    catch (const std::exception& e) {
//        ERRORLOG("Failed to create reply subscriber: {}", e.what());
//        return -1;
//    }
//}

