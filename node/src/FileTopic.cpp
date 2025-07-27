#include "../../utils/pch.h"

#include "FileTopic.h"
#include "../utils/log/BaseLog.h"
#include "../db/DbBase.h"
#include "../utils/FileSplit.h"
#include "../../utils/msgQueue.h"

#include <Poco/FileStream.h>
#include <Poco/Zip/Decompress.h>
#include <Poco/Zip/Compress.h>
#include <Poco/Timestamp.h>
#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>
#include <Poco/Path.h>
#include <Poco/DateTimeFormatter.h>
#include <chrono>
#include <filesystem>
#include "../utils/uuid.h"

#ifdef USE_S3
#include "../../utils/aws/s3/FileWatcher.h"
#endif
#include <boost/chrono/duration.hpp>

using namespace std;
using namespace Poco;
using namespace Poco::Zip;
//namespace fs = std::filesystem;

using namespace cn::seisys::dds;

moodycamel::ConcurrentQueue<cn::seisys::dds::NodeCmd> FileTopic::nodeCmdToFile_;

FileTopic::FileTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
#ifdef USE_S3
    s3 = S3Singleton::getInstance();
#endif
}

FileTopic::~FileTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void FileTopic::start_writer(bool wait) {
    
    if (!writer_thread_) {
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&FileTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }

}

