#include "Poco/Process.h"
#include "Poco/PipeStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/JSON/Object.h"
#include "Poco/JSON/Array.h"
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <unordered_map>
#include <chrono>

#include "../utils/log/BaseLog.h"

#ifdef _WIN32
#define DEFAULT_SHELL "cmd.exe"
#define SHELL_ARGS "/c"
#else
#define DEFAULT_SHELL "/bin/bash"
#define SHELL_ARGS "-c"
#endif

const int MAX_OUTPUT_LINES = 1000;
const std::chrono::seconds OUTPUT_TIMEOUT(5); // 5 秒超时
const std::chrono::seconds GET_OUTPUT_TIMEOUT(10); // 获取输出的超时时间


class CommandExecutor {
public:
	std::unordered_map<std::string, std::deque<std::string>> output_lines;
	std::unordered_map<std::string, std::mutex> queue_mutexes;
	std::unordered_map<std::string, std::condition_variable> queue_conds;

	static std::unordered_map<std::string, Poco::ProcessHandle*> process_handles;
	static std::unordered_map<std::string, bool> process_running;

	std::unordered_map<std::string, std::vector<std::string>> recentOutputs;


public:
	void execute_command(const std::string& command) {

		INFOLOG("exec process 1");

		process_running[command] = true;

		Poco::Process::Args args;
		std::string command_line = DEFAULT_SHELL;
		args.push_back(SHELL_ARGS);
		args.push_back(command);

		Poco::Pipe outPipe;
		Poco::Process::Env env;
		Poco::ProcessHandle ph = Poco::Process::launch(command_line, args, 0, &outPipe, 0, env);

		INFOLOG("exec process 2");

		recentOutputs[command] = {};

		std::deque<std::string> recentLinesDeque;

		process_handles[command] = &ph; // Store the process handle

		INFOLOG("exec process uuid:{} ph:{}", command, ph.id());
		auto startTime = std::chrono::steady_clock::now(); // Add start time
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
						INFOLOG("exec process :{},result:{}", command, result);
						{
							recentLinesDeque.push_back(result);
							if (recentLinesDeque.size() > MAX_OUTPUT_LINES) {
								recentLinesDeque.pop_front();
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
			//outPipe.close();

			});

		t.detach();  // Detach the thread to let it run independently

		while (true) {
			if (ph.tryWait() == -1) {	//如果进程还在运行，就尝试等待5分钟		
				auto currentTime = std::chrono::steady_clock::now(); // Add current time
				auto elapsedTime = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count(); // Calculate elapsed time in seconds
				if (elapsedTime >= 5) { // Check if elapsed time is greater than or equal to 30 seconds
					INFOLOG("exec process timeout,return");
					recentOutputs[command] = { recentLinesDeque.begin(), recentLinesDeque.end() };
					break;
				}
			}
			else {
				INFOLOG("exec process tryWait return");
				//t.join();
				recentOutputs[command] = { recentLinesDeque.begin(), recentLinesDeque.end() };
				std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				break;
			}
		}
	}

	void outputLineReceived(const std::string& line, const std::string& command) {
		std::lock_guard<std::mutex> lock(queue_mutexes[command]);
		output_lines[command].push_back(line);
		if (output_lines[command].size() > MAX_OUTPUT_LINES) {
			output_lines[command].pop_front();
		}
		queue_conds[command].notify_one();
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
		INFOLOG("exec process getRecentOutput in with uuid:{}", uuid);
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

	bool is_command_running(const std::string& command) {
		//std::lock_guard<std::mutex> lock(queue_mutexes[command]);
		return process_running[command];
	}

	void kill_command(const std::string& command) {
		INFOLOG("exec kill process :{}", command);
		auto it = process_handles.find(command);
		if (it != process_handles.end()) {
			//*(it->second)->kill(); // Kill the process
			INFOLOG("exec kill process 1:{}", command);
			std::thread killer([&]() {
				Poco::Process::kill(*(it->second));
				Poco::Process::wait(*(it->second));
				});
			killer.detach();

			INFOLOG("exec kill process 2:{}", command);
			process_handles.erase(it); // Remove the process handle
			process_running[command] = false;
			INFOLOG("exec kill process 3:{}", command);
			// Clear related data
			output_lines[command].clear();
			queue_conds[command].notify_all();
		}
	}




};

class CommandManager {
private:
	std::unordered_map<std::string, CommandExecutor*> executors;

public:
	void execute_command(const std::string& command) {
		INFOLOG("exec Command:{} begin", command);
		auto it = executors.find(command);
		if (it != executors.end()) {
			INFOLOG("exec Command:{} is already in execution", command);
			//executors[command]->kill_command(command);
			executors.erase(it);
			//delete executors[command];
			//executors[command]->execute_command(command);
		}

		{
			auto executor = new CommandExecutor();
			INFOLOG("execute_command process 1");
			executors[command] = std::move(executor);
			INFOLOG("execute_command process 2");
			executors[command]->execute_command(command);
		}
	}

	std::string get_output(const std::string& command) {
		auto it = executors.find(command);
		if (it != executors.end()) {
			return it->second->getRecentOutput(command);
		}
		return "[]";
	}

	bool is_command_running(const std::string& command) {
		auto it = executors.find(command);
		if (it != executors.end()) {
			return it->second->is_command_running(command);
		}
		return false;
	}

	void kill_command(const std::string& command) {
		auto it = executors.find(command);
		if (it != executors.end()) {
			it->second->kill_command(command);
			executors.erase(it);
		}
	}

	std::string getAllProcesses() {
		Poco::JSON::Array processesJson;

		for (const auto& pair : CommandExecutor::process_handles) {
			const std::string& command = pair.first;
			Poco::JSON::Object processJson;
			bool is_running = false;
			if (pair.second != nullptr) {
				INFOLOG("execute_command process:{},pid:{} ", command, pair.second->id());
				is_running = Poco::Process::isRunning(*pair.second); //CommandExecutor::process_running[command];
				CommandExecutor::process_running[command] = is_running;

			}
			else {
				INFOLOG("execute_command process:{},pid:{} ", command, -1);
				is_running = false;
				CommandExecutor::process_running[command] = is_running;
			}

			processJson.set("command", command);
			processJson.set("is_running", is_running);


			processesJson.add(processJson);
		}

		std::ostringstream oss;
		processesJson.stringify(oss);
		return oss.str();
	}

};

/*
int main() {
	CommandManager manager;
	std::vector<std::string> commands = {"command1 arg1 arg2", "command2", "command3 arg1"};

	for (const auto& command : commands) {
		manager.execute_command(command);
	}

	// 获取命令输出和状态
	for (const auto& command : commands) {
		std::string output = manager.get_output(command);
		std::cout << "Command: " << command << std::endl;
		std::cout << "Running: " << manager.is_command_running(command) << std::endl;
		std::cout << "Output: " << output << std::endl;
		std::cout << std::endl;
	}

	return 0;
}
*/