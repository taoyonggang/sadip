//** 模块间消息通讯

#pragma once
#include "log/BaseLog.h"
#include "../dds/NodeInfo.h"

//using namespace cn::seisys::dds;
//using namespace eprosima::fastrtps;

namespace utility
{
	class node {
	public:
		static NodeInfo nodeInfo_;
	};
}
