#include "ProcessManager.h"
#include <iostream>
#include "../../utils/log/BaseLog.h"
#include "../../utils/split.h"

//#if defined(unix) || defined(__unix) || defined(__unix__)
//#include <stdio.h>
//#include <sys/stat.h>
////#include <io.h>
//#endif

#ifdef _WIN32
#include <windows.h>
#include <pdh.h>
#include <psapi.h>
#pragma comment(lib, "pdh.lib")
#elif __linux__
#include <fstream>
#include <unistd.h>
#include <sys/times.h>
#endif

using namespace boost::process;

boost::asio::io_service ios;


ProcessManager::ProcessManager() {

}

ProcessManager::~ProcessManager() = default;

/**
@fn init

@brief 初始化进程参数.

@param[in] void.

@param[out] void.

@return void.
 **/
bool ProcessManager::add_process(std::string uuid, std::string runPath, std::string runFile, bool keepAlive) {
	INFOLOG("add process with cmd:{}, paras:{}", uuid, runFile);
	bool find = false;

	int count = (int)this->m_instances.size();
	INFOLOG("m_instances size:{}", count);
	for (int i = 0; i < count; i++) {
		auto& p = this->m_instances[i];
		if (p.uuid == uuid) {
			find = true;
			ERRORLOG("process:{} is exist,add process manager failed", uuid);
			return true;
		}
	}
	//count = (int)this->m_instances.size();
	//INFOLOG("m_instances size:{}", count);
#if defined(unix) || defined(__unix) || defined(__unix__)
	const std::string exe = utility::string::cutFisrtLeftStr(uuid, " ");
	//const std::string cmd = "chmod +x " + exe;

	//bp::child(cmd, ios);
	int stat = chmod((runPath+"/" + exe).c_str(), S_IEXEC);
	if (stat) {
		INFOLOG("run chmod +x {}:success", exe);
	}
	else
	{
		INFOLOG("run chmod +x {}:failed", exe);
	}
#endif
	auto e = boost::this_process::environment();
	bp::environment e_ = e;
	//e_["DEBUG"] = "hello world.";

	ProcessAttribute p{};
	p.env = e_;
	p.uuid = uuid;
	p.launch_cmd = runFile;
	p.work_path = runPath;

	p.except_runnig = keepAlive;

	this->m_instances.push_back(std::move(p));
	//count = (int)this->m_instances.size();
	//INFOLOG("m_instances size:{}", count);
	INFOLOG("process:{},cmd:{},work_path:{} add process manager success", uuid, p.launch_cmd, p.work_path);

	return true;
}


/**
@fn remove_process

@brief 去除被管理的进程.

@param[in] std::string .

@param[out] boold.

@return void.
 **/
bool ProcessManager::remove_process(std::string uuid) {
	INFOLOG("remove process with cmd:{}", uuid);
	kill_process(uuid);

	bool find = false;

	std::vector<ProcessAttribute>::iterator itr = this->m_instances.begin();
	while (itr != this->m_instances.end())
	{
		if (itr->uuid == uuid)
		{
			itr = this->m_instances.erase(itr);//删除元素,返回值指向已删除元素的下一个位置
		}
		else
		{
			++itr;
		}
	}

	return true;
}

/**
@fn start_process

@brief 在打开的终端窗口中启动指定进程.

@param[in] void.

@param[out] void.

@return void.
 **/
void ProcessManager::start_all_process() {
	INFOLOG("start all process ");
	std::string cmd;
	try {
		int count = (int)this->m_instances.size();
		for (int i = 0; i < count; i++) {
			auto& p = m_instances[i];
			//const std::string cmd = "terminator --title=" + p.uuid + " -e \"" + p.launch_cmd +"\" -x bash";
			cmd = p.launch_cmd;
			if (p.env.empty()) {
				//bp::start_dir(p.work_path);
				p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios);
			}
			else {
				//bp::start_dir(p.work_path);
				p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios, p.env);
			}
			//p.except_runnig = true;
		}
	}
	catch (std::exception& e) {
		ERRORLOG("start process exception:{},cmd:{}", e.what(), cmd);
	}
}

/**
@fn start_process

@brief 在打开的终端窗口中启动指定进程.

@param[in] void.

@param[out] void.

@return void.
 **/
