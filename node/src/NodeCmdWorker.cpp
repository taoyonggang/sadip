#include "../../utils/pch.h"

#include <iostream>
#include <string>
#include <chrono>  

#include "NodeCmdWorker.h"
#include "../../utils/msgQueue.h"
#include "../../utils/uuid.h"
#include "../../utils/split.h"

#include <Poco/File.h>
#include <Poco/DirectoryIterator.h>

#include "PodmanManager.h"
#include "proto/node.pb.h"
#include "LogMonitorManager.h"


using namespace cn::seisys::dds;
using namespace std;
//using namespace eprosima::fastrtps;

NodeInfo utility::node::nodeInfo_;

moodycamel::ConcurrentQueue<::NodeCmd> utility::msg::mainCmdQueueIn_;
moodycamel::ConcurrentQueue<::NodeCmd> utility::msg::mainCmdQueueOut_;
moodycamel::ConcurrentQueue<::NodeCmdReply> utility::msg::mainCmdReplyQueueIn_;
moodycamel::ConcurrentQueue<::NodeCmdReply> utility::msg::mainCmdReplyQueueOut_;
moodycamel::ConcurrentQueue<::NodeCmd> utility::msg::fileDataQueueIn_;
moodycamel::ConcurrentQueue<::NodeCmd> utility::msg::fileDataQueueOut_;
moodycamel::ConcurrentQueue<::NodeCmdReply> utility::msg::fileDataReplyQueueIn_;
moodycamel::ConcurrentQueue<::NodeCmdReply> utility::msg::fileDataReplyQueueOut_;

std::unordered_map<std::string, long>* NodeCmdWorker::symbolicNameToId = nullptr;
cppmicroservices::FrameworkFactory* NodeCmdWorker::factory = nullptr;
cppmicroservices::Framework* NodeCmdWorker::framework = nullptr;

ProcessManager* NodeCmdWorker::pm_ = nullptr;
//ExecManager* NodeCmdWorker::exec_ = nullptr;
LogMonitorManager* NodeCmdWorker::lmm_ = nullptr;

PodmanManager* NodeCmdWorker::podm_ = nullptr;

CommandManager* NodeCmdWorker::cm_ = nullptr;
std::unordered_map<std::string, Poco::ProcessHandle*> CommandExecutor::process_handles;
std::unordered_map<std::string, bool> CommandExecutor::process_running;

using json = Poco::JSON::Object::Ptr;

cppmicroservices::Bundle NodeCmdWorker::get_bundle(const std::string& str) {
	std::stringstream ss(str);

	long int id = -1;
	ss >> id;
	if (!ss) {
		id = -1;
		auto it = symbolicNameToId->find(str);
		if (it != symbolicNameToId->end()) {
			id = it->second;
		}
	}

	return framework->GetBundleContext().GetBundle(id);
};


bool NodeCmdWorker::init(cppmicroservices::FrameworkFactory& factory, cppmicroservices::Framework& framework, std::unordered_map<std::string, long>& symbolicNameToId) {
	this->factory = &factory;
	this->framework = &framework;
	this->symbolicNameToId = &symbolicNameToId;

	if (NodeCmdWorker::pm_ == nullptr) {
		NodeCmdWorker::pm_ = new ProcessManager();
	}

	//if (NodeCmdWorker::exec_ == nullptr) {
	//	NodeCmdWorker::exec_ = new ExecManager();
	//}

	if (NodeCmdWorker::cm_ == nullptr) {
		NodeCmdWorker::cm_ = new CommandManager();
	}

	if (NodeCmdWorker::lmm_ == nullptr) {
		TopicInfo logTopic = this->topicInfo_;
		logTopic.topic_ = "LogDataTopic";
		logTopic.partition_ = this->topicInfo_.nodeId_;
		logTopic.needReply_ = false;
		NodeCmdWorker::lmm_ = new LogMonitorManager(logTopic);
	}

	if (NodeCmdWorker::podm_ == nullptr) {
		NodeCmdWorker::podm_ = new PodmanManager(this->topicInfo_.podmanBaseUrl_);
	}

	return true;
}

void NodeCmdWorker::uinit() {
	if (NodeCmdWorker::pm_ != nullptr) {
		delete NodeCmdWorker::pm_;
		NodeCmdWorker::pm_ = nullptr;
	}

	//if (NodeCmdWorker::exec_ != nullptr) {
	//	delete NodeCmdWorker::exec_;
	//	NodeCmdWorker::exec_ = nullptr;
	//}


	if (NodeCmdWorker::cm_ != nullptr) {
		delete NodeCmdWorker::cm_;
		NodeCmdWorker::cm_ = nullptr;
	}

	if (NodeCmdWorker::lmm_ != nullptr) {
		delete NodeCmdWorker::lmm_;
		NodeCmdWorker::lmm_ = nullptr;
	}

	if (NodeCmdWorker::podm_ != nullptr) {
		delete NodeCmdWorker::podm_;
		NodeCmdWorker::podm_ = nullptr;
	}
}

