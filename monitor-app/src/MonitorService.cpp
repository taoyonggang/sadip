#include "../../utils/pch.h"
#include <iostream>
#include <set>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <boost/asio.hpp>

#include "../../db/DbBase.h"
#include "../../utils/ini/INIReader.h"
#include "../../utils/split.h"
#include"../../utils/parseJson.h"
#include "MonitorTopic.h"
#include "../../utils/msgQueue.h"
#include "../../utils/log/BaseLog.h"
#include "../../utils/config/ConfigMap.h"
#include "../../utils/common.h"
#include "../../utils/FileSplit.h"
#include "../../dds/zenoh_topic_manager.h"






class SingleInstance {
private:
    fs::path lockfile;
#ifdef _WIN32
    HANDLE fileHandle;
#else
    int fileDescriptor;
#endif

public:
    SingleInstance(const std::string& filename) : lockfile(filename) {
#ifdef _WIN32
        fileHandle = INVALID_HANDLE_VALUE;
#else
        fileDescriptor = -1;
#endif
    }

    bool isAnotherInstanceRunning() {
#ifdef _WIN32
        fileHandle = CreateFileW(
            lockfile.wstring().c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,  // Prevent other processes from opening the file
            NULL,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (fileHandle == INVALID_HANDLE_VALUE) {
            return true;  // Another instance is running
        }
#else
        fileDescriptor = open(lockfile.c_str(), O_RDWR | O_CREAT, 0644);
        if (fileDescriptor == -1) {
            std::cerr << "Unable to open lockfile" << std::endl;
            return true;
        }

        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fl.l_len = 0;

        if (fcntl(fileDescriptor, F_SETLK, &fl) == -1) {
            close(fileDescriptor);
            return true;  // Another instance is running
        }
#endif
        return false;  // This is the first instance
    }

    ~SingleInstance() {
#ifdef _WIN32
        if (fileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(fileHandle);
        }
#else
        if (fileDescriptor != -1) {
            close(fileDescriptor);
        }
#endif
        fs::remove(lockfile);
    }
};


DbBase* DbBase::dbInstance_ = NULL;

NodeInfo utility::node::nodeInfo_;


int main(int argc, char** argv) {


    ////防止程序多次启动
    //SingleInstance instance("./node.lock");

    //if (instance.isAnotherInstanceRunning()) {
    //    std::cout << "Another instance is already running." << std::endl;
    //    return 1;
    //}

    INIReader nodeConfig(argv[2]);

    if (nodeConfig.ParseError() < 0) {
        std::cout << "Can't load ini:" << argv[2] << endl;
        return -1;
    }

    spdlog::level::level_enum mode = spdlog::level::info;

    int logMode = nodeConfig.GetInteger("base", "log_level", 2);
    mode = (spdlog::level::level_enum)logMode;

    INITLOG("logs/monitor-app.log", "monitor-sink",mode);
    INFOLOG("Start Monitor App");

    //MapWrapper confMap;

    int groupCount = 0;
    int domain = 0;
    string name;
    string nodeId;
    string group;
    string location;
    string version;
    string monitorTopic;
    string monTopicIdStr; //mon监控消息topic中节点id组（一般只有管理节点一个）



    string groupPath = nodeConfig.GetString("group", "group_path", "");
    if (groupPath == "")
    {
        ERRORLOG("Don't get group path,progress exit");
        return 0;
    }
    std::map<std::string, std::vector<std::string>>groupMap;
    //根据group.json文件解析组信息
    groupMap = parseJsonToMap(groupPath);

    domain = nodeConfig.GetInteger("base", "domain", 10);
    name = nodeConfig.GetString("base", "name", "");
    nodeId = nodeConfig.GetString("base", "node_id", "");
    group = nodeConfig.GetString("base", "group", "");
    location = nodeConfig.GetString("base", "location", "");
    version = nodeConfig.GetString("base", "version", "");

    //消息监控统计topic字段值
    string monProject = nodeConfig.GetString("route", "mon_project", "platform");
    string monBussDomain = nodeConfig.GetString("route", "mon_buss_domain", "mon");
    string monDomain = nodeConfig.GetString("route", "mon_domain", "cloud");
    string monGroup = nodeConfig.GetString("route", "mon_group", "mgr_x86_linux");
    string monBuss = nodeConfig.GetString("route", "mon_buss", "monitor");
    string monFunc = nodeConfig.GetString("route", "mon_func", "mon");

    string zenohConf = nodeConfig.GetString("route", "databus_conf", "config/databus.json5");


    //获取监控统计topic中节点id字符串,并转换为数组
    std::vector<std::string> monGroupName; //fileUp消息的所有组名数组
    std::vector<std::string> monIdsVec;//fileUp消息的所有组名对应的节点id数组
    utility::string::split(monGroup, monGroupName, ',');
    getGroupVec(groupMap, monGroupName, monIdsVec);
    INFOLOG("monIdsVec size:{}", monIdsVec.size());

    ZenohTopicManager manager;

    auto monStatsTopic = manager.createTopic(
        ZenohTopicManager::Domain::TIMES,
        monProject,               //项目
        monBussDomain,             // 业务域
        monDomain,               // 区域
        monIdsVec[0],              // 节点ID
        monBuss,            // 模块
        monFunc,               // 功能
        ZenohTopicManager::Direction::UP // 方向
    );

    manager.addTopic(monStatsTopic);

    std::cout << "monStatsTopic:" << manager.buildKeyExpr(monStatsTopic) << std::endl;

    utility::node::nodeInfo_.domain_ = domain;
    utility::node::nodeInfo_.group_ = group;
    utility::node::nodeInfo_.location_ = location;
    utility::node::nodeInfo_.nodeId_ = nodeId;
    utility::node::nodeInfo_.nodeName_ = name;
    utility::node::nodeInfo_.version_ = version;

   

    int dbType = nodeConfig.GetInteger("db", "type", 1);
    string dbIp = nodeConfig.GetString("db", "ip", "10.4.1.151");
    string dbUser = nodeConfig.GetString("db", "user", "root");
    string dbPwd = nodeConfig.GetString("db", "pwd", "Seisys*77889900");
    string dbName = nodeConfig.GetString("db", "name", "databus-dev");
    int dbPort = nodeConfig.GetInteger("db", "port", 4000);
    int dbPoolNum = nodeConfig.GetInteger("db", "pool_num", 30);
    DbBase* db = DbBase::getDbInstance(dbType, dbIp, dbUser, dbPwd, dbName, dbPort, dbPoolNum);
    INFOLOG("db_ip:{} db_name:{} db_port:{} db_pool_num:{}", dbIp, dbName, dbPort, dbPoolNum);


    //节点只负责接收数据,不需要回执,
    MonitorTopic* monitor = NULL;
    TopicInfo recvMonitor(utility::node::nodeInfo_);
    recvMonitor.nodeName_ = name;
    recvMonitor.nodeId_ = nodeId;
    recvMonitor.domain_ = domain;
    recvMonitor.topic_ = manager.buildKeyExpr(monStatsTopic);
    recvMonitor.configPath_ = zenohConf;

    monitor = new MonitorTopic(recvMonitor);
    monitor->start_reader(false);
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

            //INFOLOG("Start Syncdb App is running ...");
    }
        return 0;
}

