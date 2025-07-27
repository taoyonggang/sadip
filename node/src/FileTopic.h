#pragma once

#include "../../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include <concurrentqueue/concurrentqueue.h>
#include "proto/node.pb.h"
#include "proto/file.pb.h"
#include "TopicInfo.h"
#include "soci/soci.h"


//#ifdef USE_S3
#include "../../utils/aws/s3/helper.h"
#include "../../utils/aws/s3/S3Singleton.h"
//#endif

using namespace cn::seisys::dds;
using namespace std;

class FileTopic : public ZenohBase {
public:
    explicit FileTopic(const TopicInfo& topic_info);
    ~FileTopic() override;

    void start_writer(bool wait = false);
    void start_reader(bool wait = false);

private:
    void writer_worker();
    void reader_worker();
    bool send_file_data(const FileData& file_data);
    bool send_reply(const FileDataReply& reply_data);
    void handle_file_data(const zenoh::Sample& sample);
    void handle_reply(const zenoh::Sample& sample);
    bool file_data_reply_in(const FileDataReply& reply_data);

    // 文件处理相关方法
    bool send_file(string fileName, string& srcNodeId, string& uuid, string& toNodeId, string& subDir);
    bool sendOneBlock(FileData& fileData, std::string& uuid, std::string& toNodeId, 
        const std::string& recvfile, std::string& saveFileName, const std::string& sendfile_tmp, 
        size_t& nCount, long long fileSize, const size_t& fileNum, bool& retFlag, 
        const std::string& subDir, bool sendOne = false);
    bool sendOneFile(string fileName, string& srcNodeId, string& uuid, string& toNodeId, 
        const std::string& subDir, int blockNum = 0, int totalNum = 0, int fileTotalSize = 0);

    void move_clear_tmp(const size_t& size, cn::seisys::dds::FileData& fileData, std::string& mergeFileName, std::string& tmpDir, cn::seisys::dds::FileDataReply& fileDataReply, int& retFlag);

    // 压缩相关方法
    bool zipFile(string inputDirPath, string outputFilePath);
    bool zipDir(string inputDirPath, string outputFilePath);
    bool zip(string inputDirPath, string outputFilePath);
    bool uZip(string inputDirPath, string outputFilePath);

    // 文件信息获取
    cn::seisys::dds::FileInfos  getFileInfos(string path);

    // 辅助方法
    void process_node_cmd(const cn::seisys::dds::NodeCmd& cmd);
    void handle_file_transfer_command(const cn::seisys::dds::NodeCmd& cmd, cn::seisys::dds::NodeCmdReply& reply);
    void handle_s3_operations(const cn::seisys::dds::NodeCmd& cmd, cn::seisys::dds::NodeCmdReply& reply);

private:
    TopicInfo topic_info_;
    std::atomic<bool> is_writer_{ false };
    std::atomic<bool> is_reader_{ false };

    std::shared_ptr<zenoh::Publisher> file_publisher_;
    std::shared_ptr<zenoh::Publisher> reply_publisher_;
    std::optional<zenoh::Subscriber<void>> subscriber_;
    std::optional<zenoh::Subscriber<void>> reply_subscriber_;

    std::queue<FileData> file_queue_;
    std::queue<FileDataReply> reply_queue_;
    std::mutex queue_mutex_;
    std::mutex file_mutex_;

    std::unique_ptr<std::thread> writer_thread_;
    std::unique_ptr<std::thread> reader_thread_;

#ifdef USE_S3
    S3Singleton* s3 = nullptr;
#endif

    // Constants
    const string src_files_path = "./files/send/";
    const string to_files_path = "./files/recv/";
    const string src_tmp_path_prefix = "%s_tmp";
    const string to_tmp_path_prefix = "%s_tmp";

    static moodycamel::ConcurrentQueue<cn::seisys::dds::NodeCmd> nodeCmdToFile_;

    // 兼容性保留的监听器
    //class NodePubListener : public eprosima::fastdds::dds::DataWriterListener {
    //public:
    //    NodePubListener() : matched_(0), firstConnected_(false) {}
    //    void on_publication_matched(
    //        eprosima::fastdds::dds::DataWriter* writer,
    //        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;
    //    int matched_;
    //    bool firstConnected_;
    //} nodePubListener_;

    //class NodeReplyPubListener : public eprosima::fastdds::dds::DataWriterListener {
    //public:
    //    NodeReplyPubListener() : matched_(0), firstConnected_(false) {}
    //    void on_publication_matched(
    //        eprosima::fastdds::dds::DataWriter* writer,
    //        const eprosima::fastdds::dds::PublicationMatchedStatus& info) override;
    //    int matched_;
    //    bool firstConnected_;
    //} nodeReplyPubListener_;
};