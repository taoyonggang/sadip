
#include "MonitorTopic.h"
#include "../../db/DbBase.h"

using namespace cn::seisys::dds;

MonitorTopic::MonitorTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
}

MonitorTopic::~MonitorTopic() {
    is_reader_ = false;
}



void MonitorTopic::start_reader(bool wait) {

    if (!reader_thread_) {
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&MonitorTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }

}


void MonitorTopic::reader_worker() {
    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize monitor reader base ZenohBase");
            return;
        }

        ////创建监控发送端
        //ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（MonitorTopic）只有订阅，不需要回执
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_mon(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("Monitor Subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));
    }
    catch (const std::exception& e)
    {
        ERRORLOG("Monitor reader worker error: {}", e.what());
    }

    while (is_reader_) {

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}





void MonitorTopic::handle_mon(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        size_t size = sample.get_payload().size();
        TopicMonInfo mon_msg;
        mon_msg.ParsePartialFromArray(data.data(), size);

        DEBUGLOG("Receive mon info: src_node_id:{},to_node_id:{},program_name:{},topic_name:{},src_type:{}", mon_msg.srcnodeid(),
            mon_msg.tonodeid(), mon_msg.progarmname(), mon_msg.topicname(), mon_msg.srctype());
        // 统计监控消息入库
        insert_mon(mon_msg);
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle mon failed: {}", e.what());
    }
}



void MonitorTopic::insert_mon(cn::seisys::dds::TopicMonInfo monitor)
{
    DbBase* db = DbBase::getDbInstance();
    int ret = 1;
    char monInfoSql[4096];
    sprintf(monInfoSql, "insert into `topic_mon_info`(`uuid`,`domain`,`src_node_id`,\
	`to_node_id`,`topic_name`,`cycle`,`n_count`,\
	`n_size`,`src_type`,`program_name`)\
	VALUES\
	('%s', %d, '%s',\
	'%s', '%s', %ld, %ld,\
	%ld, %d,'%s');",
        monitor.uuid().c_str(), monitor.domain(), monitor.srcnodeid().c_str(),
        monitor.tonodeid().c_str(), monitor.topicname().c_str(), monitor.cycle(), monitor.ncount(),
        monitor.nsize(), monitor.srctype(),monitor.progarmname().c_str());
    ret = db->excuteSql(monInfoSql);
    if (ret < 0)
    {
        ERRORLOG("Insert MonInfo Data Fail,MonInfoSql:{}", monInfoSql);
    }
}


