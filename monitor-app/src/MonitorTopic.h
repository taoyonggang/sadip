#pragma once

#include "../../dds/ZenohBase.h"
#include <mutex>
#include <atomic>
#include <thread>
#include <queue>
#include <optional>
#include "../../dds/monitor.pb.h"
#include "../../dds/TopicInfo.h"

//#include "MonitorReplyDataReaderListenerImpl.h"

using namespace cn::seisys::dds;


class MonitorTopic : public ZenohBase {
public:
	explicit MonitorTopic(const TopicInfo& topic_info);
	~MonitorTopic() override;

	void start_reader(bool wait = false);

private:
	void reader_worker();
	void handle_mon(const zenoh::Sample& sample);
	void insert_mon(cn::seisys::dds::TopicMonInfo monitor);


private:
	TopicInfo topic_info_;
	std::atomic<bool> is_reader_{ false };
	std::optional<zenoh::Subscriber<void>> subscriber_;
	std::unique_ptr<std::thread> reader_thread_;
};