bool ProcessManager::start_process(std::string uuid) {
	INFOLOG("start process with cmd:{}", uuid);
	std::string cmd;
	try {
		int count = (int)this->m_instances.size();
		for (int i = 0; i < count; i++) {
			auto& p = m_instances[i];
			INFOLOG("find process with uuid:{}, state:{}，work_path:{}", p.uuid.c_str(), p.instance.running(), p.work_path);
			if (p.uuid == uuid && !p.instance.running()) {
				INFOLOG("The process with uuid:{}, and it is not runnig,now start", p.uuid.c_str());
				//const std::string cmd = "terminator --title=" + p.uuid + " -e \"" + p.launch_cmd +"\" -x bash";
				p.latest_start_time = get_current_sec();
				cmd = p.launch_cmd;
				//boost::asio::io_context io_context;
				if (p.env.empty()) {
					//bp::start_dir(p.work_path);
					p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios);
				}
				else {
					//bp::start_dir(p.work_path);
					p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios, p.env);
				}
				//io_context.run();
				//p.instance.wait();
				// 等待 10 秒
				if (p.instance.wait_for(std::chrono::seconds(10)))
				{
					// 进程在超时前结束
					INFOLOG("Process finished with exit code:{}", p.instance.exit_code());
				}
				p.except_runnig = true;
				return true;
			}
		}
	}
	catch (std::exception& e) {
		ERRORLOG("start process exception:{},cmd:{}", e.what(), cmd);
	}
	catch (...) {
		ERRORLOG("start process exception:{},cmd:{}", "unknown error", cmd);
	}
	return false;
}

/**
@fn    reboot_all_process

@brief 重启异常退出的进程.

@param[in] void.

@param[out] void.

@return bool.
 **/
bool ProcessManager::reboot_all_process() {
	std::string cmd;

	int count = (int)this->m_instances.size();
	for (int i = 0; i < count; i++) {

		auto& p = m_instances[i];
		try {
			if (p.exception_flag) {
				//const std::string cmd = "terminator --title=" + p.uuid + " -e \"" + p.launch_cmd + "\" -x bash";
				cmd = p.launch_cmd;
				if (p.env.empty()) {
					//bp::start_dir(p.work_path);
					p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios);
				}
				else {
					//bp::start_dir(p.work_path);
					p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios, p.env);
				}
				//p.instance.wait();
				if (p.instance.wait_for(std::chrono::seconds(10)))
				{
					// 进程在超时前结束
					INFOLOG("Process finished with exit code:{}", p.instance.exit_code());
				}
				p.exception_flag = false;
				//p.except_runnig = true;
				p.reboot_count++;
			}
		}
		catch (std::exception& e) {
			ERRORLOG("start process exception:{},cmd:{},work_path:{}", e.what(), cmd, p.work_path);
			return false;
		}
		catch (...) {
			ERRORLOG("start process exception:{},cmd:{},work_path:{}", "unknown error", cmd, p.work_path);
			return false;
		}

	}


	return true;
}

/**
@fn    reboot_process

@brief 重启异常退出的进程.

@param[in] void.

@param[out] void.

@return bool.
 **/
bool ProcessManager::reboot_process(std::string uuid) {
	std::string cmd;
	try {
		int count = (int)this->m_instances.size();
		for (int i = 0; i < count; i++) {
			auto& p = m_instances[i];
			if (p.uuid == uuid) {
				if (p.exception_flag) {
					//const std::string cmd = "terminator --title=" + p.uuid + " -e \"" + p.launch_cmd + "\" -x bash";
					cmd = p.launch_cmd;
					if (p.env.empty()) {
						//bp::start_dir(p.work_path);
						p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios);
					}
					else {
						//bp::start_dir(p.work_path);
						p.instance = bp::child(cmd, bp::start_dir(p.work_path), ios, p.env);
					}
					//p.instance.wait();
					if (p.instance.wait_for(std::chrono::seconds(10)))
					{
						// 进程在超时前结束
						INFOLOG("Process finished with exit code:{}", p.instance.exit_code());
					}
					//p.exception_flag = false;
					//p.except_runnig = true;
					p.reboot_count++;
				}
			}
		}
	}
	catch (std::exception& e) {
		ERRORLOG("start process exception:{},cmd:{}", e.what(), cmd);
		return false;
	}
	catch (...) {
		ERRORLOG("start process exception:{},cmd:{}", "unknown error", cmd);
		return false;
	}

	return true;
}

