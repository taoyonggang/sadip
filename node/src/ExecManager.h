#include <Poco/Process.h>
#include <Poco/PipeStream.h>
#include <Poco/StreamCopier.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <mutex>
#include <thread>
#include <deque>
#include <unordered_map>

#include "../utils/log/BaseLog.h"

class ExecManager {
private:
	std::unordered_map<std::string, Poco::ProcessHandle*> processes;
	//std::unordered_map<std::string, Poco::Pipe> pipes;
	std::unordered_map<std::string, std::vector<std::string>> recentOutputs;
	std::mutex mtx;

public:
	// Start a new process and return its UUID
	bool startProcess(const std::string& command, const Poco::Process::Args& args, const std::string& uuid, int recentLines = 2000) {
		std::lock_guard<std::mutex> lock(mtx);
		Poco::Process::Args argss;

		Poco::Process::Env env;
		
		argss.push_back("-c");
		argss.push_back(command);
		for (auto arg : args)		{
			argss.push_back(arg);
		}
		//argss.push_back(args.);
		Poco::Pipe outPipe;
#if defined(unix) || defined(__unix) || defined(__unix__)
		Poco::ProcessHandle ph = Poco::Process::launch("/bin/bash", argss, 0, &outPipe, 0, env);
#else defined(windows)     
		Poco::ProcessHandle ph = Poco::Process::launch("cmd.exe", argss, 0, &outPipe, 0, env);
#endif

		INFOLOG("exec process uuid:{} ph:{}", uuid, ph.id());
		processes[uuid] = &ph;
		//pipes[uuid] = outPipe;
		recentOutputs[uuid] = {};

		std::deque<std::string> recentLinesDeque;

		auto startTime = std::chrono::steady_clock::now(); // Add start time
		// Create a separate thread to handle the command output
		std::thread t([&]() {
			Poco::PipeInputStream istr(outPipe);
			std::string result;
			
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			while (true) {
				//DEBUGLOG("exec process is running ...");
				if (istr.good() && istr.peek() != EOF)
				{
					{
						Poco::StreamCopier::copyToString(istr, result);
						INFOLOG("exec process :{},result:{}", uuid, result);
						{
							recentLinesDeque.push_back(result);
							if (recentLinesDeque.size() > recentLines) {
								//    //recentLinesDeque.pop_front();
								//recentOutputs[uuid] = { recentLinesDeque.begin(), recentLinesDeque.end() };
								INFOLOG("exec process too more result,return");
								break;
							}

						}
						result.clear();
					}
				}
				auto currentTime = std::chrono::steady_clock::now(); // Add current time
				auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count(); // Calculate elapsed time in seconds
				if (elapsedTime >= 5) { // Check if elapsed time is greater than or equal to 30 seconds
					INFOLOG("exec process timeout,return");
					//recentOutputs[uuid] = { recentLinesDeque.begin(), recentLinesDeque.end() };
					break;
				}
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}			

			});

		t.detach();  // Detach the thread to let it run independently

		while (true) {
			if (ph.tryWait() == -1) {	//如果进程还在运行，就尝试等待5分钟		
				auto currentTime = std::chrono::steady_clock::now(); // Add current time
				auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count(); // Calculate elapsed time in seconds
				if (elapsedTime >= 5) { // Check if elapsed time is greater than or equal to 30 seconds
					INFOLOG("exec process timeout,return");
					recentOutputs[uuid] = { recentLinesDeque.begin(), recentLinesDeque.end() };
					break;
				}
			}
			else {
				INFOLOG("exec process tryWait return");
				//t.join();
				recentOutputs[uuid] = { recentLinesDeque.begin(), recentLinesDeque.end() };
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				break;
			}
		}

		return true;
	}

	// Stop and remove a process by its UUID
	//void stopAndRemoveProcess(const std::string& uuid) {
	//    std::lock_guard<std::mutex> lock(mtx);
	//    if (processes.count(uuid) > 0) {
	//        Poco::Process::kill(*processes[uuid]);
	//        delete processes[uuid];
	//        processes.erase(uuid);
	//        pipes.erase(uuid);
	//        recentOutputs.erase(uuid);
	//    }
	//}
	// Stop a process by its UUID
	void stopProcess(const std::string& uuid) {
		//std::lock_guard<std::mutex> lock(mtx);
		INFOLOG("exec process stopProcess in");
		if (processes.count(uuid) > 0) {
			try {
				if (Poco::Process::isRunning(*(processes[uuid]))) {
					Poco::Process::kill(*processes[uuid]);
				}
			}
			catch (Poco::SystemException& e) {
				// 处理异常
				INFOLOG("exec process stopProcess error:{}", e.what());
			}
		}
		INFOLOG("exec process stopProcess out");
	}

	// Remove a process by its UUID
	void removeProcess(const std::string& uuid) {
		INFOLOG("exec process removeProcess in");
		std::lock_guard<std::mutex> lock(mtx);
		if (processes.count(uuid) > 0) {
			processes.erase(uuid);
			//pipes.erase(uuid);
			recentOutputs.erase(uuid);
		}
		INFOLOG("exec process removeProcess out");
	}

	// Get the most recent output of a process
	std::string getRecentOutput() {
		//std::lock_guard<std::mutex> lock(mtx);
		Poco::JSON::Array::Ptr jsonArray = new Poco::JSON::Array();
		for (const auto& line : recentOutputs) {
			jsonArray->add(line);
		}
		std::stringstream ss;
		jsonArray->stringify(ss);
		return ss.str();
	}

	std::string getRecentOutput(const std::string& uuid) {
		//std::lock_guard<std::mutex> lock(mtx);
		INFOLOG("exec process getRecentOutput in with uuid:{}",uuid);
		if (recentOutputs.count(uuid) > 0) {
			Poco::JSON::Array::Ptr jsonArray = new Poco::JSON::Array();
			INFOLOG("exec process getRecentOutput line size:{}", recentOutputs.size());
			for (const auto& line : recentOutputs[uuid]) {
				INFOLOG("exec process getRecentOutput line:{}", line);
				jsonArray->add(line);
			}
			INFOLOG("exec process getRecentOutput json to string");
			std::stringstream ss;
			jsonArray->stringify(ss);
			INFOLOG("exec process getRecentOutput json to string:{}", ss.str());
			return ss.str();
		}
		INFOLOG("exec process getRecentOutput out");
		return "[]";
	}

	std::string getAllProcesses() {
		INFOLOG("exec process getAllProcesses in");
		//std::lock_guard<std::mutex> lock(mtx);
		Poco::JSON::Array::Ptr jsonArray = new Poco::JSON::Array();
		for (const auto& process : processes) {
			INFOLOG("exec process is running:{}", process.first);
			try {
				//if (processes.count(process.first) > 0) 
				{
					//if (process.second != nullptr && Poco::Process::isRunning(*processes[process.first])) 
					{
						Poco::JSON::Object::Ptr jsonObject = new Poco::JSON::Object();
						INFOLOG("exec process uuid:{}", process.first);
						jsonObject->set("uuid", process.first);
						//INFOLOG("exec process pid:{}", process.second->id());
						//jsonObject->set("pid", process.second->id());            
						jsonArray->add(jsonObject);
					}
				}
				//else {
				//	INFOLOG("exec process uuid:{} is not running", process.first);
					//removeProcess(process.first);
				//}
			}
			catch (Poco::SystemException& e) {
				// 处理异常
				INFOLOG("exec process stopProcess error:{}", e.what());
			}
		}
		std::stringstream ss;
		jsonArray->stringify(ss);
		INFOLOG("exec process getAllProcesses out");
		return ss.str();
	}
};
