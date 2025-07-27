#include "../../utils/pch.h"

#include "../../utils/aws/s3/S3Singleton.h"

#include <boost/asio.hpp>

#include <cppmicroservices/Bundle.h>
#include <cppmicroservices/BundleContext.h>
#include <cppmicroservices/Framework.h>
#include <cppmicroservices/FrameworkEvent.h>
#include <cppmicroservices/FrameworkFactory.h>

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <map>
#include <stdexcept>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>
#include <list>


#include "../../utils/ini/INIReader.h"
#include "../../utils/split.h"
#include"../../utils/parseJson.h"
#include "../db/DbBase.h"
#include "NodeTopic.h"
#include "NodeCmdTopic.h"
#include "NodeCmdWorker.h"
#include "PingTopic.h"
#include "FileTopic.h"
#include "MachineStateTopic.h"
//#include "../../utils/config/ConfigMap.h"
#include "../../utils/msgQueue.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#endif

#include "../../utils/FileSplit.h"
#include "../../utils/aws/s3/FileWatcher.h"
#include "../../dds/zenoh_topic_manager.h"

DbBase* DbBase::dbInstance_ = NULL;
std::vector<FileWatcher*> FileWatcherFactory::fileWatchers;
std::mutex FileWatcherFactory::mtx;

//namespace fs = std::filesystem;

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