/**
@fn kill_process

@brief 根据唯一编号杀死指定进程.

@param[in] uuid:进程的唯一编号.

@param[out] void.

@return void.
 **/
bool ProcessManager::kill_process(const std::string& uuid) {
	INFOLOG("kill process with uuid:{}", uuid);
	try {
		for (auto& p : m_instances) {
			if (p.uuid == uuid) {
				pid_t pid = p.instance.id();
				INFOLOG("Attempting to terminate process: pid:{}, work path:{}", pid, p.work_path);

				// 首先尝试温和地终止进程
				p.instance.terminate();

				// 等待进程结束，最多等待15秒
				if (p.instance.wait_for(std::chrono::seconds(15)))
				{
					INFOLOG("Process terminated gracefully with exit code:{}", p.instance.exit_code());
				}
				else
				{
					WARNLOG("Process did not terminate gracefully, forcing termination");
					// 如果进程没有正常结束，则强制结束
#if defined(_WIN32) || defined(_WIN64)
	// Windows 特定代码
					HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
					if (hProcess != NULL)
					{
						TerminateProcess(hProcess, 1);
						CloseHandle(hProcess);
					}
#else
	// Unix 特定代码
					kill(pid, SIGKILL);
#endif
			}

				// 再次等待，确保进程完全结束
				if (p.instance.wait_for(std::chrono::seconds(5)))
				{
					INFOLOG("Process fully terminated");
				}
				else
				{
					ERRORLOG("Failed to fully terminate process");
				}

				// 清理资源
				p.except_runnig = false;
				p.reboot_count = 0;

				// 从列表中移除已终止的进程
				m_instances.erase(std::remove_if(m_instances.begin(), m_instances.end(),
					[&uuid](const auto& proc) { return proc.uuid == uuid; }),
					m_instances.end());
		}
	}
}
	catch (std::exception& e) {
		ERRORLOG("Exception while killing process: {}, uuid:{}", e.what(), uuid);
		return false;
	}
	catch (...) {
		ERRORLOG("Unknown exception while killing process, uuid:{}", uuid);
		return false;
	}

	return true;
}

/**
@fn kill_all_process

@brief 根据唯一编号杀死指定进程.

@param[in] uuid:进程的唯一编号.

@param[out] void.

@return void.
 **/
bool ProcessManager::kill_all_process() {
	INFOLOG("Attempting to kill all processes");
	try {
		for (auto it = m_instances.begin(); it != m_instances.end(); ) {
			auto& p = *it;
			pid_t pid = p.instance.id();
			INFOLOG("Attempting to terminate process: pid:{}, work path:{}", pid, p.work_path);

			// 首先尝试温和地终止进程
			p.instance.terminate();

			// 等待进程结束，最多等待15秒
			if (p.instance.wait_for(std::chrono::seconds(15)))
			{
				INFOLOG("Process terminated gracefully with exit code:{}", p.instance.exit_code());
			}
			else
			{
				WARNLOG("Process did not terminate gracefully, forcing termination");
				// 如果进程没有正常结束，则强制结束
#if defined(_WIN32) || defined(_WIN64)
	// Windows 特定代码
				HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
				if (hProcess != NULL)
				{
					TerminateProcess(hProcess, 1);
					CloseHandle(hProcess);
				}
#else
	// Unix 特定代码
				kill(pid, SIGKILL);
#endif
			}

			// 再次等待，确保进程完全结束
			if (p.instance.wait_for(std::chrono::seconds(5)))
			{
				INFOLOG("Process fully terminated: pid:{}", pid);
			}
			else
			{
				ERRORLOG("Failed to fully terminate process: pid:{}", pid);
			}

			// 清理资源
			p.except_runnig = false;

			// 从列表中移除已终止的进程
			it = m_instances.erase(it);
		}
	}
	catch (std::exception& e) {
		ERRORLOG("Exception while killing all processes: {}", e.what());
		return false;
	}
	catch (...) {
		ERRORLOG("Unknown exception while killing all processes");
		return false;
	}

	INFOLOG("All processes have been terminated");
	return true;
}

/**
@fn listener

@brief 非阻塞监听进程状态.

@param[in] void.

@param[out] void.

@return bool.
 **/
