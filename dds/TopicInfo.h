#pragma once
#include "../../utils/pch.h"
#include <string>
#include "NodeInfo.h"
class TopicInfo:public NodeInfo
{
public:
	TopicInfo() = default;
	TopicInfo(const NodeInfo& nodeInfo) : NodeInfo(nodeInfo) {}
public:
	//needReply 1 = need
	bool needReply_;
	//topic  1 pub, 2 sub
	int type_;
	//topic
	std::string topic_;
	//topic
	std::string partition_;
	//»ØÖ´topic
	std::string topicReply_;
	//»ØÖ´·ÖÇø
	std::string replyPartition_;
	//topic monitor recv nodeid
	std::string monNodeId_;
	//monitor topic name
	std::string monTopicName_;
	//seconds
	unsigned int monSendSleep_ = 60;
	// kafka 开启标志：true 打开 false 关闭
	bool kafkaFlag_;
	// kafka topic
	std::string kafkaTopic_;
	// kafka broker
	std::string kafkaBroker_;
	// kafka user
	std::string kafkaUser_;
	// kafka password
	std::string kafkaPwd_;
	// 创建writer_qos级别:create_writer_qos:-1;create_writer_qos0:0;create_writer_qos1:1
	int writerQosLev_;
	// 创建reader_qos级别:create_reader_qos:-1;create_reader_qos0:0;create_reader_qos1:1
	int readerQosLev_;
	// Pomdan http接口地址
	std::string podmanBaseUrl_ = "http://127.0.0.1:8888";
	//zenoh 连接配置文件
	std::string configPath_ = "router.json5";

};

