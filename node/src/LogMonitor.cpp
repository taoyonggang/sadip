#include "LogMonitor.h"
#include <Poco/RunnableAdapter.h>
#include <Poco/FileStream.h>
#include <Poco/File.h>
#include <chrono>
#include <thread>

//moodycamel::ConcurrentQueue<std::string> LogMonitor::logQueue_;

LogMonitor::LogMonitor(const std::string& filePath) : _filePath(filePath), _stop(false) {}

void LogMonitor::startMonitoring(LogTopic* logTopic)
{
	INFOLOG("LogData: start ...");
	logTopic_ = logTopic;
	//Poco::RunnableAdapter<LogMonitor> runnable(*this, &LogMonitor::run);
	//_thread.start(runnable);
	std::thread threadId(LogMonitor::run, this);
	threadId.detach();
}

void LogMonitor::stopMonitoring()
{
	INFOLOG("LogData: stoped ...");
	_stop = true;
}

/*
void LogMonitor::run(const void* arg)
{
	auto logMonitor = (LogMonitor*)arg;
	INFOLOG("LogData: run trace log:{}.", logMonitor->_filePath);
	logMonitor->_stop = false;
	auto start = std::chrono::high_resolution_clock::now();
	std::streampos lastPos = 0;
	std::streampos maxPos = 0;
	Poco::FileStream fs(logMonitor->_filePath, std::ios::in); // Move file stream outside the loop
	while (!logMonitor->_stop)
	{
		std::string line;
		INFOLOG("LogData:trace:{}", logMonitor->_filePath);
		bool hasNewLine = false;
		fs.clear(); // Clear the error state of the file stream
		fs.seekg(lastPos); // Seek to the last position without moving to the end
		if (!fs.good()) {
			INFOLOG("LogData:read failed:{}", logMonitor->_filePath);
			fs.close();
			fs.open(logMonitor->_filePath, std::ios::in);
			lastPos = 0;
		}
		else {
			INFOLOG("LogData:file open good:{}", logMonitor->_filePath);
			fs.seekg(0, fs.end);
			std::streampos fileSize = fs.tellg();
			if (lastPos > fileSize) {
				INFOLOG("LogData:fileSize changed,reopen:{}", logMonitor->_filePath);
				fs.close();
				fs.open(logMonitor->_filePath, std::ios::in);
				lastPos = 0;
			}
			fs.seekg(lastPos);
		}
		size_t lineCount = 0;
		bool isEof = false;
		while (std::getline(fs, line))
		{
			if (!fs.eof()) {
				lineCount++;
				logMonitor->logQueue_.enqueue(line);
				if (logMonitor->logQueue_.size_approx() > logMonitor->_maxQueueSize)
				{
					std::string logData;
					bool dequeued = logMonitor->logQueue_.try_dequeue(logData); // Check the return value of try_dequeue
					if (!dequeued) {
						// Handle the case where try_dequeue failed
					}
				}
				auto now = std::chrono::high_resolution_clock::now();
				auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
				if (duration.count() < 200) {
					Poco::Thread::sleep(1);
					continue;
				}
				if (logMonitor->logTopic_ != nullptr && logMonitor->logQueue_.size_approx() > 0) {
					std::string jsonData = logMonitor->consumeLogQueueWithJson();
					if (logMonitor->logTopic_->write_log_data(logMonitor->_filePath, jsonData)) {
						INFOLOG("LogData:{} send success.", logMonitor->_filePath);
					};
				}
				INFOLOG("LogData:{} read line:{}.", logMonitor->_filePath, lineCount);
				start = std::chrono::high_resolution_clock::now();
				isEof = false;

				lastPos = fs.tellg();
			}
			else 
			{
				isEof = true;
				lastPos = fs.tellg();
				Poco::Thread::sleep(200);
			}

			if (logMonitor->_stop) {
				fs.close();
				return;
			}
		}
		lastPos = fs.tellg(); // Save the current position of the file stream
		if (lastPos > maxPos) {
			maxPos = lastPos;
		}
		INFOLOG("LogData:lastPos:{}", lastPos);
		if (fs.eof()) {
			INFOLOG("LogData:file is eneed,seekg to lastPos:{}", lastPos);
			fs.clear(); // Clear the EOF flag
			fs.seekg(lastPos); // Go back to the last position
			Poco::Thread::sleep(200); // If end of file is reached, sleep for 200 ms
		}
	}
	fs.close(); // Close the file stream after the loop
}
*/