bool ProcessManager::listener() {
	INFOLOG("listener");
	ios.poll();
	int count = (int)this->m_instances.size();
	INFOLOG("listener pool,m_instances size:{}", this->m_instances.size());

	bool exception = false;
	for (int i = 0; i < count; i++) {
		//for (auto &p : m_instances) {
		auto& p = m_instances[i];
		INFOLOG("check pid:{} work_path:{} uuid:{} exception_flag:{} exception_running:{} reboot num:{} state...", p.instance.id(), p.work_path, p.uuid, p.exception_flag, p.except_runnig, (int)p.reboot_count);
		if (!p.instance.running() && p.except_runnig) {
			INFOLOG("process pid:{} is not running, work_path:{} exception_flag:{} exception_running:{}", p.instance.id(), p.work_path, p.exception_flag, p.except_runnig);
			exception = true;
			p.exception_flag = true;
			INFOLOG("The process has exited:{},reboot num={},pid:{},work_path:{},exception_flag:{},exception_running:{}", p.uuid, (int)p.reboot_count, p.instance.id(), p.work_path, p.exception_flag, p.except_runnig);
			if (p.except_runnig) {
				INFOLOG("The process uuid:{}, pid:{},work_path:{},exception_flag:{},exception_running:{},reboot_num:{}, except runnig,but now exit, run it again", p.uuid, p.instance.id(), p.work_path, p.exception_flag, p.except_runnig, (int)p.reboot_count);
				start_process(p.uuid);
				p.reboot_count++;
			}

		}
		else {
			exception = false;
			p.exception_flag = false;
			p.latest_stop_time = get_current_sec();
			p.cpu_usage = getCpuUsage(p.instance.id());
			p.mem_usage = getMemoryUsage(p.instance.id());
		}
	}
	INFOLOG("check process finished.");
	return exception;
}

/**
@fn ls_process

@brief 根据唯一进程号,获取进程状态.

@param[in] uuid:进程的唯一编号.

@param[out] std::vector<std::string>.

@return std::vector<std::string>.
 **/
bool ProcessManager::ls_process() {
	INFOLOG("ls process");
	return(this->listener());
	//return &this->m_instances;
}


double ProcessManager::getCpuUsage(int pid) {
	double cpuUsage = 0.0;
#ifdef _WIN32
	// Windows platform
	PDH_HQUERY cpuQuery;
	PDH_HCOUNTER cpuTotal;
	PDH_FMT_COUNTERVALUE counterVal;

	PdhOpenQuery(NULL, NULL, &cpuQuery);
	std::string counterPath = "\\Process(" + std::to_string(pid) + ")\\% Processor Time";
	PdhAddEnglishCounter(cpuQuery, counterPath.c_str(), NULL, &cpuTotal);
	PdhCollectQueryData(cpuQuery);

	PdhCollectQueryData(cpuQuery);
	PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	cpuUsage = counterVal.doubleValue;
	PdhCloseQuery(cpuQuery);
#elif __linux__
	// Linux platform
	long ticks_per_second = sysconf(_SC_CLK_TCK);
	double total_time = 0;
	std::ifstream proc_stat("/proc/" + std::to_string(pid) + "/stat");
	if (proc_stat.is_open()) {
		std::string value;
		for (int i = 0; i < 13; ++i) proc_stat >> value;  // Skip to utime
		long utime, stime;
		proc_stat >> utime >> stime;  // Get user and system CPU time
		total_time = (double)(utime + stime) / ticks_per_second;
}
	cpuUsage = total_time;
#endif
	DEBUGLOG("cpuUsage:{}", cpuUsage);
	return cpuUsage;
}


double ProcessManager::getMemoryUsage(int pid) {
	double memoryUsage = 0.0;
#ifdef _WIN32
	// Windows platform
	HANDLE process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
	if (process) {
		PROCESS_MEMORY_COUNTERS_EX pmc;
		if (GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
			memoryUsage = pmc.WorkingSetSize / (1024.0 * 1024.0);
		}
		CloseHandle(process);
	}
#elif __linux__
	// Linux platform
	std::string line;
	std::ifstream stat_file("/proc/" + std::to_string(pid) + "/stat");

	if (getline(stat_file, line)) {
		size_t pos = line.find_last_of(' ');
		if (pos != std::string::npos) {
			long int utime = std::stol(line.substr(0, pos));
			long int stime = std::stol(line.substr(pos + 1));
			long int total_time = utime + stime;
			long int uptime = sysconf(_SC_CLK_TCK);
			memoryUsage = (total_time / (double)uptime) * 100;
	}
}
#endif
	DEBUGLOG("memUsage:{}", memoryUsage);
	return memoryUsage;
}