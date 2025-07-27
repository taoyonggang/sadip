#pragma once
/// <summary>
/// NodeCmd消息处理类，需要处理下面消息类型
/// 			REBOOT,    //重启节点
/// 			RESTART,   //重启应用
/// 			STARTPLUGIN,  //启动插件
/// 			STOPPLUGIN,   //停止插件
/// 			RESTARTPLUGIN,  //重启插件
/// 			LS,              //查询目录
/// 			MKDIR,          //创建目录
/// 			DEL,            //删除文件
/// 			DELDIR,         //删除目录
/// 			SENDFILE,   //传输文件
/// 			RUNAPP,     //启动独立进程
/// 			STOPAPP,    //停止独立进程
/// 			RESTARTAPP  //重新启动独立进程
/// </summary>

#include <cppmicroservices/Bundle.h>
#include <cppmicroservices/BundleContext.h>
#include <cppmicroservices/Framework.h>
#include <cppmicroservices/FrameworkEvent.h>
#include <cppmicroservices/FrameworkFactory.h>

#include "../../dds/TopicInfo.h"
#include "ProcessManager.h"
#include "ExecManager.h"
#include "LogMonitorManager.h"
#include "PodmanManager.h"

#include "CommandExecutor.h"
#include "proto/node.pb.h"
#include "proto/file.pb.h"
//#include "node.h"

using namespace std;

class NodeCmdWorker
{
public:
    static std::unordered_map<std::string, long>* symbolicNameToId;
    static cppmicroservices::FrameworkFactory*  factory;

    static cppmicroservices::Framework * framework;

    TopicInfo topicInfo_;

public:
    static ProcessManager * pm_;
    static ExecManager* exec_;
    static LogMonitorManager* lmm_;
    static CommandManager* cm_;
    static PodmanManager* podm_;


public:
    bool init(cppmicroservices::FrameworkFactory& factory, cppmicroservices::Framework &framework, std::unordered_map<std::string, long> &symbolicNameToId);
    void uinit();

	void start_cmd_worker(int argc, char** argv, bool wait = false);

	static void worker(const void* arg);

    void setLsPluginDesc(NodeCmdWorker* nodeCmdWorker, cn::seisys::dds::NodeCmdReply& nodeCmdReply);
    void setLsAppDesc(std::vector<ProcessAttribute> &instances, cn::seisys::dds::NodeCmdReply& nodeCmdReply, string paras1,string paras2);
    string combUuid(std::string cmd, std::string paras, std::string paras1, std::string paras2);

    cppmicroservices::Bundle get_bundle(const std::string& str);

private:
    bool start_plugin(std::string pluginName);
    bool stop_plugin(std::string pluginName);
};