void LogMonitor::run(const void* arg) {
	auto logMonitor = (LogMonitor*)arg;
	INFOLOG("LogData: run trace log:{}.", logMonitor->_filePath);
	logMonitor->_stop = false;
	auto start = std::chrono::high_resolution_clock::now();
	Poco::FileInputStream fs(logMonitor->_filePath, std::ios::in);
	std::streampos lastPos = 0;
	std::string line;
	size_t lineCount = 0;
	while (!logMonitor->_stop) {
		fs.clear();
		fs.seekg(lastPos);

		while (!logMonitor->_stop&&std::getline(fs, line)) {
			// 处理每一行的数据
			//std::cout << line << std::endl;
			lineCount++;
			logMonitor->logQueue_.enqueue(line);
			if (logMonitor->logQueue_.size_approx() > logMonitor->_maxQueueSize)
			{
				std::string logData;
				bool dequeued = logMonitor->logQueue_.try_dequeue(logData); // Check the return value of try_dequeue
				if (!dequeued) {
					// Handle the case where try_dequeue failed
				}
			}
			auto now = std::chrono::high_resolution_clock::now();
			auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
			if (duration.count() < 200) {
				Poco::Thread::sleep(1);
				continue;
			}
			if (logMonitor->logTopic_ != nullptr && logMonitor->logQueue_.size_approx() > 0) {
				std::string jsonData = logMonitor->consumeLogQueueWithJson();
				if (logMonitor->logTopic_->write_log_data(logMonitor->_filePath, jsonData)) {
					INFOLOG("LogData:{} send success.", logMonitor->_filePath);
				};
			}
			INFOLOG("LogData:{} read line:{}.", logMonitor->_filePath, lineCount);
			start = std::chrono::high_resolution_clock::now();
			lastPos = fs.tellg();
			Poco::Thread::sleep(1);
		}

		if (fs.eof()) {
			// 如果到达文件的末尾，等待一段时间再次尝试读取
			Poco::Thread::sleep(500);
			fs.clear();
			fs.close();
			Poco::File file(logMonitor->_filePath);
			if (file.getSize() > lastPos) {
				// 如果文件的大小大于上次读取的位置，说明有新的数据写入，继续读取
				fs.open(logMonitor->_filePath, std::ios::in);
				continue;
			}
		}
	}

	fs.close();
}


std::string LogMonitor::getActiveMonitor()
{
	return _filePath;
}

std::vector<std::string> LogMonitor::consumeLogQueue()
{
	//Poco::Mutex::ScopedLock lock(_mutex); // Lock for thread safety
	std::vector<std::string> messages;
	while (logQueue_.size_approx() > 0)
	{
		std::string logData;
		if (logQueue_.try_dequeue(logData)) {
			messages.push_back(logData);
		}
	}
	return messages;
}

std::string LogMonitor::consumeLogQueueWithJson()
{
	//Poco::Mutex::ScopedLock lock(_mutex); // Lock for thread safety
	std::vector<std::string> messages;
	while (logQueue_.size_approx() > 0)
	{
		std::string logData;
		if (logQueue_.try_dequeue(logData)) {
			messages.push_back(logData);
		}
	}

	Poco::JSON::Array::Ptr jsonArray = new Poco::JSON::Array();
	for (const auto& message : messages)
	{
		jsonArray->add(message);
	}

	std::stringstream ss;
	Poco::JSON::Stringifier::stringify(jsonArray, ss);

	INFOLOG("LogData:return jsonStr:()", ss.str());

	return ss.str();
}