#pragma once
#include "../pch.h"
#define SPDLOG_HEADER_ONLY
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define SPDLOG_TRACE_ON
//#define SPDLOG_DEBUG_ON
//#define WIN32_LEAN_AND_MEAN
#include "spdlog/spdlog.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/sinks/stdout_color_sinks.h"

//spdlog 简易封装
//''' 示例
// Qt使用时 CONFIG += utf8_source
// INITLOG(QString("日志.log").toLocal8Bit().toStdString());
// ERRORLOG("测试下 {0} {1} {2} {3}",123,true,"测试下",QString("测试下").toStdString());
// WARNLOG("测试下 {0} {1} {2} {3}",123,true,"测试下",QString("测试下").toStdString());
// INFOLOG("测试下 {0} {1} {2} {3}",123,true,"测试下",QString("测试下").toStdString());
// DEBUGLOG("测试下 {0} {1} {2} {3}",123,true,"测试下",QString("测试下").toStdString());
// TRACELOG("测试下 {0} {1} {2} {3}",123,true,"测试下",QString("测试下").toStdString());
//'''
//spdlog讲解参见博客 https://www.cnblogs.com/oucsheep/p/8426548.html
class BaseLog
{
private:
	BaseLog() = default;
public:
	static BaseLog* getInstance() {
		static BaseLog instance;
		return &instance;
	}

	//初始化日志,路径使用locale编码
	//如: QString("日志.log").toLocal8Bit().toStdString()
	void init(const std::string& path, const std::string& logName, spdlog::level::level_enum mode) {
		//名字必须唯一
		logName_ = logName;
		//自定义的sink
		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		//console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][t:%t][%^%l%$][%s %!:%#]:%v");
		//auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>("DataBus", "logs/node.log", 1048576 * 100, 7);
		auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(path, 1024 * 1024 * 20, 5);
		//rotating_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e][t:%t][%^%l%$][%s %!:%#]:%v");
		std::vector<spdlog::sink_ptr> sinks;
		sinks.push_back(console_sink);
		sinks.push_back(rotating_sink);

		logPtr_ = new spdlog::logger(logName_, begin(sinks), end(sinks));

		//可以配置多个sink
		//std::make_shared<spdlog::logger>
		//spdlog::register_logger(logPtr); 配合 spdlog::drop_all();

		//设置日志记录级别
		//logPtr_->set_level(spdlog::level::debug);
		logPtr_->set_level(mode);
		//设置格式
		//参见文档 https://github.com/gabime/spdlog/wiki/3.-Custom-formatting
		//[%Y-%m-%d %H:%M:%S.%e] 时间
		//[%l] 日志级别
		//[%t] 线程
		//[%s] 文件
		//[%#] 行号
		//[%!] 函数
		//[%v] 实际文本
		logPtr_->set_pattern("[%Y-%m-%d %H:%M:%S.%e][t:%t][%^%l%$][%s %!:%#]:%v");
		//设置当出发 err 或更严重的错误时立刻刷新日志到  disk
		logPtr_->flush_on(spdlog::level::trace);
		spdlog::flush_every(std::chrono::seconds(5));
		spdlog::register_logger((std::shared_ptr<spdlog::logger>)logPtr_);
	}

	void uinit() {
		spdlog::drop(logName_);
		//if (logPtr_)
		//	delete logPtr_;		
		//logPtr_ = NULL;
	}

	auto logger() {
		return logPtr_;
	}

private:
	spdlog::logger* logPtr_;
	std::string logName_;
};

#define INITLOG(path,logName,mode)      BaseLog::getInstance()->init(path,logName,mode)
#define UINITLOG()      BaseLog::getInstance()->uinit()
//参见SPDLOG_LOGGER_CALL
#define SPDLOG_BASE(logger, level, ...) (logger)->log(spdlog::source_loc{__FILE__, __LINE__, __func__}, level, __VA_ARGS__)
#define TRACELOG(...)     SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::trace, __VA_ARGS__)
#define DEBUGLOG(...)     SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::debug, __VA_ARGS__)
#define INFOLOG(...)      SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::info, __VA_ARGS__)
#define WARNLOG(...)      SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::warn, __VA_ARGS__)
#define ERRORLOG(...)     SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::err, __VA_ARGS__)
#define CRITICALLOG(...)  SPDLOG_BASE(BaseLog::getInstance()->logger(), spdlog::level::critical, __VA_ARGS__)