int main(int argc, char** argv) {
	
	//防止程序多次启动
	SingleInstance instance("./node.lock");

	if (instance.isAnotherInstanceRunning()) {
		std::cout << "Another instance is already running." << std::endl;
		return 1;
	}


	//MapWrapper confMap;
	std::unordered_map<std::string, long> symbolicNameToId;
	///***********************************************************************************
   ///* 添加node业务处理调度模块
	//直到的确有需要数据库的地方打开此配置,边缘节点可能不需要数据库,所以无需打开
	//DbBase* db = DbBase::getDbInstance(1, "10.4.1.151", "root", "Seisys@77889900", "databus-dev", 4000);
	//首先加载节点本身基础配置
	std::string configPathName = "config/node1.ini";
    if (argc > 1) {
		std::cout << "Usage: ./node -c " << argv[2] << std::endl;
		configPathName = argv[2];
	}

	INIReader nodeConfig(configPathName);

	if (nodeConfig.ParseError() < 0) {
		std::cout<<"Can't load ini:"<< argv[2]<<endl;
		return -1;
	}


	spdlog::level::level_enum mode = spdlog::level::info;

	
	int logMode = nodeConfig.GetInteger("base", "log_level", 2);
	mode = (spdlog::level::level_enum)logMode;

	INITLOG("logs/node.log", "node-sink", mode);
	INFOLOG("Start framework");

	int groupCount = 0;
	int domain = 0;
	string name;
	string nodeId;
	string group;
	string location;
	string version;
	string osType;
	string archType;
	int maxRetry;
	int blockTimeout;
	int blockLastTime;
	int blockSize;
	string endpointUrl;
	string accessKey;
	string secretKey;



	string groupPath= nodeConfig.GetString("group", "group_path", "");
	if (groupPath == "")
	{
		ERRORLOG("Don't get group path,progress exit");
		return 0;
	}
	std::map<std::string, std::vector<std::string>>groupMap;
	//根据group.json文件解析组信息
	groupMap = parseJsonToMap(groupPath);
	//打印每个可以下对应vector大小
	 // 遍历 map 并打印每个键对应的 vector 大小
	for (const auto& [key, value] : groupMap) {
		INFOLOG("Group Name: {}- Vector Size:{}", key, value.size());
		/*std::cout << "Group Name: " << key << " - Vector Size: " << value.size() << std::endl;*/
	}


	maxRetry = nodeConfig.GetInteger("route", "file_max_retry", 100);
	blockSize = nodeConfig.GetInteger("route", "file_block_size", 62);
	blockTimeout= nodeConfig.GetInteger("route", "file_block_timeout", 5000);
	blockLastTime= nodeConfig.GetInteger("route", "file_block_last_timeout", 30000);
	domain = nodeConfig.GetInteger("base", "domain", 10);
	name = nodeConfig.GetString("base", "name", "");
	nodeId = nodeConfig.GetString("base", "node_id", "");
	group = nodeConfig.GetString("base", "group", "");
	location = nodeConfig.GetString("base", "location", "");
	version = nodeConfig.GetString("base", "version", "");
	osType = nodeConfig.GetString("base", "os_type", "");
	archType = nodeConfig.GetString("base", "arch_type", "");
	endpointUrl = nodeConfig.GetString("s3", "endpoint_url", "10.4.1.241:31239");
	accessKey = nodeConfig.GetString("s3", "access_key", "xNTUsotTZMMepl3N");
	secretKey = nodeConfig.GetString("s3", "secret_key", "m4rS20T2RRbuZ1DVb45N1ahWv0Smi89e");
	
	string progarmName= nodeConfig.GetString("base", "program_name", "node");
	string monNodeId = nodeConfig.GetString("monitor", "node_id", "000000000000");
	int monSendSleep = nodeConfig.GetInteger("monitor", "send_sleep", 60);

	string zenohConf = nodeConfig.GetString("route", "databus_conf", "config/databus.json5");

	string nodeMsgFuncDomain = nodeConfig.GetString("route", "node_msg_func_domain", "times");
	string nodeMsgProject = nodeConfig.GetString("route","node_msg_project","platform");
	string nodeMsgBussDomain= nodeConfig.GetString("route", "node_msg_buss_domain", "mgr");
	string nodeMsgDomain=nodeConfig.GetString("route", "node_msg_domain", "cloud");
	string nodeMsgGroup=nodeConfig.GetString("route", "node_msg_group", "");
	string nodeMsgBuss=nodeConfig.GetString("route", "node_msg_buss", "node");
	string nodeMsgFunc=nodeConfig.GetString("route", "node_msg_func", "mgmt");
	string nodeMsgDirec = nodeConfig.GetString("route", "node_msg_direc", "down");

	string nodePingFuncDomain = nodeConfig.GetString("route", "node_ping_func_domain", "times");
	string nodePingProject = nodeConfig.GetString("route", "node_ping_project", "platform");
	string nodePingBussDomain = nodeConfig.GetString("route", "node_ping_buss_domain", "mgr");
	string nodePingDomain = nodeConfig.GetString("route", "node_ping_domain", "cloud");
	string nodePingGroup = nodeConfig.GetString("route", "node_ping_group", "");
	string nodePingBuss = nodeConfig.GetString("route", "node_ping_buss", "nodePing");
	string nodePingFunc = nodeConfig.GetString("route", "node_ping_func", "ping");
	string nodePingDirec = nodeConfig.GetString("route", "node_ping_direc", "up");

	string nodeMstateProject = nodeConfig.GetString("route", "node_mstate_project", "platform");
	string nodeMstateBussDomain = nodeConfig.GetString("route", "node_mstate_buss_domain", "mgr");
	string nodeMstateDomain = nodeConfig.GetString("route", "node_mstate_domain", "cloud");
	string nodeMstateGroup = nodeConfig.GetString("route", "node_mstate_group", "");
	string nodeMstateBuss = nodeConfig.GetString("route", "node_mstate_buss", "nodeMachState");
	string nodeMstateFunc = nodeConfig.GetString("route", "node_mstate_func", "machstate");

	string nodeCmdProject = nodeConfig.GetString("route", "node_cmd_project", "platform");
	string nodeCmdBussDomain = nodeConfig.GetString("route", "node_cmd_buss_domain", "mgr");
	string nodeCmdDomain = nodeConfig.GetString("route", "node_cmd_domain", "cloud");
	string nodeCmdGroup = nodeConfig.GetString("route", "node_cmd_group", "");
	string nodeCmdBuss = nodeConfig.GetString("route", "node_cmd_buss", "nodeCmd");
	string nodeCmdFunc = nodeConfig.GetString("route", "node_cmd_func", "cmd");

	string fileDownProject = nodeConfig.GetString("route", "file_down_project", "platform");
	string fileDownBussDomain = nodeConfig.GetString("route", "file_down_buss_domain", "mgr");
	string fileDownDomain = nodeConfig.GetString("route", "file_down_domain", "cloud");
	string fileDownGroup = nodeConfig.GetString("route", "file_down_group", "");
	string fileDownBuss = nodeConfig.GetString("route", "file_down_buss", "file");
	string fileDownFunc = nodeConfig.GetString("route", "file_down_func", "down");

	string fileUpProject = nodeConfig.GetString("route", "file_up_project", "platform");
	string fileUpBussDomain = nodeConfig.GetString("route", "file_up_buss_domain", "mgr");
	string fileUpDomain = nodeConfig.GetString("route", "file_up_domain", "cloud");
	string fileUpGroup = nodeConfig.GetString("route", "file_up_group", "");
	string fileUpBuss = nodeConfig.GetString("route", "file_up_buss", "file");
	string fileUpFunc = nodeConfig.GetString("route", "file_ip_func", "up");

	//消息监控统计topic字段值
	//string monFuncDomain=nodeConfig.GetString("route", "mon_func_domain", "times");
	string monProject = nodeConfig.GetString("route", "mon_project", "platform");
	string monBussDomain = nodeConfig.GetString("route", "mon_buss_domain", "mon");
	string monDomain = nodeConfig.GetString("route", "mon_domain", "cloud");
	string monGroup = nodeConfig.GetString("route", "mon_group", "mgr_x86_linux");
	string monBuss = nodeConfig.GetString("route", "mon_buss", "monitor");
	string monFunc = nodeConfig.GetString("route", "mon_func", "mon");
	//string monDirec=nodeConfig.GetString("route", "mon_direc", "up");


	//获取nodeMsg发送、接收topic中节点id字符串,并转换为数组（节点id组一般只有管理节点一个）
	std::vector<std::string> nodeMsgGroupName; //nodeMsg消息的所有组名数组
	std::vector<std::string> nodeMsgNodeIdsVec;//nodeMsg消息的所有组名对应的节点id数组
	utility::string::split(nodeMsgGroup, nodeMsgGroupName, ',');
	getGroupVec(groupMap, nodeMsgGroupName, nodeMsgNodeIdsVec);
	INFOLOG("NodeMsgGtoupId size:{}", nodeMsgNodeIdsVec.size());

	//// 获取nodePing接收topic中节点id
	std::vector<std::string> nodePingGroupName; //nodePing消息的所有组名数组
	std::vector<std::string> nodePingNodeIdsVec;//nodePing消息的所有组名对应的节点id数组
	utility::string::split(nodePingGroup, nodePingGroupName, ',');
	getGroupVec(groupMap, nodePingGroupName, nodePingNodeIdsVec);
	INFOLOG("NodePingGtoupId size:{}", nodePingNodeIdsVec.size());

	//// 获取nodeMacheState接收topic中节点id
	std::vector<std::string> nodeMstateGroupName; //nodeMacheState消息的所有组名数组
	std::vector<std::string> nodeMachStateNodeIdsVec;//nodeMacheState消息的所有组名对应的节点id数组
	utility::string::split(nodeMstateGroup, nodeMstateGroupName, ',');
	getGroupVec(groupMap, nodeMstateGroupName, nodeMachStateNodeIdsVec);
	INFOLOG("NodeMstateGtoupId size:{}", nodeMachStateNodeIdsVec.size());

	////获取cmd发送、接收topic中节点id字符串,并转换为数组
	std::vector<std::string> nodeCmdeGroupName; //nodeCmd消息的所有组名数组
	std::vector<std::string> nodeCmdTopicVec;//nodeCmd消息的所有组名对应的节点id数组
	utility::string::split(nodeCmdGroup, nodeCmdeGroupName, ',');
	getGroupVec(groupMap, nodeCmdeGroupName, nodeCmdTopicVec);
	INFOLOG("NodeCmdGtoupId size:{}", nodeCmdTopicVec.size());

	// 获取fileDown发送、接收回执topic中节点id字符串, 并转换为数组
	std::vector<std::string> fileDownGroupName; //fileDown消息的所有组名数组
	std::vector<std::string> fileDownNodeIdsVec;//fileDown消息的所有组名对应的节点id数组
	utility::string::split(fileDownGroup, fileDownGroupName, ',');
	getGroupVec(groupMap, fileDownGroupName, fileDownNodeIdsVec);
	INFOLOG("FileDownGtoupId size:{}", fileDownNodeIdsVec.size());

	//获取fileUp接收、发送回执topic中节点id字符串,并转换为数组
	std::vector<std::string> fileUpGroupName; //fileUp消息的所有组名数组
	std::vector<std::string> fileUpNodeIdsVec;//fileUp消息的所有组名对应的节点id数组
	utility::string::split(fileUpGroup, fileUpGroupName, ',');
	getGroupVec(groupMap, fileUpGroupName, fileUpNodeIdsVec);
	INFOLOG("FileUpGtoupId size:{}", fileUpNodeIdsVec.size());

	//获取监控统计topic中节点id字符串,并转换为数组
	std::vector<std::string> monGroupName; //fileUp消息的所有组名数组
	std::vector<std::string> monIdsVec;//fileUp消息的所有组名对应的节点id数组
	utility::string::split(monGroup, monGroupName, ',');
	getGroupVec(groupMap, monGroupName, monIdsVec);
	INFOLOG("monIdsVec size:{}", monIdsVec.size());
	

	ZenohTopicManager manager;

	//// 1. 创建内存Topic
	//auto memoryTopic = manager.createTopic(
	//	ZenohTopicManager::Domain::MEMORY,
	//	"project1",              // 项目名
	//	"business1",             // 业务域
	//	"region1",               // 区域
	//	"node001",              // 节点ID
	//	"perception",            // 模块
	//	"fusion",               // 功能
	//	ZenohTopicManager::Direction::UP
	//);

	//// 2. 创建本地文件持久化Topic
	//auto fileTopic = manager.createFileTopic(
	//	"project2",              // 项目名
	//	"business2",             // 业务域
	//	"region2",               // 区域
	//	"node002",              // 节点ID
	//	"storage",              // 模块
	//	"data",                 // 功能
	//	ZenohTopicManager::Direction::BOTH
	//);

	//3、节点命令Topic下发模板（下云）
	auto nodeCmdDownTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodeCmdProject,               //项目
		nodeCmdBussDomain,             // 业务域
		nodeCmdDomain,               // 区域
		nodeCmdTopicVec[0],              // 节点ID
		nodeCmdBuss,            // 模块
		nodeCmdFunc,               // 功能
		ZenohTopicManager::Direction::DOWN //消息方向
	);

	// 4、节点命令topic回执上传模板（上云）
	auto nodeCmdUpTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodeCmdProject,               //项目
		nodeCmdBussDomain,             // 业务域
		nodeCmdDomain,               // 区域
		nodeCmdTopicVec[0],              // 节点ID
		nodeCmdBuss,            // 模块
		nodeCmdFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	//5、节点配置下发模板（下云）
	auto nodeMsgBaseDownTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodeMsgProject,               //项目
		nodeMsgBussDomain,             // 业务域
		nodeMsgDomain,               // 区域
		"*",              // 节点ID
		nodeMsgBuss,            // 模块
		nodeMsgFunc,               // 功能
		ZenohTopicManager::Direction::DOWN
	);

	//6、节点配置回执模板（上云）
	auto nodeMsgBaseUpTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodeMsgProject,               //项目
		nodeMsgBussDomain,             // 业务域
		nodeMsgDomain,               // 区域
		"*",              // 节点ID
		nodeMsgBuss,            // 模块
		nodeMsgFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	//7、节点ping消息topic（向云）
	auto nodePingTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodePingProject,               //项目
		nodePingBussDomain,             // 业务域
		nodePingDomain,               // 区域
		nodePingNodeIdsVec[0],              // 节点ID(管理节点nodeId)
		nodePingBuss,            // 模块
		nodePingFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	//节点machstate消息topic（向云）
	auto nodeMstateTopic = manager.createTopic(
		ZenohTopicManager::Domain::TIMES,
		nodeMstateProject,               //项目
		nodeMstateBussDomain,             // 业务域
		nodeMstateDomain,               // 区域
		nodeMachStateNodeIdsVec[0],              // 节点ID(管理节点nodeId)
		nodeMstateBuss,            // 模块
		nodeMstateFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	//文件下发topic（下云）
	auto fileDownBaseTopic = manager.createTopic(
		ZenohTopicManager::Domain::DEFAULT,
		fileDownProject,               //项目
		fileDownBussDomain,             // 业务域
		fileDownDomain,               // 区域
		"*",              // 节点ID(应填入各分组节点的nodeId)
		fileDownBuss,            // 模块
		fileDownFunc,               // 功能
		ZenohTopicManager::Direction::DOWN
	);

	//文件下发回执topic（上云）
	auto fileDownReplyBaseTopic = manager.createTopic(
		ZenohTopicManager::Domain::DEFAULT,
		fileDownProject,               //项目
		fileDownBussDomain,             // 业务域
		fileDownDomain,               // 区域
		"*",              // 节点ID(应填入各分组节点的nodeId)
		fileDownBuss,            // 模块
		fileDownFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	// 文件上传topic（上云）
	auto fileUpBaseTopic = manager.createTopic(
		ZenohTopicManager::Domain::DEFAULT,
		fileUpProject,               //项目
		fileUpBussDomain,             // 业务域
		fileUpDomain,               // 区域
		"*",              // 节点ID(应填入各分组节点的nodeId)
		fileUpBuss,            // 模块
		fileUpFunc,               // 功能
		ZenohTopicManager::Direction::UP
	);

	//文件上云回执topic(下云)
	auto fileUpReplyBaseTopic = manager.createTopic(
		ZenohTopicManager::Domain::DEFAULT,
		fileUpProject,               //项目
		fileUpBussDomain,             // 业务域
		fileUpDomain,               // 区域
		"*",              // 节点ID(应填入各分组节点的nodeId)
		fileUpBuss,            // 模块
		fileUpFunc,               // 功能
		ZenohTopicManager::Direction::DOWN
	);

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



	// 添加Topic
	//manager.addTopic(memoryTopic);
	//manager.addTopic(fileTopic);
	manager.addTopic(nodeCmdDownTopic);
	manager.addTopic(nodeCmdUpTopic);
	manager.addTopic(nodeMsgBaseDownTopic);
	manager.addTopic(nodeMsgBaseUpTopic);
	manager.addTopic(nodePingTopic);
	manager.addTopic(nodeMstateTopic);
	manager.addTopic(fileDownBaseTopic);
	manager.addTopic(fileDownReplyBaseTopic);
	manager.addTopic(fileUpBaseTopic);
	manager.addTopic(fileUpReplyBaseTopic);
	manager.addTopic(monStatsTopic);

	// 获取Topic字符串
	/*std::cout << "Memory Topic: " << manager.buildKeyExpr(memoryTopic) << std::endl;
	std::cout << "File Topic: " << manager.buildKeyExpr(fileTopic) << std::endl;*/
	std::cout << "nodeMsgBaseDown Topic:" << manager.buildKeyExpr(nodeMsgBaseDownTopic) << std::endl;
	std::cout << "nodeMsgBaseUp Topic:" << manager.buildKeyExpr(nodeMsgBaseUpTopic) << std::endl;
	std::cout << "nodePing Topic:" << manager.buildKeyExpr(nodePingTopic) << std::endl;
	std::cout << "nodeMstate Topic:" << manager.buildKeyExpr(nodeMstateTopic) << std::endl;
	std::cout << "nodeCmdDown Topic:" << manager.buildKeyExpr(nodeCmdDownTopic) << std::endl;
	std::cout << "nodeCmdUp Topic:" << manager.buildKeyExpr(nodeCmdUpTopic) << std::endl;
	std::cout << "fileDownBaseTopic :" << manager.buildKeyExpr(fileDownBaseTopic) << std::endl;
	std::cout << "fileDownReplyBaseTopic :" << manager.buildKeyExpr(fileDownReplyBaseTopic) << std::endl;
	std::cout << "fileUpBaseTopic :" << manager.buildKeyExpr(fileUpBaseTopic) << std::endl;
	std::cout << "fileUpReplyBaseTopic :" << manager.buildKeyExpr(fileUpReplyBaseTopic) << std::endl;
	std::cout << "monStatsTopic:" << manager.buildKeyExpr(monStatsTopic) << std::endl;
	//return 0;

	//管理节点数据库连接

	if (group == "mgr")
	{
		int dbType = nodeConfig.GetInteger("db", "type", 1);
		string dbIp = nodeConfig.GetString("db", "ip", "10.4.1.203");
		string dbUser = nodeConfig.GetString("db", "user", "root");
		string dbPwd = nodeConfig.GetString("db", "pwd", "Seisys*77889900");
		string dbName = nodeConfig.GetString("db", "name", "databus-dev");
		int dbPort = nodeConfig.GetInteger("db", "port", 3306);
		int dbPoolNum = nodeConfig.GetInteger("db", "pool_num", 10);
		DbBase* db = DbBase::getDbInstance(dbType, dbIp, dbUser, dbPwd, dbName, dbPort, dbPoolNum);
	}

	//S3初始化

#ifdef USE_S3
	endpointUrl = nodeConfig.GetString("s3", "endpoint_url", "10.4.1.241:31239");
	accessKey = nodeConfig.GetString("s3", "access_key", "xNTUsotTZMMepl3N");
	secretKey = nodeConfig.GetString("s3", "secret_key", "m4rS20T2RRbuZ1DVb45N1ahWv0Smi89e");
	if (!S3Singleton::getInstance()->init(endpointUrl, accessKey, secretKey)) {
		ERRORLOG("S3 init failed. exit. please check s3 config.");
		return -1;
	}
#endif


	//INFOLOG(("Node: name:%s group:%s topic:%s participant:%d "), name, group, nodeTopic, domain);
	INFOLOG("Node: name:{},group:{} ,participant:{}", name, group, domain);

	//初始化s3Watch
	int s3WatchSize = nodeConfig.GetInteger("s3watch", "count", 0);
	for (int i = 0; i < s3WatchSize; i++)
	{
		S3WATCH s3WatchStruct;
		string bucketKey = "bucket_" + to_string(i);
		string directoryKey = "directory_" + to_string(i);
		string sleepKey = "sleep_" + to_string(i);
		string keepFileKey = "keep_file_" + to_string(i);
		string zipFileKey="zip_flag_"+ to_string(i);
		string watchSubDirKey="watch_sub_dir_"+ to_string(i);

		s3WatchStruct.bucket = nodeConfig.GetString("s3watch", bucketKey, "");
		s3WatchStruct.directory = nodeConfig.GetString("s3watch", directoryKey, "");
		s3WatchStruct.sleep=nodeConfig.GetInteger("s3watch", sleepKey, 30);
		s3WatchStruct.keepFile= nodeConfig.GetBoolean("s3watch", keepFileKey,true);
		s3WatchStruct.zipFlag = nodeConfig.GetBoolean("s3watch", zipFileKey, true);
		s3WatchStruct.watchSubDir = nodeConfig.GetBoolean("s3watch", watchSubDirKey, true);
		utility::node::nodeInfo_.s3WatchVec_.push_back(std::move(s3WatchStruct));
	}

	// 初始化进程配置
	int processSize = nodeConfig.GetInteger("process", "count", 0);
	for (int i = 0; i < processSize; i++)
	{
		PROCESS process;
		string stateKey = "state_" + to_string(i);
		string pathKey = "path_name_" + to_string(i);
		//string cmdKey = "cmd_" + to_string(i);
		string parasKey = "paras_" + to_string(i);
		string addParas1 = "paras1_" + to_string(i);
		string addParas2 = "paras2_"+ to_string(i);
		string keep_alive = "keep_alive_"+ to_string(i);
		process.state = nodeConfig.GetInteger("process", stateKey, 0);
		process.path = nodeConfig.GetString("process", pathKey, "");
		//process.cmd = nodeConfig.GetString("process", cmdKey, "");
		process.paras = nodeConfig.GetString("process", parasKey, "");
		process.add_paras1 = nodeConfig.GetString("process", addParas1, "");
		process.add_paras2 = nodeConfig.GetString("process", addParas2, "");
		process.keep_live = nodeConfig.GetBoolean("process", keep_alive, true);
		if (process.path == "")
		{
			continue;
		}
		INFOLOG("process state:{},process path:{}，process paras：{}，process paras1: {}, process paras2: {}, keep_alive: {}", process.state, process.path, process.paras, process.add_paras1, process.add_paras2, process.keep_live);
		utility::node::nodeInfo_.processVec_.push_back(std::move(process));
	}

	utility::node::nodeInfo_.domain_ = domain;
	utility::node::nodeInfo_.group_ = group;
	utility::node::nodeInfo_.location_ = location;
	utility::node::nodeInfo_.nodeId_ = nodeId;
	utility::node::nodeInfo_.nodeName_ = name;
	utility::node::nodeInfo_.version_ = version;
	utility::node::nodeInfo_.osType_ = osType;
	utility::node::nodeInfo_.archType_ = archType;
	utility::node::nodeInfo_.fileMaxReTry_ = maxRetry;
	utility::node::nodeInfo_.fileBlockTimeout_ = blockTimeout;
	utility::node::nodeInfo_.fileBlockLastTimeout_ = blockLastTime;
	utility::node::nodeInfo_.fileSplitSize_ = blockSize;
	utility::node::nodeInfo_.endpointOverride = endpointUrl;
	utility::node::nodeInfo_.accessKey = accessKey;
	utility::node::nodeInfo_.secretKey = secretKey;
	utility::node::nodeInfo_.programName_ = progarmName;

	
	// nodeMsg消息基本信息
	TopicInfo topicInfo(utility::node::nodeInfo_);
	topicInfo.nodeName_ = name;
	topicInfo.nodeId_ = nodeId;
	topicInfo.domain_ = domain;
	topicInfo.needReply_ = true;
	topicInfo.monNodeId_ = monNodeId;
	topicInfo.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	topicInfo.monSendSleep_ = monSendSleep;
	topicInfo.configPath_ = zenohConf;


	//node cmd消息基本信息
	TopicInfo topicInfoCmd((TopicInfo)utility::node::nodeInfo_);
	topicInfoCmd.nodeName_ = name;
	topicInfoCmd.nodeId_ = nodeId;
	topicInfoCmd.domain_ = domain;
	topicInfoCmd.needReply_ = true;
	topicInfoCmd.topic_ = manager.buildKeyExpr(nodeCmdDownTopic);
	topicInfoCmd.topicReply_ = manager.buildKeyExpr(nodeCmdUpTopic);
	topicInfoCmd.monNodeId_ = monNodeId;
	topicInfoCmd.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	topicInfoCmd.monSendSleep_ = monSendSleep;
	topicInfoCmd.configPath_ = zenohConf;

	//设备心跳上报：管理节点发送、接收；工作节点只负责发送
	TopicInfo topicInfoPing((TopicInfo)utility::node::nodeInfo_);
	topicInfoPing.nodeName_ = name;
	topicInfoPing.nodeId_ = nodeId;
	topicInfoPing.domain_ = domain;
	topicInfoPing.needReply_ = false;
	topicInfoPing.topic_ = manager.buildKeyExpr(nodePingTopic);
	topicInfoPing.partition_ = nodeId;
	topicInfoPing.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	topicInfoPing.monNodeId_ = monNodeId;
	topicInfoPing.monSendSleep_ = monSendSleep;
	topicInfoPing.configPath_ = zenohConf;

	if (group == "mgr")
	{
		PingTopic* pingRecv = new PingTopic(topicInfoPing);
		pingRecv->start_reader(false);
	}

	PingTopic* pingSend = new PingTopic(topicInfoPing);
	pingSend->start_writer(false);


	//机器状态信息上报：管理节点发送、接收；工作节点只负责发送
	TopicInfo topicInfoMstate((TopicInfo)utility::node::nodeInfo_);
	topicInfoMstate.nodeName_ = name;
	topicInfoMstate.nodeId_ = nodeId;
	topicInfoMstate.domain_ = domain;
	topicInfoMstate.needReply_ = false;
	topicInfoMstate.topic_ = manager.buildKeyExpr(nodeMstateTopic);
	topicInfoMstate.monNodeId_ = monNodeId;
	topicInfoMstate.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	topicInfoMstate.monSendSleep_ = monSendSleep;
	topicInfoMstate.configPath_ = zenohConf;

	if (group == "mgr")
	{
		MachineStateTopic* monRecv = new MachineStateTopic(topicInfoMstate);
		monRecv->start_reader(false);
	}

	MachineStateTopic* monSend = new MachineStateTopic(topicInfoMstate);
	monSend->start_writer(false);
	
	std::list<NodeTopic*> nodeTopicDownList;
	std::list<NodeTopic*> nodeTopicUpList;
	// 管理节点创建配置发送一对一多通道
	if (group == "mgr") {
		//topicInfo.partition_ = nodeId; //需要订阅自己的topic,收发给自己的消息
		std::list<NodeTopic*> nodeTopicDownList;
		std::string nodeMsgBaseDown = manager.buildKeyExpr(nodeMsgBaseDownTopic);
		std::string nodeMsgBaseUp = manager.buildKeyExpr(nodeMsgBaseUpTopic);

		for (int i = 0; i < nodeMsgNodeIdsVec.size(); ++i) {

			topicInfo.topic_ = utility::string::replaceFirstString(nodeMsgBaseDown, "*", nodeMsgNodeIdsVec[i]);
			topicInfo.topicReply_ = utility::string::replaceFirstString(nodeMsgBaseUp, "*", nodeMsgNodeIdsVec[i]);
			topicInfo.partition_ = nodeMsgNodeIdsVec[i];
			DEBUGLOG("send topic:{},receive reply topic:{}", topicInfo.topic_, topicInfo.topicReply_);

			auto* ntw = new NodeTopic(topicInfo);
			ntw->start_writer(false);


			//管理节点只接收发给自己的配置信息
			if (nodeMsgNodeIdsVec[i] == nodeId)
			{
				auto* ntr = new NodeTopic(topicInfo);
				ntr->start_reader(false);
				nodeTopicUpList.push_back(ntr);
			}
			nodeTopicDownList.push_back(ntw);
		}
	}
	else { 
			// 边缘节点接收发给自己的配置数据
			std::string nodeMsgBaseDown = manager.buildKeyExpr(nodeMsgBaseDownTopic);
			std::string nodeMsgBaseUp = manager.buildKeyExpr(nodeMsgBaseUpTopic);
			topicInfo.topic_ = utility::string::replaceFirstString(nodeMsgBaseDown, "*", nodeId);
			topicInfo.topicReply_ = utility::string::replaceFirstString(nodeMsgBaseUp, "*", nodeId);
			topicInfo.partition_ = nodeId;
			DEBUGLOG("receive topic:{},pub reply topic:{}", topicInfo.topic_, topicInfo.topicReply_);
			auto* ntr = new NodeTopic(topicInfo);
			//ntr->init(topicInfo);
			ntr->start_reader(false);
			nodeTopicUpList.push_back(ntr);

	}

	cppmicroservices::FrameworkFactory factory;
	auto framework = factory.NewFramework();

	static auto get_bundle = [&framework, &symbolicNameToId](const std::string& str) {
		std::stringstream ss(str);

		long int id = -1;
		ss >> id;
		if (!ss) {
			id = -1;
			auto it = symbolicNameToId.find(str);
			if (it != symbolicNameToId.end()) {
				id = it->second;
			}
		}

		return framework.GetBundleContext().GetBundle(id);
		};

	try {
		framework.Start();
		INFOLOG("Node started with success.");
	}
	catch (const std::exception& e) {
		ERRORLOG(e.what());
		return -1;
	}



	////// 一对多模式下 节点自身文件接收配置
	////// 处理一对多的读,下发通道设置
	TopicInfo fileDownInfo(utility::node::nodeInfo_);
	fileDownInfo.nodeName_ = name;
	fileDownInfo.nodeId_ = nodeId;
	fileDownInfo.domain_ = domain;
	fileDownInfo.needReply_ = true;
	fileDownInfo.topic_ = manager.buildKeyExpr(fileDownBaseTopic);
	fileDownInfo.topicReply_ = manager.buildKeyExpr(fileDownReplyBaseTopic);
	fileDownInfo.monNodeId_ = monNodeId;
	fileDownInfo.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	fileDownInfo.monSendSleep_ = monSendSleep;
	fileDownInfo.configPath_ = zenohConf;

	

	////// 文件发送配置,文件发送需要一对一建立发送通道,类似Node消息处理,群发也按这样处理,不然大文件发送会造成宽带浪费
	///上传通道设置
	TopicInfo fileUpInfo(utility::node::nodeInfo_);
	fileUpInfo.nodeName_ = name;
	fileUpInfo.nodeId_ = nodeId;
	fileUpInfo.domain_ = domain;
	fileUpInfo.needReply_ = true;
	fileUpInfo.topic_ = manager.buildKeyExpr(fileUpBaseTopic);
	fileUpInfo.topicReply_ = manager.buildKeyExpr(fileUpReplyBaseTopic);
	fileUpInfo.monNodeId_ = monNodeId;
	fileUpInfo.monTopicName_ = manager.buildKeyExpr(monStatsTopic);
	fileUpInfo.monSendSleep_ = monSendSleep;
	fileUpInfo.configPath_ = zenohConf;

	std::list<FileTopic*> fileReaderTopicList;
	std::list<FileTopic*> fileWriterTopicList;
	//管理节点,需要发送文件到每个节点（包括自己）,会建立专用管理通道

	//消息分区都必须在多端,不然管理节点没法区分消息来源,会全部收到

	if (utility::node::nodeInfo_.group_.compare("mgr") == 0) {  //处理一对多的写
		
		std::string fileBaseDown = manager.buildKeyExpr(fileDownBaseTopic);
		std::string fileBaseDownReply = manager.buildKeyExpr(fileDownReplyBaseTopic);
		std::string fileBaseUp = manager.buildKeyExpr(fileUpBaseTopic);
		std::string fileBaseUpReply = manager.buildKeyExpr(fileUpReplyBaseTopic);

		for (int i = 0; i < fileDownNodeIdsVec.size(); i++)
		{

			fileDownInfo.topic_ = utility::string::replaceFirstString(fileBaseDown, "*", fileDownNodeIdsVec[i]);
			fileDownInfo.topicReply_ = utility::string::replaceFirstString(fileBaseDownReply, "*", fileDownNodeIdsVec[i]);
			fileDownInfo.partition_ = fileDownNodeIdsVec[i];
			fileDownInfo.replyPartition_= fileDownNodeIdsVec[i];

			fileUpInfo.topic_ = utility::string::replaceFirstString(fileBaseUp, "*", fileDownNodeIdsVec[i]);
			fileUpInfo.topicReply_ = utility::string::replaceFirstString(fileBaseUpReply, "*", fileDownNodeIdsVec[i]);
			fileUpInfo.partition_ = fileDownNodeIdsVec[i];
			fileUpInfo.replyPartition_ = fileDownNodeIdsVec[i];
			DEBUGLOG("file down topic:{},file down reply topic:{},file up topic:{},file up reply topic:{}", fileDownInfo.topic_, fileDownInfo.topicReply_, fileUpInfo.topic_, fileUpInfo.topicReply_);

			//下发-写
			auto ftw = new FileTopic(fileDownInfo);
			ftw->start_writer(false);
			fileWriterTopicList.push_back(ftw);
			//上传-读
			auto ftr = new FileTopic(fileUpInfo);
			ftr->start_reader(false);
			fileReaderTopicList.push_back(ftr);
		}
	}
	else { //工作节点,需要发送文件到对应的管理节点（fileSendInfo.partition_.replyPartition_）

		std::string fileBaseDown = manager.buildKeyExpr(fileDownBaseTopic);
		std::string fileBaseDownReply = manager.buildKeyExpr(fileDownReplyBaseTopic);
		std::string fileBaseUp = manager.buildKeyExpr(fileUpBaseTopic);
		std::string fileBaseUpReply = manager.buildKeyExpr(fileUpReplyBaseTopic);

		fileDownInfo.topic_ = utility::string::replaceFirstString(fileBaseDown, "*", nodeId);
		fileDownInfo.topicReply_ = utility::string::replaceFirstString(fileBaseDownReply, "*", nodeId);
		fileDownInfo.partition_ = nodeId;
		fileDownInfo.replyPartition_ = nodeId;

		fileUpInfo.topic_ = utility::string::replaceFirstString(fileBaseUp, "*", nodeId);
		fileUpInfo.topicReply_ = utility::string::replaceFirstString(fileBaseUpReply, "*", nodeId);
		fileUpInfo.partition_ = nodeId;
		fileUpInfo.replyPartition_ = nodeId;
		DEBUGLOG("file down topic:{},file down reply topic:{},file up topic:{},file up reply topic:{}", fileDownInfo.topic_, fileDownInfo.topicReply_, fileUpInfo.topic_, fileUpInfo.topicReply_);

		//上传-写
		auto ftw = new FileTopic(fileUpInfo);
		ftw->start_writer(false);
		fileWriterTopicList.push_back(ftw);

		//下发-读
		auto ftr = new FileTopic(fileDownInfo);
		ftr->start_reader(false);
		fileReaderTopicList.push_back(ftr);
	}



	////进程启动必须在s3watch启动后
	////通用群发消息,支持相同消息的群发和回执
	NodeCmdTopic* nct = new NodeCmdTopic(topicInfoCmd);
	NodeCmdTopic* mgr_nct = new NodeCmdTopic(topicInfoCmd);
	NodeCmdWorker* ncw = new NodeCmdWorker();

	ncw->topicInfo_ = topicInfoCmd;

	if (group == "mgr") {  //管理节点
		nct->start_writer(argc, argv, false);
		mgr_nct->start_reader(argc, argv, false);
		ncw->init(factory, framework, symbolicNameToId);
		ncw->start_cmd_worker(argc, argv, false);
	}
	//所有节点都接收cmd命令
	else {
		nct->start_reader(argc, argv, false);
		ncw->init(factory, framework, symbolicNameToId);
		ncw->start_cmd_worker(argc, argv, false);
	}



	///***********************************************************************************
	///读取plugins配置,加载控件  
	///***********************************************************************************

	/*if (framework.GetState() == cppmicroservices::Bundle::STATE_ACTIVE) {
		int pluginCount = nodeConfig.GetInteger("plugins", "count", 0);

		for (int m = 0; m < pluginCount; m++) {
			string plugin_name = "plugin_name_" + to_string(m);
			string plugin_path_name = "plugin_path_" + to_string(m);
			string plugin_dll = nodeConfig.GetString("plugins", plugin_name, "");
			if (osType.compare("windows") == 0) {
				plugin_dll += ".dll";
			}
			else {
				plugin_dll += ".so";
			}
			string plgin_path = nodeConfig.GetString("plugins", plugin_path_name, "");

			string full_path_name = plgin_path + "/" + plugin_dll;

			INFOLOG("load dll: {}", plugin_dll);
			try {
				framework.GetBundleContext().InstallBundles(plugin_dll);
			}
			catch (const std::exception& e) {
				ERRORLOG("load plugin {} with error: {}", plugin_dll, e.what());
			}


		}
	}

	
	for (auto b : framework.GetBundleContext().GetBundles()) {
		symbolicNameToId[b.GetSymbolicName()] = b.GetBundleId();
		auto bundle = get_bundle(b.GetSymbolicName());
		if (bundle) {
			try {
				if (bundle.GetState() == cppmicroservices::Bundle::STATE_ACTIVE) {
					INFOLOG("Info: bundle already active");
				}
				else {
					bundle.Start();
				}
			}
			catch (const std::exception& e) {
				ERRORLOG("start plugin  with error: {}", e.what());
			}
		}
	}
	*/


	while (true) {
		DEBUGLOG("node main is running ...");
		std::this_thread::sleep_for(std::chrono::milliseconds(5000));
	}

	S3Singleton::getInstance()->uinit();
	framework.Stop();
	framework.WaitForStop(std::chrono::seconds(2));

	return 0;
}

#ifndef US_BUILD_SHARED_LIBS
CPPMICROSERVICES_INITIALIZE_STATIC_BUNDLE(system_bundle)
CPPMICROSERVICES_IMPORT_BUNDLE(mms_service)
CPPMICROSERVICES_IMPORT_BUNDLE(clock_service)
CPPMICROSERVICES_IMPORT_BUNDLE(clock_consumer_service)
#endif
