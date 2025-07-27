//** 模块间消息通讯

#pragma once
#include "../node/src/proto/node.pb.h"
#include "../node/src/proto/file.pb.h"
#include <concurrentqueue.h>
#include "log/BaseLog.h"
#include "../../dds/NodeInfo.h"

using namespace cn::seisys::dds;
//using namespace eprosima::fastrtps;

namespace utility
{
	class msg {
	public:
		//主程序消费NodeCmd消息,In或者Out相对于处理程序而言,谁处理后压入就是In,反正是Out
		static moodycamel::ConcurrentQueue<::NodeCmd> mainCmdQueueIn_;
		//主程序生产NodeCmd消息
		static moodycamel::ConcurrentQueue<::NodeCmd> mainCmdQueueOut_;
		//对应回执消费
		static moodycamel::ConcurrentQueue<::NodeCmdReply> mainCmdReplyQueueIn_;
		//对应回执生产
		static moodycamel::ConcurrentQueue<::NodeCmdReply> mainCmdReplyQueueOut_;

		//文件发送有两种场景：
		// 一种是管理节点直接一对多下发,直接根据数据生成发送队列数据,然后发送
		// 从数据库收到文件发送命令后,生成对应nodeCmd,压入此队列(fileDataQueueOut_),让FileTopic消费处理,直接进行发送,同时发送端会接收回执,并处理,
		// 处理好后结果填入fileDataReplyQueueOut_,nodeCmdWorker消费此数据,做最后的处理
		// 对端收到文件数据后,会进行直接落地,并发送处理回执,这些都是FileTopic直接处理了
		//另外一种是通过NodeCmd下发到工作节点,要求工作节点传送某个文件或者目录等操作
		//收到任务后,NodeCmd会根据命令类型,如果是文件类操作,就压入此队列(fileDataQueueOut_),...,需要注意的是,这个命令多了一个文件发送完成后要发送一个
		//nodeCmdReply命令,由FileTopic填入fileDataReplyQueueIn_,NodeCmdTopic消费,表示NodeCmd命令执行成功,不然NodeCmd会重复发送
		static moodycamel::ConcurrentQueue<::NodeCmd> fileDataQueueIn_;
		static moodycamel::ConcurrentQueue<::NodeCmd> fileDataQueueOut_;
		//主程序生产NodeCmd消息
		static moodycamel::ConcurrentQueue<::NodeCmdReply> fileDataReplyQueueIn_;
		static moodycamel::ConcurrentQueue<::NodeCmdReply> fileDataReplyQueueOut_;

	};

	class node {
	public:
		static NodeInfo nodeInfo_;
	};
}