void NodeCmdWorker::start_cmd_worker(int argc, char** argv, bool wait) {

	INFOLOG("start_cmd_worker() - start the NodeCmd worker thread!");

	std::thread threadId(NodeCmdWorker::worker, this);
	//必须要分离,保证独立运行,不然会出莫名奇妙错误
	threadId.detach();
}

void NodeCmdWorker::worker(const void* arg) {
	INFOLOG("worker() - start the NodeCmd worker is runing!");
	NodeCmdWorker* nodeCmdWorker = (NodeCmdWorker*)arg;
	bool hasWork = false;

	//*************************************************************
	// 增加从配置文件里面增加和启动app的代码。
	try
	{
		size_t processNum = nodeCmdWorker->topicInfo_.processVec_.size();
		INFOLOG("Self-start app num:{}", processNum);
		for (size_t i = 0; i < processNum; i++)
		{
			string cmd = utility::string::trim(nodeCmdWorker->topicInfo_.processVec_[i].path) + " " + utility::string::trim(nodeCmdWorker->topicInfo_.processVec_[i].paras);
			string uuid = nodeCmdWorker->combUuid(nodeCmdWorker->topicInfo_.processVec_[i].add_paras1, nodeCmdWorker->topicInfo_.processVec_[i].path, nodeCmdWorker->topicInfo_.processVec_[i].paras, nodeCmdWorker->topicInfo_.processVec_[i].add_paras2);
			INFOLOG("app add process with uuid:{},cmd:{}", uuid,cmd);
			int appState = nodeCmdWorker->topicInfo_.processVec_[i].state;
			string appPath = nodeCmdWorker->topicInfo_.processVec_[i].path;
			string paras = nodeCmdWorker->topicInfo_.processVec_[i].paras;
			string addParas1 = nodeCmdWorker->topicInfo_.processVec_[i].add_paras1;
			string addParas2 = nodeCmdWorker->topicInfo_.processVec_[i].add_paras2;
			bool keepLive = nodeCmdWorker->topicInfo_.processVec_[i].keep_live;
			INFOLOG("appState: {}, appPath: {}, paras: {}, addParas1:{}, addParas2:{}", appState,appPath, paras, addParas1, addParas2);
			if (nodeCmdWorker->pm_->add_process(uuid, addParas1, cmd, keepLive))
			{
				INFOLOG("Add app sucess! cmd:{}", cmd);
			}
			else
			{
				ERRORLOG("Add app failed:paras:{}", uuid);
			}

			if (nodeCmdWorker->topicInfo_.processVec_[i].state == 2)
			{

				//string appAddRunParas = nodeCmdWorker->topicInfo_.processVec_[i].path + " " + nodeCmdWorker->topicInfo_.processVec_[i].paras;
				string appAddRunParas = nodeCmdWorker->combUuid(nodeCmdWorker->topicInfo_.processVec_[i].add_paras1, nodeCmdWorker->topicInfo_.processVec_[i].path, nodeCmdWorker->topicInfo_.processVec_[i].paras, nodeCmdWorker->topicInfo_.processVec_[i].add_paras2);
				INFOLOG("worker() - start app process with uuid:{}", uuid);

				if (nodeCmdWorker->pm_->start_process(uuid))
				{
					INFOLOG("Start app sucess! uuid: {}", uuid);
				}
				else
				{
					ERRORLOG("Start app failed:paras:{}", uuid);
				}
			}
		}
	}
	catch (const std::exception& e)
	{
		ERRORLOG("Add or start app failed: error:{}", e.what());
	}


	while (true) {
		NodeCmd nodeCmd;
		nodeCmd.set_uuid("");
		//utility::msg::mainCmdQueueIn_.WaitAndPop(nodeCmd); //必须先在外面检查是否是匹配到目的节点,不然不能进队列
		bool found = utility::msg::mainCmdQueueIn_.try_dequeue(nodeCmd);

		uint64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (found) {
			INFOLOG("get cmd:{},cmdType:{} srcNodeId:{}", nodeCmd.cmd(), nodeCmd.cmd_type(), nodeCmd.src_node_id());
			NodeCmdReply nodeCmdReply;
			nodeCmdReply.set_uuid(utility::uuid::generate());
			nodeCmdReply.set_cmd_reply_uuid(nodeCmd.uuid());
			nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);  //默认失败
			nodeCmdReply.set_to_node_id(nodeCmd.src_node_id());
			nodeCmdReply.set_to_node_name(nodeCmd.src_node_name());
			nodeCmdReply.set_src_node_id(utility::node::nodeInfo_.nodeId_);
			nodeCmdReply.set_src_node_name(utility::node::nodeInfo_.nodeName_);
			nodeCmdReply.set_cmd_type(nodeCmd.cmd_type());
			/* nodeCmdReply.createdAt(now);
			 nodeCmdReply.updatedAt(now);*/
			try {
				switch (nodeCmd.cmd_type()) {
				case ::CmdType::REBOOT:
					if (nodeCmd.cmd().find("REBOOT") != std::string::npos) {
						INFOLOG("run reboot cmd:{} paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						system("shutdown -r -t 0");
						//set nodeCmdReply
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::RESTART:
					if (nodeCmd.cmd().find("RESTART") != std::string::npos) {
						INFOLOG("My process will exit(0) cmd:{} paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						// kill所有应用进程
						if (nodeCmdWorker->pm_->kill_all_process())
						{
							INFOLOG("kill all process sucess");
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						else
						{
							INFOLOG("kill all process fail");
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
						utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						exit(0);
					}
					break;
				case ::CmdType::ADDPLUGIN:
					if (nodeCmd.cmd().find("ADDPLUGIN") != std::string::npos && nodeCmd.paras().size() > 0) {
						string paras = nodeCmd.paras();
						if (utility::node::nodeInfo_.osType_.compare("windows") == 0) {
							paras += ".dll";
						}
						else {
							paras += ".so";
						}
						auto dll = paras;
						try {
							INFOLOG("load the dll:{}", dll);
							nodeCmdWorker->framework->GetBundleContext().InstallBundles(dll);
							for (auto b : nodeCmdWorker->framework->GetBundleContext().GetBundles()) {
								(*(nodeCmdWorker->symbolicNameToId))[b.GetSymbolicName()] = b.GetBundleId();
							}
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						catch (const std::exception& e) {
							ERRORLOG("load the dll{} error:{}", dll, e.what());;
						}
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
					}
					else {
						ERRORLOG("load the dll{} error:{}", nodeCmd.cmd(), "invalid paras");
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::REMOVEPLUGIN:
					if (nodeCmd.cmd().find("REMOVEPLUGIN") != std::string::npos && nodeCmd.paras().size() > 0) {
						auto bundle = nodeCmdWorker->get_bundle(nodeCmd.paras());
						DEBUGLOG("nodeCmd.paras:{}", nodeCmd.paras());
						try {
							bundle.Uninstall();
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						catch (const std::exception& e) {
							ERRORLOG("uninstall the dll{} error:{}", nodeCmd.paras(), e.what());;
						}
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
					}
					else {
						ERRORLOG("uninstall the dll{} error:{}", nodeCmd.cmd(), "invalid paras");
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STARTPLUGIN:
					if (nodeCmd.cmd().find("STARTPLUGIN") != std::string::npos && nodeCmd.paras().size() > 0) {
						if (nodeCmdWorker->start_plugin(nodeCmd.paras())) {
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						else {
							ERRORLOG("start plugin the dll{} error:{}", nodeCmd.cmd(), "invalid paras");
						}
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STOPPLUGIN:
					if (nodeCmd.cmd().find("STOPPLUGIN") != std::string::npos && nodeCmd.paras().size() > 0) {
						if (nodeCmdWorker->stop_plugin(nodeCmd.paras())) {
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						else {
							ERRORLOG("stop plugin the dll{} error:{}", nodeCmd.cmd(), "invalid paras");
						}
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::RESTARTPLUGIN:
					if (nodeCmd.cmd().find("RESTARTPLUGIN") != std::string::npos && nodeCmd.paras().size() > 0) {
						nodeCmdWorker->stop_plugin(nodeCmd.paras());
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						if (nodeCmdWorker->start_plugin(nodeCmd.paras())) {
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
					}
					else {

						ERRORLOG("stop plugin the dll{} error:{}", nodeCmd.cmd(), "invalid paras");
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::LSPLUGIN:
					if (nodeCmd.cmd().find("LSPLUGIN") != std::string::npos) {
						nodeCmdWorker->setLsPluginDesc(nodeCmdWorker, nodeCmdReply);
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					}
					else {
						ERRORLOG("list plugin {} error:{}", nodeCmd.cmd(), "invalid paras");
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::ADDAPP:
					if (nodeCmd.cmd().size() > 1) {
						string cmd = nodeCmd.paras_1() + nodeCmd.cmd();
						//string cmd = nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmd));
						if (pathName.exists()) {
							string cmdStr = nodeCmd.cmd();
							string paras = nodeCmd.paras();
							string cmd = utility::string::trim(cmdStr) + " " + utility::string::trim(paras);
							string uuid = nodeCmdWorker->combUuid(nodeCmd.paras_1(), nodeCmd.cmd(), nodeCmd.paras(), nodeCmd.paras_2());
							string runPath = nodeCmd.paras_1();
							DEBUGLOG("uuid:{},runPath;{}", uuid, runPath);
							if (nodeCmdWorker->pm_->add_process(uuid, runPath, cmd))
							{
								nodeCmdWorker->pm_->ls_process();
								nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
							else {
								ERRORLOG("run add process,process is exist:uuid:{},:cmd:{},paras:{}", uuid, nodeCmd.cmd(), nodeCmd.paras());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
					}
					else
					{
						ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::REMOVEAPP:
					if (nodeCmd.cmd().size() > 1) {
						string cmd = nodeCmd.paras_1() + nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmd));
						if (pathName.exists()) {
							//string uuid = utility::string::trim(nodeCmd.cmd()) + " " + utility::string::trim(nodeCmd.paras());
							string uuid = nodeCmdWorker->combUuid(nodeCmd.paras_1(), nodeCmd.cmd(), nodeCmd.paras(), nodeCmd.paras_2());
							DEBUGLOG("remove app uuid:{}", uuid);
							if (nodeCmdWorker->pm_->remove_process(uuid))
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								nodeCmdWorker->pm_->ls_process();
								nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
							else {
								ERRORLOG("run add process,process is exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					else {
						ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STARTAPP:
					if (nodeCmd.cmd().size() > 1) {
						string cmd = nodeCmd.paras_1() + nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmd));
						if (pathName.exists()) {
							//string uuid = utility::string::trim(nodeCmd.cmd()) + " " + utility::string::trim(nodeCmd.paras());
							string uuid = nodeCmdWorker->combUuid(nodeCmd.paras_1(), nodeCmd.cmd(), nodeCmd.paras(), nodeCmd.paras_2());
							if (nodeCmdWorker->pm_->start_process(uuid))
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								nodeCmdWorker->pm_->ls_process();
								nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
							else {
								ERRORLOG("run process,process is exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STOPAPP:
					if (nodeCmd.cmd().size() > 1) {
						string cmd = nodeCmd.paras_1() + nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmd));
						if (pathName.exists()) {
							//string uuid = utility::string::trim(nodeCmd.cmd()) + " " + utility::string::trim(nodeCmd.paras());
							string uuid = nodeCmdWorker->combUuid(nodeCmd.paras_1(), nodeCmd.cmd(), nodeCmd.paras(), nodeCmd.paras_2());
							if (nodeCmdWorker->pm_->kill_process(uuid))
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								nodeCmdWorker->pm_->ls_process();
								nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
							else {
								ERRORLOG("run process,process is exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::RESTARTAPP:
					if (nodeCmd.cmd().size() > 1) {
						string cmd = nodeCmd.paras_1() + nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmd));
						if (pathName.exists()) {
							//string uuid = utility::string::trim(nodeCmd.cmd()) + " " + utility::string::trim(nodeCmd.paras());
							string uuid = nodeCmdWorker->combUuid(nodeCmd.paras_1(), nodeCmd.cmd(), nodeCmd.paras(), nodeCmd.paras_2());
							if (nodeCmdWorker->pm_->kill_process(uuid))
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								nodeCmdWorker->pm_->start_process(uuid);
								std::this_thread::sleep_for(std::chrono::milliseconds(1000));
								nodeCmdWorker->pm_->ls_process();
								nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
							else {
								ERRORLOG("restart process,process is exist:cmd:{},paras:{}", nodeCmd.paras(), "./");
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::LSAPP:
					if (nodeCmd.cmd().find("LSAPP") != std::string::npos) {
						nodeCmdWorker->pm_->ls_process();
						nodeCmdWorker->setLsAppDesc(nodeCmdWorker->pm_->m_instances, nodeCmdReply, nodeCmd.paras_1(), nodeCmd.paras_2());
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					}
					else
					{
						ERRORLOG("list app {} error:{}", nodeCmd.cmd(), "invalid paras");
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STARTTRACELOG:
					if (nodeCmd.cmd().size() > 1) {
						INFOLOG("start Log Trace with cmd:{} paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						string cmdStr = nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmdStr));
						if (pathName.exists()) {
							string cmd = utility::string::trim(cmdStr);
							nodeCmdWorker->lmm_->startMonitoring(cmd);
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							nodeCmdReply.set_desc(nodeCmdWorker->lmm_->getActiveMonitors());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);

						}
						else {
							ERRORLOG("log path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::STOPTRACELOG:
					if (nodeCmd.cmd().size() > 1) {
						INFOLOG("stop Log Trace with cmd:{} paras:{}", nodeCmd.cmd(), nodeCmd.paras());
						string cmdStr = nodeCmd.cmd();
						Poco::File pathName(utility::string::trim(cmdStr));
						if (pathName.exists()) {
							string cmd = utility::string::trim(cmdStr);
							DEBUGLOG("stop Log 1");
							nodeCmdWorker->lmm_->stopMonitoring(cmd);
							DEBUGLOG("stop Log 2");
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
							DEBUGLOG("stop Log 3");
							string jsonStr = nodeCmdWorker->lmm_->getActiveMonitors();
							DEBUGLOG("stop Log get jsonStr:{}", jsonStr);
							nodeCmdReply.set_desc(jsonStr);
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
						}
						else {
							ERRORLOG("log path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case ::CmdType::LSTRACELOG:
					INFOLOG("ls Log Trace with cmd:{} error:{}", nodeCmd.cmd(), nodeCmd.paras());
					nodeCmdReply.set_desc(nodeCmdWorker->lmm_->getActiveMonitors());
					nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case::CmdType::STARTEXEC:
					if (nodeCmd.cmd().size() > 1) {
						string cmdStr = nodeCmd.cmd();
						string paraStr = nodeCmd.paras();
						INFOLOG("exec process with cmd:{},para:{}", nodeCmd.cmd(), nodeCmd.paras());
						//Poco::File pathName(utility::string::trim(nodeCmd.cmd()));
						if (true) {//pathName.exists()) {							
							string cmd = utility::string::trim(cmdStr);
							Poco::Process::Args args;
							utility::string::split(utility::string::trim(paraStr), args);
							//args.push_back(utility::string::trim(nodeCmd.paras()));
							string uuid = cmd + " " + utility::string::trim(paraStr);
							INFOLOG("exec process begin with uuid:{},cmd:{},para:{}", uuid, nodeCmd.cmd(), nodeCmd.paras());;
							//if (nodeCmdWorker->exec_->startProcess(cmd,args,uuid,2000))
							nodeCmdWorker->cm_->execute_command(uuid);
							{
								INFOLOG("exec process success with uuid:{},cmd:{},para:{}", uuid, nodeCmd.cmd(), nodeCmd.paras());
								std::this_thread::sleep_for(std::chrono::milliseconds(200));
								nodeCmdReply.set_desc(nodeCmdWorker->cm_->get_output(uuid));
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}

						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case::CmdType::STOPEXEC:
					if (nodeCmd.cmd().size() > 1) {
						string cmdStr = nodeCmd.cmd();
						string paraStr = nodeCmd.paras();
						Poco::File pathName(utility::string::trim(cmdStr));
						if (true) {//pathName.exists()) {	
							string cmd = utility::string::trim(cmdStr);
							Poco::Process::Args args;
							args.push_back(utility::string::trim(paraStr));
							string uuid = cmd + " " + utility::string::trim(paraStr);
							ERRORLOG("stop process uuid:{}", uuid);
							nodeCmdWorker->cm_->kill_command(uuid);
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(200));
								ERRORLOG("rm process uuid:{}", uuid);
								nodeCmdReply.set_desc(nodeCmdWorker->cm_->getAllProcesses());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case::CmdType::RMEXEC:
					if (nodeCmd.cmd().size() > 1) {
						string cmdStr = nodeCmd.cmd();
						string paraStr = nodeCmd.paras();
						Poco::File pathName(utility::string::trim(cmdStr));
						if (true) {//pathName.exists()) {	
							string cmd = utility::string::trim(cmdStr);
							Poco::Process::Args args;
							args.push_back(utility::string::trim(paraStr));
							string uuid = cmd + " " + utility::string::trim(paraStr);
							ERRORLOG("stop process uuid:{}", uuid);
							nodeCmdWorker->cm_->kill_command(uuid);
							{
								std::this_thread::sleep_for(std::chrono::milliseconds(200));
								ERRORLOG("rm process uuid:{}", uuid);
								nodeCmdReply.set_desc(nodeCmdWorker->cm_->getAllProcesses());
								nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
							}
						}
						else {
							ERRORLOG("file path name is not exist:cmd:{},paras:{}", nodeCmd.cmd(), nodeCmd.paras());
							nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::INVALID);
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case::CmdType::LSEXEC:
					if (nodeCmd.cmd().size() > 1) {
						nodeCmdReply.set_desc(nodeCmdWorker->cm_->getAllProcesses());
						nodeCmdReply.set_status(::cn::seisys::dds::CommStatus::SUCCESS);
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				case::CmdType::PODMANCMD:
					if (nodeCmd.podman_cmd() > 0 && nodeCmd.podman_cmd() <= 255 && podm_ != nullptr) {
						INFOLOG("执行Podman命令: 类型:{}, 参数:{}, 参数1:{}, 参数2:{}", nodeCmd.podman_cmd(), nodeCmd.paras(), nodeCmd.paras_1(), nodeCmd.paras_2());
						try {
							switch (static_cast<PodmanCmd>(nodeCmd.podman_cmd())) {
							case PodmanCmd::LIST_CONTAINERS:
								DEBUGLOG("执行listContainers命令");
								nodeCmdReply.set_desc_str(podm_->listContainers(true));
								break;
							case PodmanCmd::CREATE_CONTAINER:
								DEBUGLOG("执行createContainer命令");
								nodeCmdReply.set_desc_str(podm_->createContainer(PodmanManager::stringToJson(nodeCmd.paras_2())));
								break;
							case PodmanCmd::START_CONTAINER:
								DEBUGLOG("执行startContainer命令");
								nodeCmdReply.set_desc_str(podm_->startContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::STOP_CONTAINER:
								DEBUGLOG("执行stopContainer命令");
								nodeCmdReply.set_desc_str(podm_->stopContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::REMOVE_CONTAINER:
								DEBUGLOG("执行removeContainer命令");
								nodeCmdReply.set_desc_str(podm_->removeContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::INSPECT_CONTAINER:
								DEBUGLOG("执行inspectContainer命令");
								nodeCmdReply.set_desc_str(podm_->inspectContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::RESTART_CONTAINER:
								DEBUGLOG("执行restartContainer命令");
								nodeCmdReply.set_desc_str(podm_->restartContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::PAUSE_CONTAINER:
								DEBUGLOG("执行pauseContainer命令");
								nodeCmdReply.set_desc_str(podm_->pauseContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::UNPAUSE_CONTAINER:
								DEBUGLOG("执行unpauseContainer命令");
								nodeCmdReply.set_desc_str(podm_->unpauseContainer(nodeCmd.paras()));
								break;
							case PodmanCmd::EXEC_IN_CONTAINER:
								DEBUGLOG("执行execInContainer命令");
								nodeCmdReply.set_desc_str(podm_->execInContainer(nodeCmd.paras(), PodmanManager::stringToJson(nodeCmd.paras_2())));
								break;
							case PodmanCmd::LIST_IMAGES:
								DEBUGLOG("执行listImages命令");
								nodeCmdReply.set_desc_str(podm_->listImages());
								break;
							case PodmanCmd::PULL_IMAGE:
								DEBUGLOG("执行pullImage命令");
								nodeCmdReply.set_desc_str(podm_->pullImage(nodeCmd.paras(), nodeCmd.paras_1(), nodeCmd.paras_2()));
								break;
							case PodmanCmd::REMOVE_IMAGE:
								DEBUGLOG("执行removeImage命令");
								nodeCmdReply.set_desc_str(podm_->removeImage(nodeCmd.paras()));
								break;
							case PodmanCmd::INSPECT_IMAGE:
								DEBUGLOG("执行inspectImage命令");
								nodeCmdReply.set_desc_str(podm_->inspectImage(nodeCmd.paras()));
								break;
							case PodmanCmd::TAG_IMAGE:
								DEBUGLOG("执行tagImage命令");
								nodeCmdReply.set_desc_str(podm_->tagImage(nodeCmd.paras(), nodeCmd.paras_1()));
								break;
							case PodmanCmd::LIST_NETWORKS:
								DEBUGLOG("执行listNetworks命令");
								nodeCmdReply.set_desc_str(podm_->listNetworks());
								break;
							case PodmanCmd::CREATE_NETWORK:
								DEBUGLOG("执行createNetwork命令");
								nodeCmdReply.set_desc_str(podm_->createNetwork(PodmanManager::stringToJson(nodeCmd.paras_2())));
								break;
							case PodmanCmd::REMOVE_NETWORK:
								DEBUGLOG("执行removeNetwork命令");
								nodeCmdReply.set_desc_str(podm_->removeNetwork(nodeCmd.paras()));
								break;
							case PodmanCmd::INSPECT_NETWORK:
								DEBUGLOG("执行inspectNetwork命令");
								nodeCmdReply.set_desc_str(podm_->inspectNetwork(nodeCmd.paras()));
								break;
							case PodmanCmd::LIST_VOLUMES:
								DEBUGLOG("执行listVolumes命令");
								nodeCmdReply.set_desc_str(podm_->listVolumes());
								break;
							case PodmanCmd::CREATE_VOLUME:
								DEBUGLOG("执行createVolume命令");
								nodeCmdReply.set_desc_str(podm_->createVolume(PodmanManager::stringToJson(nodeCmd.paras_2())));
								break;
							case PodmanCmd::REMOVE_VOLUME:
								DEBUGLOG("执行removeVolume命令");
								nodeCmdReply.set_desc_str(podm_->removeVolume(nodeCmd.paras()));
								break;
							case PodmanCmd::INSPECT_VOLUME:
								DEBUGLOG("执行inspectVolume命令");
								nodeCmdReply.set_desc_str(podm_->inspectVolume(nodeCmd.paras()));
								break;
							case PodmanCmd::LIST_STACKS:
								DEBUGLOG("执行listStacks命令");
								nodeCmdReply.set_desc_str(podm_->listStacks());
								break;
							case PodmanCmd::DEPLOY_STACK:
								DEBUGLOG("执行deployStack命令");
								nodeCmdReply.set_desc_str(podm_->deployStack(nodeCmd.paras(), nodeCmd.paras_1()));
								break;
							case PodmanCmd::REMOVE_STACK:
								DEBUGLOG("执行removeStack命令");
								nodeCmdReply.set_desc_str(podm_->removeStack(nodeCmd.paras()));
								break;
							case PodmanCmd::INSPECT_STACK:
								DEBUGLOG("执行inspectStack命令");
								nodeCmdReply.set_desc_str(podm_->inspectStack(nodeCmd.paras()));
								break;
							case PodmanCmd::UPDATE_STACK:
								DEBUGLOG("执行updateStack命令");
								nodeCmdReply.set_desc_str(podm_->updateStack(nodeCmd.paras(), nodeCmd.paras_1()));
								break;
							case PodmanCmd::START_STACK:
								DEBUGLOG("执行startStack命令");
								nodeCmdReply.set_desc_str(podm_->startStack(nodeCmd.paras()));
								break;
							case PodmanCmd::STOP_STACK:
								DEBUGLOG("执行stopStack命令");
								nodeCmdReply.set_desc_str(podm_->stopStack(nodeCmd.paras()));
								break;
							case PodmanCmd::RESTART_STACK:
								DEBUGLOG("执行restartStack命令");
								nodeCmdReply.set_desc_str(podm_->restartStack(nodeCmd.paras()));
								break;
							case PodmanCmd::PAUSE_STACK:
								DEBUGLOG("执行pauseStack命令");
								nodeCmdReply.set_desc_str(podm_->pauseStack(nodeCmd.paras()));
								break;
							case PodmanCmd::UNPAUSE_STACK:
								DEBUGLOG("执行unpauseStack命令");
								nodeCmdReply.set_desc_str(podm_->unpauseStack(nodeCmd.paras()));
								break;
							case PodmanCmd::SCALE_STACK:
								DEBUGLOG("执行scaleStack命令");
								nodeCmdReply.set_desc_str(podm_->scaleStack(nodeCmd.paras(), PodmanManager::stringToJson(nodeCmd.paras_2())));
								break;
							case PodmanCmd::GET_STACK_LOGS:
								DEBUGLOG("执行getStackLogs命令");
								nodeCmdReply.set_desc_str(podm_->getStackLogs(nodeCmd.paras()));
								break;
							case PodmanCmd::LIST_REGISTRIES:
								DEBUGLOG("执行listRegistries命令");
								nodeCmdReply.set_desc_str(podm_->listRegistries());
								break;
							case PodmanCmd::ADD_REGISTRY:
								DEBUGLOG("执行addRegistry命令");
								nodeCmdReply.set_desc_str(podm_->addRegistry(nodeCmd.paras()));
								break;
							case PodmanCmd::REMOVE_REGISTRY:
								DEBUGLOG("执行removeRegistry命令");
								nodeCmdReply.set_desc_str(podm_->removeRegistry(nodeCmd.paras()));
								break;
							case PodmanCmd::LOGIN_REGISTRY:
								DEBUGLOG("执行loginRegistry命令");
								nodeCmdReply.set_desc_str(podm_->loginRegistry(nodeCmd.paras(), nodeCmd.paras_1(), nodeCmd.paras_2()));
								break;
							default:
								ERRORLOG("未知的Podman命令: {}, 参数:{}, 参数1:{}, 参数2:{}", nodeCmd.podman_cmd(), nodeCmd.paras(), nodeCmd.paras_1(), nodeCmd.paras_2());
								nodeCmdReply.set_status(CommStatus::INVALID);
								nodeCmdReply.set_desc_str("未知的Podman命令");
								break;
							}
							nodeCmdReply.set_status(CommStatus::SUCCESS);
							INFOLOG("Podman命令执行成功");
						}
						catch (const std::exception& e) {
							ERRORLOG("Podman命令执行出错: {}", e.what());
							nodeCmdReply.set_status(CommStatus::INVALID);
							nodeCmdReply.set_desc_str(std::string("Podman命令执行失败: ") + e.what());
						}
					}
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					break;
				default:
					utility::msg::mainCmdReplyQueueOut_.enqueue(nodeCmdReply);
					ERRORLOG("unknown cmd:{} paras:{}", nodeCmd.cmd(), nodeCmd.paras());
					break;
				}
			}
			catch (...) {
				ERRORLOG("unknown cmdType:{} cmd:{} paras:{}", to_string(nodeCmd.cmd_type()), nodeCmd.cmd(), nodeCmd.paras());
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(5));
		}
		else {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}

	}

}

void NodeCmdWorker::setLsPluginDesc(NodeCmdWorker* nodeCmdWorker, cn::seisys::dds::NodeCmdReply& nodeCmdReply)
{
	std::map<long, cppmicroservices::Bundle> bundles;
	for (auto& b : nodeCmdWorker->framework->GetBundleContext().GetBundles()) {
		bundles.insert(std::make_pair(b.GetBundleId(), b));
	}
	string json = "[";
	for (auto& bundle : bundles) {
		INFOLOG("plugin id:{} SymbolicName:{} state:{}", to_string(bundle.first), bundle.second.GetSymbolicName(), to_string(bundle.second.GetState()));
		json += "{\"id\":" + to_string(bundle.first);
		json += ",\"SymbolicName\":\"" + bundle.second.GetSymbolicName() + "\"";
		json += ",\"State\":\"" + to_string(bundle.second.GetState()) + "\"";
		json += "},";
	}
	if (json.size() > 1)
		json = json.substr(0, json.size() - 1) + "]";
	else
		json += "]";
	DEBUGLOG("json:{}", json);
	nodeCmdReply.set_desc(json);
}

void NodeCmdWorker::setLsAppDesc(std::vector<ProcessAttribute>& instances, cn::seisys::dds::NodeCmdReply& nodeCmdReply, string paras1, string paras2)
{
	INFOLOG("para process json in");
	int count = (int)instances.size();
	INFOLOG("instances size:{}", count);
	string json = "[";
	//for (auto& p : *instances) {
	for (int i = 0; i < count; i++) {
		//for (auto &p : m_instances) {
		//INFOLOG("get instance:{}", i);
		auto& p = instances[i];
		INFOLOG("process: pid:{},status:{},uuid:{},launch_cmd:{},paras1:{},paras2:{}", p.instance.id(), p.exception_flag, p.uuid, p.launch_cmd, p.work_path, "");
		json += "{\"id\":" + to_string(p.instance.id());
		json += ",\"state\":\"" + to_string(!p.exception_flag) + "\"";
		json += ",\"uuid\":\"" + p.uuid + "\"";
		json += ",\"launch_cmd\":\"" + p.launch_cmd + "\"";
		json += ",\"paras1\":\"" + p.work_path + "\"";
		json += ",\"paras2\":\" \"";
		json += "},";
	}
	if (json.size() > 2)
		json = json.substr(0, json.size() - 1) + "]";
	else
		json += "]";
	DEBUGLOG("json:{}", json);
	nodeCmdReply.set_desc(json);
	INFOLOG("para process json out");
}

bool NodeCmdWorker::start_plugin(std::string pluginName) {
	try {
		auto bundle = this->get_bundle(pluginName);
		if (bundle) {

			/* starting an already started bundle does nothing.
			   There is no harm in doing it. */
			if (bundle.GetState() == cppmicroservices::Bundle::STATE_ACTIVE) {
				INFOLOG("Info: bundle already active");
			}
			bundle.Start();
			return true;

		}
		else {
			ERRORLOG("Error: unknown id or symbolic name with dll:{}", pluginName);
		}
	}
	catch (const std::exception& e) {
		ERRORLOG("start plugin:{} error:{}", pluginName, e.what());
	}
	return false;
}
bool NodeCmdWorker::stop_plugin(std::string pluginName) {
	try {
		auto bundle = this->get_bundle(pluginName);
		if (bundle) {
			bundle.Stop();
			std::this_thread::sleep_for(std::chrono::milliseconds(2000));
			//if (bundle.GetBundleId() == 0) {
			//    return true;
			//}
			return true;
		}
		else {
			ERRORLOG("Error: unknown id or symbolic name wiht  dll:{}", pluginName);
		}
	}
	catch (const std::exception& e) {
		ERRORLOG("stop plugin:{} error:{}", pluginName, e.what());
	}
	return false;
}

string NodeCmdWorker::combUuid(std::string paras1, std::string cmd, std::string paras, std::string paras2)
{
	string uuid = paras1 + " " + cmd + " " + paras + " " + paras2;

	return utility::string::trim(uuid);
}