void FileTopic::start_reader(bool wait) {
   
    if (!reader_thread_) {
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&FileTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
    
}
void FileTopic::writer_worker() {
    //auto* topic = (FileTopic*)(arg);
    DbBase* db = DbBase::getDbInstance();
    int ctlCount = 0;
     //创建数据下发发送端，回执数据接收端
    try {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize file writer base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者
        if (!file_publisher_) {
            file_publisher_ = std::make_shared<zenoh::Publisher>(
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
            DEBUGLOG("File reply Subscriber dropped");
            };

        reply_subscriber_.emplace(session_->declare_subscriber(
            reply_key,
            std::move(on_reply),
            std::move(on_drop_reply)
        ));

#ifdef USE_S3
        // S3 目录自启动
        try {
            for (const auto& s3Watch : topic_info_.s3WatchVec_) {
                INFOLOG("Monitor and upload s3 files and self-starting,dir: {}, bucket: {}, "
                    "keep file:{}, zip file: {}, sleep time: {}, watchSubDir:{}",
                    s3Watch.directory, s3Watch.bucket, s3Watch.keepFile,
                    s3Watch.zipFlag, s3Watch.sleep, s3Watch.watchSubDir);

                Poco::File dirStart(s3Watch.directory);
                if (dirStart.exists() && dirStart.isDirectory()) {
                    FileWatcherFactory::createFileWatcher(
                        s3Watch.directory, s3Watch.bucket,
                        topic_info_.nodeId_ + "/up/",
                        s3Watch.sleep, s3Watch.keepFile,
                        s3Watch.zipFlag, s3Watch.watchSubDir
                    );
                }
                else {
                    ERRORLOG("s3 watcher: directory is wrong or does not exist: {}",
                        s3Watch.directory);
                }
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("Start s3 watcher Failed: {}", e.what());
        }
#endif

    }
    catch (const std::exception& e) {
        ERRORLOG("Init file writer failed: {}", e.what());
        return;
    }


    while (is_writer_) {
        try {
            // 处理文件传输命令
            NodeCmd node_cmd;
            if (utility::msg::fileDataQueueIn_.try_dequeue(node_cmd)) {
                process_node_cmd(node_cmd);
            }

            // 管理节点处理数据库中的文件传输任务
            if (utility::node::nodeInfo_.group_ == "mgr" && ctlCount % 20 == 0) {
                char task_sql[1024];
                sprintf(task_sql, 
                    "select `file_name`,`src_node_id`,`to_node_id`,`uuid`,`sub_dir` "
                    "from file_data where file_data_status_id=0 and to_node_id='%s'", 
                    topic_info_.partition_.c_str());

                soci::rowset<soci::row> rows = db->select(task_sql);
                for (const soci::row& row : rows) {
                    string fileName = row.get<string>(0);
                    string srcNodeId = row.get<string>(1);
                    string toNodeId = row.get<string>(2);
                    string uuid = row.get<string>(3);
                    string subDirs = row.get<string>(4, ".");

                    // 更新状态为处理中
                    db->excuteSql(fmt::format(
                        "update file_data set file_data_status_id=1 where uuid='{}'", 
                        uuid));

                    bool success = send_file(fileName, srcNodeId, uuid, toNodeId, subDirs);

                    if (success)
                    {
                        INFOLOG("{} send success,srcNodeId:{},toNodeId:{}", fileName, srcNodeId, toNodeId);
                    }
                    
                    // 更新处理结果
                    int status = success ? 2 : 6;
                    db->excuteSql(fmt::format(
                        "update file_data set file_data_status_id={} where uuid='{}'", 
                        status, uuid));

                    if (!success) {
                        // 记录失败信息
                        db->excuteSql(fmt::format(
                            "insert into file_data_reply(file_name,to_node_id,src_node_id,"
                            "reply_uuid,total_block_num,current_block_num,status_description,"
                            "file_data_status_id)values('{}','{}','{}','{}',{},{},'{}',{})",
                            fileName, srcNodeId, toNodeId, uuid, 0, 0, 
                            fileName + " Read Exception!", 6));
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                }
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("FIle writer worker error: {}", e.what());
        }

        ++ctlCount;
        //if (++ctlCount % 20 == 0) {
        //    DEBUGLOG("FileData: writer running on topic: {}, reply: {}", 
        //        topic_info_.topic_,
        //        topic_info_.topicReply_);
        //}

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void FileTopic::reader_worker() {
    //auto* topic = (FileTopic*)(arg);
    /*int count = 0;*/
    
    //创建数据接收端、回执发送端连接  
    try {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize file reader base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（fileTopic）
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_file_data(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("File Subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));

        //回执发送者创建（fileReplyTopic）
        if (!reply_publisher_) {
            reply_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topicReply_)
            );
        }

    }
    catch (const std::exception& e) {
        ERRORLOG("Init file reader failed: {}", e.what());
        return;
    }


    while (is_reader_) {
        if (topic_info_.needReply_) {
            FileDataReply reply;
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                if (!reply_queue_.empty()) {
                    reply = std::move(reply_queue_.front());
                    reply_queue_.pop();
                }
            }

            if (!reply.uuid().empty()) {
                if (send_reply(reply)) {
                    INFOLOG("FileDataReply sent successfully for file: {}", 
                        reply.file_name());
                }
            }
        }

        //++count;
       /* if (++count % 20 == 0) {
            DEBUGLOG("FileData: reader running on topic: {}, reply: {}", 
                topic_info_.topic_,
                topic_info_.topicReply_);
        }*/

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool FileTopic::send_file_data(const FileData& file_data) {
    try {
        std::vector<uint8_t> serialized_msg(file_data.ByteSize());
        file_data.SerializeToArray(serialized_msg.data(), file_data.ByteSize());
        file_publisher_->put(serialized_msg);

        //构建统计发送端监控数据
        stat_writer_mon_data(file_data.ByteSize(), file_data.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send file data failed: {}", e.what());
        return false;
    }
}

bool FileTopic::send_reply(const FileDataReply& reply_data) {
    try {
        std::vector<uint8_t> serialized_msg(reply_data.ByteSize());
        reply_data.SerializeToArray(serialized_msg.data(), reply_data.ByteSize());
        reply_publisher_->put(serialized_msg);

        //构建回执发送监控数据
        stat_reply_writer_mon_data(reply_data.ByteSize(), reply_data.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send reply failed: {}", e.what());
        return false;
    }
}

void FileTopic::handle_file_data(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        FileData file_data;
        file_data.ParseFromArray(data.data(), data.size());
        

        // 构建数据接收监控数据
        stat_reader_mon_data(topic_info_.topic_, file_data.src_node_id(), file_data.ByteSize());
        
        //接收文件块，并组合文件
        INFOLOG("File: nodeId = {} ,filename = {}", file_data.src_node_id(), file_data.file_name());

        std::string fileName = file_data.file_name();
        std::string tmpDir = fileName + "_tmp";
        std::string tmpFileName = tmpDir + "/" + std::to_string(file_data.block_seq());
        INFOLOG("File: recv file block:{} ,nodeId = {} ,filename = {}", file_data.block_seq(), file_data.src_node_id(), file_data.file_name());
        utility::file::mkdir(tmpDir);
        utility::file::write_file(tmpFileName.c_str(), file_data.block_content().data(), file_data.block_content().size());


        fs::path filePath(fileName);
        filePath.make_preferred();
        std::vector<std::string> fileNames;
        utility::string::split(filePath.string(), fileNames, fs::path::preferred_separator);
        size_t size = fileNames.size();
        FileDataReply fileDataReply;
        fileDataReply.set_block_seq(file_data.block_seq());
        fileDataReply.set_uuid(utility::uuid::generate());
        fileDataReply.set_reply_uuid(file_data.uuid());
        fileDataReply.set_file_name(fileNames[size - 1]);
        fileDataReply.set_total_block_num(file_data.total_block_num());
        fileDataReply.set_src_node_id(file_data.to_node_id());
        fileDataReply.set_is_end(0);
        fileDataReply.set_state(1);
        fileDataReply.set_to_node_id(file_data.src_node_id());;
        fileDataReply.set_file_total_size(file_data.file_total_size());

        //state:0 未知,1,传输中,2,发送端传输完成,3,需要重转指定序列号,4,需要完全重传,5,接收端确认成功

        if (file_data.is_end()) {   //合并文件和发送结束回执
            //尝试合并10次,间隔5秒
            int mergeCount = 0;
            int result = -1;
            while (true && mergeCount < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                result = utility::file::merge(tmpDir, file_data.total_block_num());
                mergeCount++;
                if (result == 0) {
                    std::string mergeFileName = tmpDir + "/merge.out";
                    const size_t size = utility::file::GetFileLength(mergeFileName);
                    int retFlag;
                    move_clear_tmp(size, file_data, mergeFileName, tmpDir, fileDataReply, retFlag);
                    if (retFlag == 2) break;

                }
                else if (result > 0) {
                    //会自动重发,无需处理
                    fileDataReply.set_block_seq(result);
                    fileDataReply.set_is_end(1);
                    fileDataReply.set_state(3);
                    //失败消息重发两次间隔5秒重传回执
                    //fileDataReplyQueue_.enqueue(fileDataReply);
                    INFOLOG("File: {} need retry send block:{}.", file_data.file_name(), result);
                    //std::this_thread::sleep_for(std::chrono::milliseconds(15 * 1000));
                    //if (utility::file::GetFileLength(fileData.saveFileName()) <= 0) {  //防止其他地方合并了文件
                    //	int new_result = utility::file::merge(tmpDir, fileData.totalBlockNum());
                    //	int nRetry = 0;
                    //	if (new_result == result) {
                    //		fileDataReplyQueue_.enqueue(fileDataReply);
                    //		INFOLOG("File: {} need retry send block:{}.", fileData.fileName(), result);
                    //	}
                    //	else {
                    //		fileDataReply.blockSeq(new_result);
                    //		fileDataReplyQueue_.enqueue(fileDataReply);
                    //		INFOLOG("File: {} need retry send block:{}.", fileData.fileName(), new_result);
                    //	}
                    //}
                    //else {  //已经合并成功,需要发送完成标志
                    //	fileDataReply.blockSeq(fileDataReply.totalBlockNum());
                    //	fileDataReply.isEnd(1);
                    //	fileDataReply.state(5);
                    //	fileDataReplyQueue_.enqueue(fileDataReply);
                    //	INFOLOG("File: {} recv commplete.", fileData.fileName());
                    //}
                    //std::this_thread::sleep_for(std::chrono::milliseconds(15 * 1000));
                    break;
                }
                else {
                    ERRORLOG("File: {} send failed.", file_data.file_name());
                    //需要发送传送失败回执
                    fileDataReply.set_is_end(1);
                    fileDataReply.set_state(4);
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    reply_queue_.push(fileDataReply);
                    break;
                }
                //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        else {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            reply_queue_.push(fileDataReply);
        }

        //边缘节点发向管理节点的文件（cmd命令发送文件）直接更新回执表
        if (file_data.src_node_id()!=topic_info_.monNodeId_)
        {
            if (!file_data_reply_in(fileDataReply))
            {
                ERRORLOG("file reply update {} block seq failed", fileDataReply.block_seq());
            }
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle file data failed: {}", e.what());
    }
}

void FileTopic::handle_reply(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        FileDataReply reply;
        reply.ParseFromArray(data.data(), data.size());
        

        // 构建回执接收监控数据,是否需要管理节点入库的条数统计，边缘节点不入库回执不统计？
        stat_reply_reader_mon_data(topic_info_.topicReply_, reply.src_node_id(), reply.ByteSize());

        /*file传输回执数据入库,管理节点才会去入库回执数据*/
        try
        {
            if (utility::node::nodeInfo_.group_.compare("mgr") == 0) {

                if (!file_data_reply_in(reply))
                {
                    ERRORLOG("file reply update {} block seq failed", reply.block_seq());
                }
            }

            std::lock_guard<std::mutex> lock(queue_mutex_);
            reply_queue_.push(reply);
            //this->fileDataReplyQueue_.enqueue(fileDataReply);
            INFOLOG("File: recv FileDataReply, nodeid = {},fileName = {},block:{}", reply.src_node_id(), reply.file_name(), reply.block_seq());
        }
        catch (std::exception& e)
        {
            ERRORLOG("file reply update failed: {}", e.what());
        }

    }
    catch (const std::exception& e) {
        ERRORLOG("Handle reply failed: {}", e.what());
    }
}

void FileTopic::process_node_cmd(const NodeCmd& cmd) {
    NodeCmdReply reply;
    reply.set_uuid(utility::uuid::generate());
    reply.set_cmd_reply_uuid(cmd.uuid());
    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
    reply.set_to_node_id(cmd.src_node_id());
    reply.set_to_node_name(cmd.src_node_name());
    reply.set_src_node_id(topic_info_.nodeId_);
    reply.set_src_node_name(topic_info_.nodeName_);
    reply.set_cmd_type(cmd.cmd_type());

    switch (cmd.cmd_type()) {
        case CmdType::LS:
        case CmdType::MKDIR:
        case CmdType::DEL:
        case CmdType::DELDIR:
        case CmdType::SENDFILE:
        case CmdType::MKFILE:
        case CmdType::CPFILE:
        case CmdType::ZIP:
        case CmdType::DZIP:
            handle_file_transfer_command(cmd, reply);
            break;

#ifdef USE_S3
        case CmdType::GETS3FILE:
        case CmdType::PUTS3FILE:
        case CmdType::LSS3DIR:
        case CmdType::DELS3FILE:
        case CmdType::ADDS3WATCH:
        case CmdType::RMS3WATCH:
        case CmdType::LSS3WATCH:
            handle_s3_operations(cmd, reply);
            break;
#endif

        default:
            ERRORLOG("Unknown command: {} params: {}", cmd.cmd(), cmd.paras());
            break;
    }

    //utility::msg::fileDataReplyQueueOut_.enqueue(reply);
}

// 实现文件传输相关命令处理
void FileTopic::handle_file_transfer_command(const NodeCmd& cmd, NodeCmdReply& reply) {
    // 这里包含原有的文件操作逻辑，包括：
    // LS, MKDIR, DEL, DELDIR, SENDFILE, MKFILE, CPFILE, ZIP, DZIP 等命令的处理
    // 由于代码较长，这里省略具体实现，但保持与原代码相同的功能
    try
    {
        switch (cmd.cmd_type()) {
        case ::CmdType::LS:
            if (cmd.paras().size() > 0) {
                FileInfos fileInfoVec = getFileInfos(cmd.paras());
                for (int i = 0; i < fileInfoVec.files_size(); i++)
                {
                    auto fileInfos = reply.add_result();
                    fileInfos->CopyFrom(fileInfoVec.files(i));
                }
                //reply.set_result(getFileInfos(cmd.paras()));
                reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "get directory sucess: " + cmd.paras());
                INFOLOG("{} gets directory successfully:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::MKDIR:
            if (cmd.paras().size() > 0) {
                utility::file::mkdir(cmd.paras());
                string oriParas = cmd.paras();
                string splitParas = utility::string::cutlaststr(oriParas, "/");

                FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                for (int i = 0; i < fileInfoVec.files_size(); i++)
                {
                    auto fileInfos = reply.add_result();
                    fileInfos->CopyFrom(fileInfoVec.files(i));
                }

                //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "creat directory sucess: " + cmd.paras());
                INFOLOG("{} make directory successfully:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::DEL:
            if (cmd.paras().size() > 0) {
                std::string paras = cmd.paras();
                Poco::File file(utility::string::trim(paras));
                if (file.exists())
                {
                    if (utility::file::del(cmd.paras()))
                    {
                        reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                        string oriParas = cmd.paras();
                        string splitParas = utility::string::cutlaststr(oriParas, "/");

                        FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                        for (int i = 0; i < fileInfoVec.files_size(); i++)
                        {
                            auto fileInfos = reply.add_result();
                            fileInfos->CopyFrom(fileInfoVec.files(i));
                        }
                        //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "delete file sucess: " + cmd.paras());
                        INFOLOG("{} delete file successfully:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                    else {
                        INFOLOG("{} delete file failed:{} paras:{} maybe a dictory", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                        ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
                    }
                }
                else
                {
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "deleted file is not exist,invalid parameter");
                    ERRORLOG("deleted file is not exist or invalid parameter,unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::DELDIR:
            if (cmd.paras().size() > 0) {
                Poco::File dir(cmd.paras());
                if (dir.exists() && dir.isDirectory())
                {
                    utility::file::remove_all_files(cmd.paras());
                    string oriParas = cmd.paras();
                    string splitParas = utility::string::cutlaststr(oriParas, "/");

                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }
                    //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "delete directory sucess: " + cmd.paras());
                    INFOLOG("{} delete directory successfully:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else
                {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "deleted dir is not exist,invalid parameter");
                    ERRORLOG("deleted dir is not exist or invalid parameter,unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::SENDFILE:
            //处理发送文件
            if (cmd.paras().size() > 0) {
                std:string srcNodeId = cmd.src_node_id();
                std::string uuid = cmd.uuid();
                std::string toNodeId = cmd.to_node_id();
                std::string subDir = cmd.paras();
                if (send_file(cmd.paras(), toNodeId, uuid, srcNodeId, subDir)) {
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    string oriParas = cmd.paras();
                    string splitParas = utility::string::cutlaststr(oriParas, "/");

                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }
                    //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "send file sucess: " + cmd.paras());
                    INFOLOG("{} send file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "send file fail: " + cmd.paras());
                    ERRORLOG("{} send file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::MKFILE:
            //create文件
            if (cmd.paras().size() > 0) {
                std::string path = cmd.paras();
                Poco::File file(utility::string::trim(path));
                if (!file.exists()) {
                    bool result = file.createFile();
                    if (result) {
                        reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                        string oriParas = cmd.paras();
                        string splitParas = utility::string::cutlaststr(oriParas, "/");

                        FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                        for (int i = 0; i < fileInfoVec.files_size(); i++)
                        {
                            auto fileInfos = reply.add_result();
                            fileInfos->CopyFrom(fileInfoVec.files(i));
                        }
                        //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "mk file sucess: " + cmd.paras());
                        INFOLOG("{} make file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                    else {
                        reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "mk file fail: " + cmd.paras());
                        ERRORLOG("{} make file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }

                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "make file fail,file exist: " + cmd.paras());
                    ERRORLOG("{} make file failed,file exist,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::CPFILE:
            //copy文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                std::string path = cmd.cmd();
                Poco::File file(utility::string::trim(path));
                if (file.exists()) {
                    Poco::File copy(cmd.paras());
                    file.copyTo(copy.path());
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    string oriParas = cmd.paras();
                    string splitParas = utility::string::cutlaststr(oriParas, "/");

                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }
                    //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "copy file sucess: " + cmd.paras());
                    INFOLOG("{} send file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "copy file fail: " + cmd.paras());
                    ERRORLOG("{} send file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::ZIP:
            //压缩文件
            if (cmd.paras().size() > 0) {
                //判定输入压缩的文件或目录是否存在
                std::string fileName = cmd.cmd();
                string zip_file_name = utility::string::trim(fileName);
                char last_char = zip_file_name.back();
                bool dir_flag = false;
                bool file_file = false;
                if (last_char == '/')
                {
                    Poco::File dir(cmd.cmd());
                    if (dir.exists() && dir.isDirectory())
                    {
                        dir_flag = true;
                    }
                }
                else
                {
                    Poco::File file(cmd.cmd());
                    if (file.exists())
                    {
                        file_file = true;
                    }
                }
                if (dir_flag || file_file)
                {
                    if (zip(cmd.cmd(), cmd.paras())) {
                        reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                        //string oriParas = "./"+nodeCmd.cmd();
                        string oriParas = "./" + cmd.cmd();
                        size_t pos1 = oriParas.find_last_of('/');
                        string splitParas1 = oriParas.substr(0, pos1);

                        if (oriParas[oriParas.length() - 1] == '/')
                        {
                            splitParas1 = splitParas1.substr(0, splitParas1.find_last_of('/'));

                        }
                        //string splitParas = utility::string::cutlaststr(oriParas, "/");
                        //string splitParas = utility::string::cutFisrtLeftStr(oriParas, "/");
                        //reply.result(fileTopic->getFileInfos(splitParas.c_str()));

                        FileInfos fileInfoVec = getFileInfos(splitParas1.c_str());
                        for (int i = 0; i < fileInfoVec.files_size(); i++)
                        {
                            auto fileInfos = reply.add_result();
                            fileInfos->CopyFrom(fileInfoVec.files(i));
                        }
                        //reply.result(fileTopic->getFileInfos(splitParas1.c_str()));
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "zip file sucess: " + cmd.paras());
                        INFOLOG("{} zip file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                    else {
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "zip file fail: " + cmd.paras());
                        ERRORLOG("{} zip file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + " zipped file or dir is not exist,invalid parameter");
                    ERRORLOG("zipped file or dir is not exist or invalid parameter,unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::DZIP:
            //解压文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Poco::File file(cmd.cmd());
                if (file.exists())
                {
                    if (uZip(cmd.cmd(), cmd.paras())) {
                        reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                        //string oriParas = nodeCmd.cmd();
                        string oriParas = "./" + cmd.cmd();
                        size_t pos1 = oriParas.find_last_of('/');
                        string splitParas = oriParas.substr(0, pos1);
                        //string splitParas = utility::string::cutlaststr(oriParas, "/");

                        FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                        for (int i = 0; i < fileInfoVec.files_size(); i++)
                        {
                            auto fileInfos = reply.add_result();
                            fileInfos->CopyFrom(fileInfoVec.files(i));
                        }
                        //reply.result(fileTopic->getFileInfos(splitParas.c_str()));
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "uzip file sucess: " + cmd.paras());
                        INFOLOG("{} unzip file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                    else {
                        reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "uzip file fail: " + cmd.paras());
                        ERRORLOG("{} unzip file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                    }
                }
                else
                {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + " dzipped file is not exist,invalid parameter");
                    ERRORLOG("unzipped file is not exist or invalid parameter,unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;

        default:
            utility::msg::mainCmdReplyQueueOut_.enqueue(reply);
            ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            break;

        }
    }
    catch (const std::exception& e)
    {
        ERRORLOG("handle file conmand failed:error:{}", e.what());
    }

}

#ifdef USE_S3
// 实现S3相关操作处理
void FileTopic::handle_s3_operations(const NodeCmd& cmd, NodeCmdReply& reply) {
    // 这里包含原有的S3操作逻辑，包括：
    // GETS3FILE, PUTS3FILE, LSS3DIR, DELS3FILE, ADDS3WATCH, RMS3WATCH, LSS3WATCH 等命令的处理
    // 由于代码较长，这里省略具体实现，但保持与原代码相同的功能
    try {
        switch (cmd.cmd_type())
        {
        case ::CmdType::GETS3FILE:
            //获取s3文件,会自动返回存放s3目录列表
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                //Aws::Client::ClientConfiguration& clientConfig = fileTopic->s3->s3_helper_->config;
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKey = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                string paras = cmd.paras();
                string oriParas = utility::string::formatPath(paras);
                string splitParas = utility::string::cutlaststr(oriParas, "/");
                DEBUGLOG("GETS3FILE fileKey:{},saving path:{}", cmd.cmd(), splitParas);
                Poco::File dirDown(splitParas);
                if (!dirDown.exists())
                {
                    utility::file::mkdir(splitParas);
                    INFOLOG("make directory successfully,directory:{}", splitParas);
                }
                //string fileKey = fileKeyStr.substr(1);
                //string fileName = utility::string::cutlastrightstr(nodeCmd.cmd(), "/");
                INFOLOG("start download s3 file,bucket:{} fileKey:{},saving path:{}", bucket, fileKey, cmd.paras());
                if (s3->s3_helper_->downloadfile(bucket, fileKey, cmd.paras())) {
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);

                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }

                    //nodeCmdReply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    INFOLOG("get directory ls command successfully!");
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "download s3 file success: " + cmd.paras());
                    INFOLOG("{} get s3 file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "download s3 file failed: " + cmd.paras());
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    ERRORLOG("{} get s3 file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::PUTS3FILE:
            //上传文件到s3
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKey = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                INFOLOG("start up s3 file,bucket:{} fileKey:{},saving path:{}", bucket, fileKey, cmd.paras());
                if (s3->s3_helper_->uploadfile(bucket, fileKey, cmd.paras())) {
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    std::string paras = cmd.paras();
                    string oriParas = utility::string::formatPath(paras);
                    string splitParas = utility::string::cutlaststr(oriParas, "/");

                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }

                    //nodeCmdReply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "upload s3 file success: " + cmd.paras());
                    INFOLOG("{} up s3 file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "upload s3 file failed: " + cmd.paras());
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    ERRORLOG("{} up s3 file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::LSS3DIR:
            //获取s3目录下文件列表
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                INFOLOG("get bucket file list,bucket:{}", cmd.cmd());
                if (s3->s3_helper_->ListObjects(cmd.cmd(), clientConfig)) {
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    //处理返回的文件列表,并填入desc
                    //nodeCmdReply.result(fileTopic->getFileInfos(splitParas.c_str()));
                   /* FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }*/

                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "upload s3 file success: " + cmd.paras());
                    INFOLOG("{} get bucket file list successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "upload s3 file failed: " + cmd.paras());
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    ERRORLOG("{} get bucket file list failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::DELS3FILE:
            //拷贝s3文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKey = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                INFOLOG("start delete s3 file,bucket:{} fileKey:{},saving path:{}", bucket, fileKey, cmd.paras());
                if (s3->s3_helper_->DeleteObject(fileKey, bucket, clientConfig)) {
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    std::string paras = cmd.paras();
                    string oriParas = utility::string::formatPath(paras);
                    string splitParas = utility::string::cutlaststr(oriParas, "/");
                    //nodeCmdReply.result(fileTopic->getFileInfos(splitParas.c_str()));
                    FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }

                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "copy s3 file success: " + cmd.paras());
                    INFOLOG("{} delete s3 file successfully,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
                else {
                    reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "copy s3 file failed: " + cmd.paras());
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    ERRORLOG("{} delete s3 file failed,cmd:{} paras:{}", topic_info_.nodeName_, cmd.cmd(), cmd.paras());
                }
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::ADDS3WATCH:
            //拷贝s3文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKeyRoot = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                fileKeyRoot = fileKeyRoot.substr(1);
                INFOLOG("monitor and upload s3 file,dir:{}, bucket:{} fileKeyRoot:nodeid/up/+{},keep file:{}", fileKeyRoot, bucket, fileKeyRoot, cmd.paras());
                Poco::File dir(fileKeyRoot);
                if (dir.exists() && dir.isDirectory())
                {
                    bool keepFile = true;
                    bool needZip = true;
                    bool watchSubDir = false;
                    std::vector<std::string> sv;
                    utility::string::split(cmd.paras(), sv, '#');
                    if (sv.size() > 2) {
                        keepFile = sv[0].compare("1") == 0 ? true : false;
                        needZip = sv[1].compare("1") == 0 ? true : false;
                        watchSubDir = sv[2].compare("1") == 0 ? true : false;
                    }
                    FileWatcherFactory::createFileWatcher(fileKeyRoot, bucket, topic_info_.nodeId_ + "/up/", 30, keepFile, needZip, watchSubDir);
                    reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                    //nodeCmdReply.result(FileWatcherFactory::listAllWatchers().c_str());

                  /*  FileInfos fileInfoVec = getFileInfos(splitParas.c_str());
                    for (int i = 0; i < fileInfoVec.files_size(); i++)
                    {
                        auto fileInfos = reply.add_result();
                        fileInfos->CopyFrom(fileInfoVec.files(i));
                    }*/

                    reply.set_desc(FileWatcherFactory::listAllWatchers().c_str());
                }
                else
                {
                    reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                    reply.set_desc("dir not exist or not directory!");
                }

            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::RMS3WATCH:
            //拷贝s3文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKeyRoot = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                fileKeyRoot = fileKeyRoot.substr(1);
                INFOLOG("delete monitor and upload s3 file,dir:{}, bucket:{} fileKeyRoot:nodeid/up/+{},keep file:{}", fileKeyRoot, bucket, fileKeyRoot, cmd.paras());
                INFOLOG("before deleting monitor!");
                FileWatcherFactory::deleteFileWatcherByDirectory(fileKeyRoot);
                INFOLOG("after deleting monitor!");
                reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                //nodeCmdReply.result(FileWatcherFactory::listAllWatchers().c_str());
                reply.set_desc(FileWatcherFactory::listAllWatchers().c_str());
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;
        case ::CmdType::LSS3WATCH:
            //拷贝s3文件
            if (cmd.cmd().size() > 0 && cmd.paras().size() > 0) {
                Aws::Client::ClientConfiguration& clientConfig = s3->s3_helper_->config;
                string bucket = utility::string::cutFisrtLeftStr(cmd.cmd(), "/");
                string fileKeyRoot = utility::string::cutFisrtRightstr(cmd.cmd(), "/");
                INFOLOG("ls monitor and upload s3 file,dir:{}, bucket:{} fileKeyRoot:nodeid/up/+{},keep file:{}", fileKeyRoot, bucket, fileKeyRoot, cmd.paras());
                reply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
                //nodeCmdReply.result(FileWatcherFactory::listAllWatchers().c_str());
                reply.set_desc(FileWatcherFactory::listAllWatchers().c_str());
            }
            else {
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "invalid parameter");
                reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
            }
            utility::msg::fileDataReplyQueueOut_.enqueue(reply);
            break;

            default:
                reply.set_desc(topic_info_.nodeName_ + topic_info_.nodeId_ + "unknown cmd");
                reply.set_status(::cn::seisys::dds::CommStatus::INVALID);
                ERRORLOG("unknown cmd:{} paras:{}", cmd.cmd(), cmd.paras());
        }

    }
    catch (const std::exception& e)
    {
        ERRORLOG("handle s3 file conmand failed:error:{}", e.what());
    }
}
#endif


bool FileTopic::send_file(string fileName, string& srcNodeId, string& uuid, string& toNodeId, string& subDirs) {

    //std::srand(std::time(nullptr)); 
    //int rand = std::rand();
    const string sendfile = utility::string::subreplace(fileName, "./", "");
    const string subFileName = utility::string::subreplace(subDirs, "./", "");
    const string sendfile_tmp = "send/" + ::utility::node::nodeInfo_.nodeId_ + "/" + sendfile + "_tmp/";
    const string recvfile = "recv/" + srcNodeId + "/" + toNodeId + "/" + sendfile;

    long long fileSize = utility::file::GetFileLength(sendfile);
    if (fileSize < 0) {
        ERRORLOG("FileData:fileSize < 0,filePath invalid:{}.", sendfile);
        return false;
    }
    utility::file::mkdir(sendfile_tmp);
    utility::file::split(sendfile, utility::node::nodeInfo_.fileSplitSize_, sendfile_tmp);
    //string curDir(fileTopic->argv_[0]);
    string curDir(".");
    //string saveFileName = ::utility::node::nodeInfo_.nodeId_ + "/" + fileName;
    string saveFileName = ::utility::node::nodeInfo_.nodeId_ + "/" + subFileName;
    curDir = curDir.substr(0, curDir.find_last_of("/"));
    size_t fileNum = fileSize / (utility::node::nodeInfo_.fileSplitSize_ * 1024) + (fileSize % (utility::node::nodeInfo_.fileSplitSize_ * 1024) > 0 ? 1 : 0);
    ////utility::file::merge(sendfile_tmp);
    size_t nCount = 0;

    FileData fileData;
    fileData.set_file_name(sendfile);


    while (nCount < fileNum) {
        bool retFlag;
        bool retVal = sendOneBlock(fileData, uuid, toNodeId, recvfile, saveFileName, sendfile_tmp, nCount, fileSize, fileNum, retFlag, subDirs, false);
        if (retFlag) return retVal;
        nCount++;
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return false;
}


bool FileTopic::sendOneFile(string fileName, string& srcNodeId, string& uuid, string& toNodeId, const std::string& subDir, int blockNum, int totalNum, int fileTotalSize) {

    string sendfile = utility::string::subreplace(fileName, "./", "");
    string sendfile_tmp = sendfile + "_tmp/";
    string recvfile = sendfile;
    string saveFileName = sendfile;


    FileData fileData;
    fileData.set_file_name(sendfile);

    bool retFlag = false;
    size_t nCount = blockNum;
    bool retVal = sendOneBlock(fileData, uuid, toNodeId, recvfile, saveFileName, sendfile_tmp, nCount, fileTotalSize, totalNum, retFlag, subDir, true);

    return retVal;

}

bool FileTopic::sendOneBlock(cn::seisys::dds::FileData& fileData,
    std::string& uuid,
    std::string& toNodeId,
    const std::string& recvfile,
    std::string& saveFileName,
    const std::string& sendfile_tmp,
    size_t& nCount,
    long long fileSize,
    const size_t& fileNum,
    bool& retFlag,
    const std::string& subDir,
    bool sendOne)
{
    std::lock_guard<std::mutex> mylockguard(file_mutex_);
    retFlag = true;

    try {
        // 设置基本信息
        uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count();

        // 设置文件数据
        fileData.set_uuid(uuid);
        fileData.set_src_node_id(this->topic_info_.nodeId_);
        fileData.set_to_node_id(toNodeId);
        fileData.set_file_name(recvfile);
        fileData.set_save_file_name(saveFileName);
        fileData.set_sub_dir(subDir);

        string tmp_fileName = sendfile_tmp + std::to_string(nCount);
        uint32_t size = (uint32_t)utility::file::GetFileLength(tmp_fileName);

        fileData.set_block_content_len(size);
        fileData.set_file_total_size(fileSize);
        fileData.set_block_seq(nCount);
        fileData.set_total_block_num(fileNum);
        fileData.set_is_end(sendOne || nCount == fileNum - 1 ? 1 : 0);
        fileData.set_f_modify_time(now);
        fileData.set_transfer_id(".");
        fileData.set_md5(".");

        if (fileData.block_content_len() > 0) {
            // 读取文件内容
            std::unique_ptr<FILE, decltype(&fclose)> fp(
                fopen(tmp_fileName.c_str(), "rb"),
                &fclose
            );

            if (!fp) {
                ERRORLOG("FileData:failed open file block:{} with name:{}", nCount, tmp_fileName);
                retFlag = false;
                return retFlag;
            }

            // 读取文件内容到文件数据对象
            fileData.mutable_block_content()->resize(fileData.block_content_len());
            std::vector<uint8_t> buffer(size);
            size_t read_size = fread(buffer.data(), 1, size, fp.get());
            memcpy(fileData.mutable_block_content()->data(), buffer.data(), size);

            int retryCount = 0;
            while (retryCount <= utility::node::nodeInfo_.fileMaxReTry_) {
                // 清空之前的回复队列
                {
                    std::lock_guard<std::mutex> lock(queue_mutex_);
                    while (!reply_queue_.empty()) {
                        reply_queue_.pop();
                    }
                }

                // 发送文件数据
                if (!send_file_data(fileData)) {
                    ERRORLOG("Failed to send file block:{}", nCount);
                    retryCount++;
                    continue;
                }

                // 等待回复
                auto timeout = fileData.is_end() ?
                    utility::node::nodeInfo_.fileBlockLastTimeout_ * 1000 :
                    utility::node::nodeInfo_.fileBlockTimeout_ * 1000;

                auto start_time = std::chrono::steady_clock::now();
                bool received_valid_reply = false;

                while (std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now() - start_time).count() < timeout) {

                    FileDataReply reply;
                    bool has_reply = false;
                    {
                        std::lock_guard<std::mutex> lock(queue_mutex_);
                        if (!reply_queue_.empty()) {
                            reply = reply_queue_.front();
                            reply_queue_.pop();
                            has_reply = true;
                        }
                    }

                    if (has_reply && reply.block_seq() == nCount) {
                        INFOLOG("FileData:recv file block:{} count at:{} with name:{}",
                            reply.block_seq(), nCount, tmp_fileName);

                        if (fileData.is_end() == 1) {
                            retFlag = true;
                            return true;
                        }
                        else {
                            retFlag = false;
                            return false;
                        }
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }

                retryCount++;
                ERRORLOG("FileData:send is failed of file block:{} with name:{}, retry...",
                    nCount, tmp_fileName);
            }

            if (retryCount > utility::node::nodeInfo_.fileMaxReTry_) {
                retFlag = true;
                return false;
            }
        }
    }
    catch (const std::exception& e) {
        ERRORLOG("Exception while sending file block: {}", e.what());
        retFlag = false;
        return false;
    }

    return retFlag;
}


bool FileTopic::zipFile(string inputDirPath, string outputFilePath) {

    //Creating ofstream object and name of our zip file is going to be same as argv[1]
    ofstream out(outputFilePath, ios::binary);

    //Creating compress object with seekableOut as true for local file
    Compress c(out, true);

    Poco::File f(inputDirPath);
    if (f.exists())
    {
        Path p(f.path());

        if (f.isDirectory())
        {
            ERRORLOG("{} Not a file. ", inputDirPath);
            return false;
        }
        else if (f.isFile())
        {
            ERRORLOG("File added succesfully as {}", f.path());
            c.addFile(p, p.getFileName());
        }
    }
    //Closing compress object
    c.close();
    out.close();

    return true;
}

bool FileTopic::zipDir(string inputDirPath, string outputFilePath) {

    bool result = false;

    Poco::Path srcdir_path(inputDirPath);
    if (srcdir_path.isDirectory()) {

        srcdir_path.makeDirectory();
        std::ofstream outstream(outputFilePath, std::ios::binary);
        Poco::Zip::Compress compress(outstream, true);
        try {
            compress.addRecursive(srcdir_path, Poco::Zip::ZipCommon::CL_NORMAL);
        }
        catch (...) {
            ERRORLOG("File compress error,src:{},dest:{}", inputDirPath, outputFilePath);
        }
        compress.close();
        outstream.close();

        result = true;
    }

    return result;
}

bool FileTopic::zip(string inputDirPath, string outputFilePath) {
    Poco::Path srcdir_path(inputDirPath);
    if (srcdir_path.isDirectory()) {
        return zipDir(inputDirPath, outputFilePath);
    }
    else {
        return zipFile(inputDirPath, outputFilePath);
    }
}


bool FileTopic::uZip(string inputDirPath, string outputFilePath) {

    bool result = false;

    Poco::File f(inputDirPath);
    if (f.exists()) {
        std::ifstream instream(inputDirPath, std::ios::binary);

        Poco::Zip::Decompress decompress(instream, outputFilePath);
        try {
            decompress.decompressAllFiles();
        }
        catch (...) {
            ERRORLOG("File decompress error,src:{},dest:{}", inputDirPath, outputFilePath);
        }
        instream.close();

        result = true;
    }
    return result;
}


//cn::seisys::dds::FileInfos FileTopic::getFileInfos(string path) {
//    // 获取当前工作路径
//    std::string current_path = Poco::Path::current();
//    INFOLOG("Current working directory:{},filepath:{}", current_path, path);
//
//    FileInfos fileInfos;
//    string ls_paras = path;
//    Poco::File dir(ls_paras);
//    if (dir.exists() && dir.isDirectory())
//    {
//        for (const auto& entry : fs::directory_iterator(path)) {
//            cn::seisys::dds::File* fi = fileInfos.add_files();  // 假设FileInfos中有files repeated字段
//            // 或者使用:
//            // cn::seisys::dds::File fi;
//            // auto* file_entry = fileInfos.add_files();
//
//            fi->set_file_path_name(entry.path().string());
//            fi->set_is_dir(::utility::file::IsDir(entry.path().string().c_str()));
//            fi->set_file_size(-1);
//            fi->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
//                std::chrono::system_clock::now().time_since_epoch()
//            ).count());
//
//            if (!fi->is_dir()) {
//                fi->set_file_size(::utility::file::GetFileLength(fi->file_path_name()));
//            }
//            fi->set_timestamp(::utility::file::GetLastUpdatetime(fi->file_path_name()));
//        }
//    }
//
//    return fileInfos;
//}

FileInfos FileTopic::getFileInfos(string path) {
    // 获取当前工作路径
    std::string current_path = Poco::Path::current();
    INFOLOG("Current working directory:{},filepath:{}", current_path, path);

    FileInfos fileInfos;
    Poco::File dir(path);
    if (dir.exists() && dir.isDirectory()) {
        for (const auto& entry : std::filesystem::directory_iterator(path)) {
            try {
                std::string file_path = entry.path().string();
                if (std::filesystem::is_regular_file(entry.status()) && std::filesystem::file_size(entry) == 0) {
                    // 跳过大小为0的文件
                    continue;
                }

                cn::seisys::dds::File* fi = fileInfos.add_files();  // 假设FileInfos中有files repeated字段
                fi->set_file_path_name(file_path);
                fi->set_is_dir(std::filesystem::is_directory(entry.status()));
                fi->set_file_size(-1);
                fi->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count());

                if (!fi->is_dir()) {
                    int64_t file_size = ::utility::file::GetFileLength(fi->file_path_name());
                    if (file_size <= 0) {
                        // 跳过无法获取大小的文件
                        continue;
                    }
                    fi->set_file_size(file_size);
                }
                fi->set_timestamp(::utility::file::GetLastUpdatetime(fi->file_path_name()));
            }
            catch (const std::exception& e) {
                // 捕获并记录异常，然后继续处理下一个文件
                ERRORLOG("Error processing file {}: {}", entry.path().string(), e.what());
                continue;
            }
        }
    }
    else {
        ERRORLOG("Path {} does not exist or is not a directory", path);
    }

    return fileInfos;
}

void FileTopic::move_clear_tmp(const size_t& size, cn::seisys::dds::FileData& fileData, std::string& mergeFileName, std::string& tmpDir, cn::seisys::dds::FileDataReply& fileDataReply, int& retFlag)
{
    retFlag = 1;
    if (size == fileData.file_total_size()) {
        INFOLOG("File: {} recv success.", fileData.file_name());
        INFOLOG("saveFileSize: {}", fileData.save_file_name().length());
        if (std::string(fileData.save_file_name()).length() > 0) {  //需要移动文件
            INFOLOG("File: move file: {} to: {}", mergeFileName, fileData.save_file_name());
            std::string path = ".";
            std::string pathName = fileData.sub_dir().c_str();
            std::vector<std::string> paths;
            utility::string::split(fileData.save_file_name(), paths, '/');
            if (paths.size() > 0) {
                if (paths.size() == 1) {  //如果只有一个,说明当前目录
                    pathName = paths[0];
                }
                else {  //有下级路径,需要创建路径
                    for (int i = 1; i < paths.size() - 1; i++) {
                        //pathName = pathName + "/" + paths[i];
                        /*if (i < paths.size()) {
                            path = path + "/";
                        }*/
                        path = path + "/" + paths[i];
                    }
                    utility::file::mkdir(path);
                }
            }
            utility::file::move(mergeFileName, pathName);
            utility::file::remove_all_files(tmpDir);
        }
        //需要发送传送成功回执
        fileDataReply.set_is_end(1);
        fileDataReply.set_state(5);
        std::lock_guard<std::mutex> lock(queue_mutex_);
        reply_queue_.push(fileDataReply);
        { retFlag = 2; return; };
    }
}

bool FileTopic::file_data_reply_in(const FileDataReply& reply_data)
{
    DbBase* db = DbBase::getDbInstance();

    char fileReplySql[1024];
    int ret = 1;
    string fileName = reply_data.file_name();
    string toNodeId = reply_data.to_node_id();
    string srcNodeId = reply_data.src_node_id();
    string subDir = reply_data.sub_dir();
    string uuid = reply_data.uuid();
    string replyUuid = reply_data.reply_uuid();
    bool isEnd = reply_data.is_end();
    uint64_t curBlockSeq = reply_data.block_seq() + 1;
    uint64_t totalBlockNum = reply_data.total_block_num();
    uint64_t commStatusId = reply_data.state();
    string statusDescription = "";
    char updataFileSats[1024];
    //state:0 未知,1,传输中,2,发送端传输完成,3,需要重转指定序列号,4,需要完全重传,5,接收端确认成功
    if (commStatusId == 1) {
        statusDescription = fileName + " " + to_string(curBlockSeq) + " Send ...";
        //更新fileData表的状态
        if (curBlockSeq == 1)
        {
            sprintf(updataFileSats, "insert into file_data_reply (`reply_uuid`,`file_name`,`to_node_id`,`src_node_id`,`sub_dir`,`current_block_num`,`total_block_num`,`status_description`,`file_data_status_id`) values('%s','%s','%s','%s','%s',%llu,%llu,'%s',%llu)", replyUuid.c_str(), fileName.c_str(), toNodeId.c_str(), srcNodeId.c_str(), subDir.c_str(), \
                curBlockSeq, totalBlockNum, statusDescription.c_str(), commStatusId);
        }
        else
        {
            sprintf(updataFileSats, "update file_data_reply set current_block_num=%llu,file_data_status_id=%llu  where reply_uuid='%s' and src_node_id='%s'", curBlockSeq, commStatusId, replyUuid.c_str(), srcNodeId.c_str());
        }
        if (db != NULL) {
            ret = db->excuteSql(updataFileSats);
            if (ret < 0)
            {
                ERRORLOG("File Update Fail:{}", updataFileSats);
                return false;
            }
        }

    }
    else if (commStatusId == 3)  //缺文件块
    {

        sprintf(updataFileSats, "update file_data_reply set current_block_num=%llu,file_data_status_id=%llu  where reply_uuid='%s' and src_node_id='%s'", curBlockSeq, commStatusId, replyUuid.c_str(), srcNodeId.c_str());
        if (db != NULL) {
            ret = db->excuteSql(updataFileSats);
            if (ret < 0)
            {
                ERRORLOG("File Update retry transfer:{}", updataFileSats);
                return false;
            }
        }
        //if (fileDataReply.isEnd()) {  //传输已经结束,但缺失,需要传输指定文件
        //    this->fileTopic_->sendOneFile(fileDataReply.fileName(), fileDataReply.toNodeId(), fileDataReply.replyUuid(), fileDataReply.srcNodeId(), fileDataReply.blockSeq(), fileDataReply.totalBlockNum(), fileDataReply.fileTotalSize());
        //}

    }
    else if (commStatusId == 4)  //需要完全重传
    {
        //需要修改数据任务状态,重新传输
        statusDescription = fileName + "Need Complete Retransmission";
        sprintf(updataFileSats, "update file_data_reply set progress='%s',comm_status_id=4  where reply_uuid='%s' and src_node_id='%s'", to_string(curBlockSeq).c_str(), replyUuid.c_str(), srcNodeId.c_str());
        if (db != NULL) {
            ret = db->excuteSql(updataFileSats);
            if (ret < 0)
            {
                ERRORLOG("Node Reply Update Fail:{}", fileReplySql);
                return false;
            }
        }

    }
    else if (commStatusId == 5) {
        char insertSql[1024];
        statusDescription = fileName + " Send sucess!";
        if (curBlockSeq == 1)
        {
            sprintf(insertSql, "insert into file_data_reply (`reply_uuid`,`file_name`,`to_node_id`,`src_node_id`,`sub_dir`,`current_block_num`,`total_block_num`,`status_description`,`file_data_status_id`) values('%s','%s','%s','%s','%s',%llu,%llu,'%s',%llu)", replyUuid.c_str(), fileName.c_str(), toNodeId.c_str(), srcNodeId.c_str(), subDir.c_str(), \
                curBlockSeq, totalBlockNum, statusDescription.c_str(), commStatusId);
            if (db != NULL) {
                ret = db->excuteSql(insertSql);
                if (ret < 0)
                {
                    ERRORLOG("File start to failed insert db:{}", insertSql);
                    return false;
                }
            }
        }
        sprintf(updataFileSats, "update file_data_reply set current_block_num=%llu, status_description='%s',file_data_status_id=5  where reply_uuid='%s' and src_node_id='%s'", curBlockSeq, statusDescription.c_str(), replyUuid.c_str(), srcNodeId.c_str());
        if (db != NULL) {
            ret = db->excuteSql(updataFileSats);
            if (ret < 0)
            {
                ERRORLOG("File completed to failed update db:{}", updataFileSats);
                return false;
            }
        }
    }
    return true;
}

