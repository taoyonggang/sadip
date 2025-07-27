#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0


#include "UserTcpServer.h"
//系统头文件
#include <iostream>
#include <utility>
#include <thread>
#include <chrono>
#include <functional>
#include <atomic>


//自编头文件
#include "../utils/log/BaseLog.h"
#include "../db/DbBase.h"
#include "CamTopic.h"
#include "../utils/FileSplit.h"
#include "../utils/nodeInfo.h"
#include "proto/FusionPathDatas.pb.h"
#include "proto/v2x.pb.h"
#include "DataInsert.h"
#include "TcpRecv.h"
#include "../utils/ini/INIReader.h"
#include "../utils/SnowFlake.h"
#include "MqttSubscriber.h"
#include "../utils/common.h"
#include <google/protobuf/util/json_util.h>
#include <soci/soci.h>
#include <soci/blob.h>
#include <soci/mysql/soci-mysql.h>


using namespace std;
using namespace std::chrono;
using namespace cn::seisys::rbx::comm::bean::multi;
using namespace cn::seisys::v2x::pb;
using google::protobuf::util::JsonStringToMessage;

extern cn::ThreadSafeQueue<::RteDistribution> rteQueue_;
extern cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData>camSendQueue_;
extern cn::ThreadSafeQueue<::RtsDistribution> rtsQueue_;
//const char* CAM_TYPE = "cn::seisys::dds:Cam";
//const char* CAM_REPLY_TYPE = "cn::seisys::dds:CamReply";
//
//CamTopic::CamTopic(int argc, char** argv, TopicInfo topicInfo) :BaseTopic<CamPubSubType, CamReplyPubSubType, DataWriter, DataReader, Cam, CamReply>(argc, argv, topicInfo) {
//	ts_ = new TypeSupport(new ::CamPubSubType());
//	trs_ = new TypeSupport(new ::CamReplyPubSubType());
//};
//
//CamTopic::~CamTopic() {
//
//}
//
//void CamTopic::start_writer(int argc, char** argv, bool wait) {
//
//	if (isReader) {
//		//ACE_ERROR((LM_ERROR, ACE_TEXT("ERROR: %N:%l: worker() -"), ACE_TEXT(" topic has started with subscriber!")));
//		ERRORLOG("worker() - cam topic has started with subscriber!");
//		return;
//	}
//
//	isWriter = true;
//
//	std::thread threadId(CamTopic::writer_worker, this);
//
//	//必须要分离,保证独立运行,不然会出莫名奇妙错误
//	threadId.detach();
//	//if (wait)
//	//    threadId.join();
//}
//
//
////int CamTopic::write_node_msg(Node nodeMsg) {
////    return (this->write_msg(nodeMsg));
////}
//
//void CamTopic::writer_worker(const void* arg) {
//
//	//请把数据库信息写到配置文件,并读取出来,还需要带上连接池的数目
//	//DbBase* db = DbBase::getDbInstance(1, "10.4.1.151", "root", "Seisys@77889900", "databus-dev", 4000);
//	//DbBase* db = DbBase::getDbInstance(1, "192.168.28.251", "root", "123456", "databus-dev1", 3306)；
//	//DbBase* db = DbBase::getDbInstance(1, "10.4.1.203","root","Seisys*77889900","databus-dev",3306);
//	//auto db = DbBase::getDbInstance();
//	auto camTopic = (CamTopic*)arg;
//
//	if (camTopic == NULL) {
//		ERRORLOG("ERROR: %N:%l: worker() - CamTopic is NULL!");
//		exit(-1);
//	}
//	string participant_name = "participant_cam_pub_" + camTopic->topicInfo_.nodeId_;
//
//	assert(!camTopic->create_participant(participant_name));
//	assert(!camTopic->register_type());
//	assert(!camTopic->create_topic(camTopic->topicInfo_.topic_, CAM_TYPE));
//	assert(!camTopic->create_publisher(camTopic->topicInfo_.partition_.c_str()));
//
//	INFOLOG("Cam:writer topic is:{},partition:{}", camTopic->topicInfo_.topic_.c_str(), camTopic->topicInfo_.partition_.c_str());
//
//	auto writer_qos = new DataWriterQos();
//	*writer_qos = DATAWRITER_QOS_DEFAULT;
//	//camTopic->create_writer_qos(writer_qos);
//	if (camTopic->topicInfo_.writerQosLev_ == -1)
//	{
//		DEBUGLOG("camTopic writer qos:{}", -1);
//		camTopic->create_writer_qos(writer_qos);
//	}
//	else if (camTopic->topicInfo_.writerQosLev_ == 0)
//	{
//		DEBUGLOG("camTopic writer qos:{}", 0);
//		camTopic->create_writer_qos0(writer_qos);
//	}
//	else if (camTopic->topicInfo_.writerQosLev_ == 1)
//	{
//		DEBUGLOG("camTopic writer qos:{}", 1);
//		camTopic->create_writer_qos1(writer_qos);
//	}
//	assert(!camTopic->set_writer_qos(writer_qos));
//
//	CamReplyDataReaderListenerImpl* nrdrli = new CamReplyDataReaderListenerImpl();
//
//	//先订阅回执topic
//	if (camTopic->topicInfo_.needReply_)
//		camTopic->create_replay_sub(*nrdrli);
//
//	assert(!camTopic->create_datawriter());
//
//
//	while (true) {
//		//下面数据需要从配置文件或者数据库中获取,这里为了测试,暂时写死
//		Cam camMsg;
//		CamData cam;
//		string camStr = "";
//		uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//		if (camSendQueue_.WaitAndTryPop(cam, std::chrono::milliseconds(1000)))
//		{
//			//DEBUGLOG("camData Ptclist Size:{} camData Trafficflow Size:{} camData :{}", cam.ptclist_size(), cam.trafficflow_size(),cam.DebugString());
//			cam.SerializeToString(&camStr);
//			//DEBUGLOG("camData Ptclist Size:{} camData :{}",cam.ptclist_size(),cam.DebugString());
//			//cam.SerializeToString(&camStr);
//
//			// 构建camMsg消息结构
//			std::vector<unsigned char> vecData(camStr.begin(), camStr.end());
//			cn::seisys::dds::OctetSeq camString(vecData.begin(), vecData.end());
//			camMsg.msgId(utility::uuid::generate());
//			camMsg.srcNodeId(utility::node::nodeInfo_.nodeId_);
//			camMsg.toNodeId("*");// 100011000110001
//			camMsg.msgType(11);
//			camMsg.length(camStr.size());
//			camMsg.data(camString);
//			camMsg.createdAt(now);
//			camMsg.updatedAt(now);
//			INFOLOG("Cam:发送任务开始获取任务数据,{} ", now);
//			size_t length = camMsg.getCdrSerializedSize(camMsg);
//			//cn::seisys::dds::CamPubSubType type_support;
//			//// 创建 SerializedPayload_t 对象
//			//eprosima::fastrtps::rtps::SerializedPayload_t payload;
//			//type_support.serialize(&camMsg, &payload);
//			//size_t length = payload.length;
//
//			try {
//				//if (camTopic->write_msg(camMsg) == 0) 
//				if ((BaseTopic*)camTopic->write_data((void*) &camMsg, length, camMsg.toNodeId()) == 0)
//				{
//					INFOLOG("Cam:send msg susscess at:{} ", now);
//				}
//				else {
//					INFOLOG("Cam:send msg failed at:{} ", now);
//				}
//			}
//			catch (const exception& e) {
//				ERRORLOG("Cam:错误:{}!", e.what());
//			}
//		}
//
//		//INFOLOG("cam_edge_app writer is running ...");
//		if (camTopic->is_stoped()) {
//			break;
//		};
//		//处理回执,最好单独开线程
//		//NodeReply nodeReply = nrdrli.nodeReplyQueue.pop();
//		//Sleep(1000);
//		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
//	}
//
//}
//
//
//
//void CamTopic::start_reader(int argc, char** argv, bool wait) {
//
//	if (isWriter) {
//		ERRORLOG("worker() - cam topic has started with publisher!");
//		return;
//	}
//
//	isReader = true;
//
//	std::thread threadId(CamTopic::reader_worker, this);
//
//	//必须要分离,保证独立运行,不然会出莫名奇妙错误
//	threadId.detach();
//
//}
//void CamTopic::reader_worker(const void* arg) {
//
//	auto camTopic = (CamTopic*)arg;
//
//	if (camTopic == NULL) {
//		ERRORLOG("worker() - CamTopic is NULL!");
//		exit(-1);
//	}
//
//
//	auto nts = new CamPubSubType();
//	auto nrts = new CamReplyPubSubType();
//
//	auto lsn = new CamDataReaderListenerImpl();
//	//需要开启监控，就需要初始化
//	lsn->initMonitorPublisher(camTopic->topicInfo_);
//	//判断是否要开启kafka转发
//	if (camTopic->topicInfo_.kafkaFlag_) {
//		//配置项加入到配置文件中
//		lsn->initKafka(camTopic->topicInfo_.kafkaBroker_, camTopic->topicInfo_.kafkaTopic_, camTopic->topicInfo_.kafkaUser_, camTopic->topicInfo_.kafkaPwd_);
//		//lsn->initKafka("10.10.1.2:30983", "v2x-cam", "kafka", "ufAvpZ$o0$N70");
//	}
//
//	auto reader_qos = new DataReaderQos(); ;
//	*reader_qos = DATAREADER_QOS_DEFAULT;
//	string participant_name = "participant_cam_sub_" + camTopic->topicInfo_.nodeId_;
//
//	assert(!camTopic->create_participant(participant_name));
//	assert(!camTopic->register_type());
//	assert(!camTopic->create_topic(camTopic->topicInfo_.topic_, CAM_TYPE));
//	//node.100011000210002
//	assert(!camTopic->create_subscriber(camTopic->topicInfo_.partition_.c_str()));
//	//assert(!camTopic->create_subscriber("100011000210002"));
//
//	//ACE_DEBUG((LM_INFO, ACE_TEXT("Cam:reader topic is:%s.%s"), camTopic->topicInfo_.topic_.c_str(), camTopic->topicInfo_.partition_.c_str()));
//	//INFOLOG("Cam:reader topic is:{},{}", camTopic->topicInfo_.topic_.c_str(), camTopic->topicInfo_.partition_.c_str());
//
//	camTopic->get_subscriber()->get_default_datareader_qos(*reader_qos);
//	//reader_qos->reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
//	//camTopic->create_reader_qos(reader_qos);
//	if (camTopic->topicInfo_.readerQosLev_ == -1)
//	{
//		DEBUGLOG("camTopic reader qos:{}", -1);
//		camTopic->create_reader_qos(reader_qos);
//	}
//	else if (camTopic->topicInfo_.readerQosLev_ == 0)
//	{
//		DEBUGLOG("camTopic reader qos:{}", 0);
//		camTopic->create_reader_qos0(reader_qos);
//	}
//	else if (camTopic->topicInfo_.readerQosLev_ == 1)
//	{
//		DEBUGLOG("camTopic reader qos:{}", 1);
//		camTopic->create_reader_qos1(reader_qos);
//	}
//
//	assert(!camTopic->set_reader_qos(reader_qos));
//	assert(!camTopic->set_reader_listener(lsn));
//
//	assert(!camTopic->create_datareader());
//
//	if (camTopic->topicInfo_.needReply_) {
//		assert(!camTopic->register_reply_type());
//		assert(!camTopic->create_reply_topic(camTopic->topicInfo_.topicReply_, CAM_REPLY_TYPE));
//		assert(!camTopic->create_replay_pub());
//		// assert(!camTopic->create_reply_datawriter());
//	}
//
//	
//
//	INIReader reader("config/cam_cloud_app.ini");
//	long macId = reader.GetInteger("gid", "machine_id", 0);
//	long datId = reader.GetInteger("gid", "datacenter_id", 0);
//	SnowFlake* g_sf = NULL;
//	g_sf = new SnowFlake(datId, macId);
//	DataInsert dataInsert;
//	auto db = DbBase::getDbInstance();
//	//添加下发rte构建线程（云->边缘）
//	//std::thread threadId(&CamTopic::build_down_rte, camTopic);
//	//threadId.detach();
//
//
//	uint64_t startTime = 0;
//	uint64_t endTime = 0;
//	uint64_t timeInterval = 0;
//	while (true) {
//
//		Cam cam;
//		if (lsn->camQueue_.TryPop(cam)) {
//
//			CamData camData;
//			std::string camStr(cam.data().begin(), cam.data().end());
//			camData.ParseFromString(camStr);
//			//CamData数据入库操作
//			//INFOLOG("camData:{}", camData.DebugString());
//			startTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//			dataInsert.camInsert(camData, db, g_sf);
//			endTime = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
//			timeInterval = endTime - startTime;
//			INFOLOG("cam_data insert use:{}ms", timeInterval);
//
//		}
//		else {
//			std::this_thread::sleep_for(std::chrono::milliseconds(100));
//		}
//
//		if (camTopic->is_stoped()) {
//			break;
//		};
//	}
//
//	if (lsn) {
//		lsn->uInitKafka();
//		delete lsn;
//		lsn = NULL;
//		camTopic->listener_ = NULL;
//	}
//
//}
//
//
//int CamTopic::create_datawriter() {
//	// Create DataWriter
//	//eprosima::fastdds::dds::Publisher* publisher_ = NULL;
//	writer_ = this->publisher_->create_datawriter(topic_, *writer_qos_, &nodePubListener_);
//
//	if (!writer_) {
//		ERRORLOG("cam create_datawriter failed!");
//		return 1;
//	}
//	return 0;
//};
//
//
//int CamTopic::create_reply_datawriter() {
//	// Create DataWriter
//	reply_writer_ = this->reply_publisher_->create_datawriter(reply_topic_, *reply_writer_qos_, &nodeReplyPubListener_);
//
//	if (!reply_writer_) {
//		ERRORLOG("cam create_datawriter reply failed!");
//		return 1;
//	}
//	return 0;
//};
//
//
//
//int CamTopic::create_datareader() {
//
//	reader_ = subscriber_->create_datareader(topic_, *reader_qos_, listener_);
//
//	//subscriber_->delete_datareader(reader_);
//	if (!reader_) {
//		ERRORLOG("cam create_datareader failed!");
//		return 1;
//	}
//	return 0;
//}
//
//int CamTopic::create_reply_datareader() {
//
//	reply_reader_ =
//		reply_subscriber_->create_datareader(reply_topic_, *reply_reader_qos_, reply_listener_);
//
//	if (!reply_reader_) {
//		ERRORLOG("create_datareader reply failed!");
//		return 1;
//	}
//
//	return 0;
//}
//
//int CamTopic::write_msg(void* msg) {
//
//	bool ret = writer_->write(msg);
//	if (!ret) {
//		ERRORLOG("write node msg failed!");
//		return 1;
//	}
//	return 0;
//};
//
//int CamTopic::write_reply_msg(void* msg) {
//
//	bool ret = reply_writer_->write(msg);
//	if (!ret) {
//		ERRORLOG("write node reply msg failed!");
//		return 1;
//	}
//	return 0;
//};
//
//
//int CamTopic::create_replay_sub(CamReplyDataReaderListenerImpl& lsn) {
//
//	//CamReplyPubSubType* nts = new CamReplyPubSubType();
//	//DataReaderQos* reply_reader_qos = new DataReaderQos();
//	//assert(!this->create_participant());
//	assert(!this->register_reply_type());
//	assert(!this->create_reply_topic(this->topicInfo_.topicReply_, CAM_REPLY_TYPE));
//	assert(!this->create_reply_subscriber("*"));
//	DataReaderQos reply_reader_qos = DATAREADER_QOS_DEFAULT;
//	//this->get_reply_subscriber()->get_default_datareader_qos(reply_reader_qos);
//	//reply_reader_qos->reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
//	this->create_reader_qos(&reply_reader_qos);
//	//reply_reader_qos->reliability.max_blocking_time.sec = 30;
//	//reply_reader_qos->reliability.max_blocking_time.nanosec = 10000 * 1000;
//	//reply_reader_qos->history.kind = KEEP_LAST_HISTORY_QOS;
//	//reply_reader_qos->history.depth = 500;
//	//reply_reader_qos->resource_limits.max_samples = 1000;
//	//reply_reader_qos->resource_limits.max_samples_per_instance = 500;
//
//	assert(!this->set_reply_reader_qos(&reply_reader_qos));
//	assert(!this->set_reply_reader_listener(&lsn));
//
//	assert(!this->create_reply_datareader());
//
//	return 0;
//}
//
//int CamTopic::create_replay_pub() {
//
//	//CamReplyPubSubType* nts = new CamReplyPubSubType();
//
//	//DataWriterQos* writer_qos = new DataWriterQos();
//
//	assert(!this->create_reply_publisher(this->topicInfo_.replyPartition_));
//
//	//this->get_reply_publisher()->get_default_datawriter_qos(writer_qos);
//	DataWriterQos* writer_qos = new DataWriterQos(DATAWRITER_QOS_DEFAULT);
//	this->create_writer_qos(writer_qos);
//	//writer_qos.reliability.kind = BEST_EFFORT_RELIABILITY_QOS;
//	//writer_qos->reliability.max_blocking_time.sec = 30;
//	//writer_qos->reliability.max_blocking_time.nanosec = 10000 * 1000;
//	//writer_qos->history.kind = KEEP_LAST_HISTORY_QOS;
//	//writer_qos->history.depth = 500;
//	//writer_qos->resource_limits.max_samples = 1000;
//	//writer_qos->resource_limits.max_samples_per_instance = 500;
//
//	assert(!this->set_reply_writer_qos(writer_qos));
//
//	assert(!this->create_reply_datawriter());
//
//	INFOLOG("Cam:replay writer is running ...");
//
//	return 0;
//}
//
//void CamTopic::NodePubListener::on_publication_matched(
//	DataWriter*,
//	const PublicationMatchedStatus& info)
//{
//	if (info.current_count_change == 1)
//	{
//		matched_ = info.total_count;
//		firstConnected_ = true;
//		INFOLOG("Publisher matched.");
//	}
//	else if (info.current_count_change == -1)
//	{
//		matched_ = info.total_count;
//		INFOLOG("Publisher unmatched.");
//	}
//	else
//	{
//		INFOLOG("{} is not a valid value for PublicationMatchedStatus current count change", info.current_count_change);
//	}
//}
//
//void CamTopic::NodeReplyPubListener::on_publication_matched(
//	DataWriter*,
//	const PublicationMatchedStatus& info)
//{
//	if (info.current_count_change == 1)
//	{
//		matched_ = info.total_count;
//		firstConnected_ = true;
//		INFOLOG("Reply Publisher  matched.");
//	}
//	else if (info.current_count_change == -1)
//	{
//		matched_ = info.total_count;
//		INFOLOG("Reply Publisher unmatched.");
//	}
//	else
//	{
//		INFOLOG("{} is not a valid value for PublicationMatchedStatus current count change", info.current_count_change);
//	}
//}
//
//void CamTopic::build_down_rte()
//{
//	try {
//		
//		while (true)
//		{
//			auto db = DbBase::getDbInstance();
//			long long id;
//			int evevnType;
//			int operateType;
//			string srcDev="";
//			string targetDev = "";
//			string content = "";
//			int contentSizeLen = 0;
//			//soci::session sql;
//			//backend_factory const& backEnd = *soci::factory_mysql();
//			////sql.open(*soci::factory_mysql(), "db=v2x-shuangzhi user=root password=Seisys*77889900 host=10.4.1.151 port=4000");
//			//sql.open(backEnd, "db=v2x-shuangzhi user=root password=Seisys*77889900 host=10.4.1.151 port=4000");
//			//auto ptr_session=db->getSession(nullptr);
//			soci::rowset<soci::row> rows = db->select("select id,event_type,source_device_id,target_device_id,operation_type,content_bytes_len,content_bytes from message_distribution_info where distribution_status_id=1 ");
//			for (const soci::row& row : rows)
//			{
//				if (row.get_indicator(0) != soci::i_null)
//				{
//					id = row.get<long long>(0);
//				}
//				if (row.get_indicator(1) != soci::i_null)
//				{
//					evevnType = row.get<int>(1);
//				}
//				if (row.get_indicator(2) != soci::i_null)
//				{
//					srcDev= row.get<string>(2);
//				}
//				if (row.get_indicator(3) != soci::i_null)
//				{
//					targetDev = row.get<string>(3);
//				}
//				if (row.get_indicator(4) != soci::i_null)
//				{
//					operateType= row.get<int>(4);
//				}
//				//void* content = NULL;
//				if (row.get_indicator(5) != soci::i_null)
//				{
//					contentSizeLen = row.get<int>(5);
//				}
//				if (row.get_indicator(6) != soci::i_null)
//				{
//					content = row.get<string>(6);
//				}
//				if (evevnType == 0)
//				{
//					// rte下发消息结构建
//					RteDistribution rteDown;
//					RteData rteData;
//					rteData.ParseFromArray(content.data(), contentSizeLen);
//
//					DEBUGLOG("rteData:{}", rteData.DebugString());
//					// 将pb结构序列化成string
//					string rteStr = "";
//					rteData.SerializeToString(&rteStr);
//					rteDown.set_msgid(std::to_string(id));
//					rteDown.set_srcnodeid(srcDev);
//					rteDown.set_tonodeid(targetDev);
//					rteDown.set_msgtype(5);
//					rteDown.set_data(rteStr);
//					rteDown.set_length(rteStr.size());
//
//					if (rteQueue_.Size()>MAX_MSG)
//					{
//						rteQueue_.Empty();
//					}
//					rteQueue_.Push(rteDown);
//				}
//				else if (evevnType == 1)
//				{
//					// rts下发消息结构建
//					RtsDistribution rtsDown;
//					RtsData rtsData;
//					rtsDown.set_msgid(std::to_string(id));
//					rtsDown.set_srcnodeid(srcDev);
//					rtsDown.set_tonodeid(targetDev);
//					rtsDown.set_msgtype(6);
//					//rtsDown.data(rtsString);
//					//rtsDown.length(rtsString.size());
//
//					if (rtsQueue_.Size() > MAX_MSG)
//					{
//						rtsQueue_.Empty();
//					}
//					rtsQueue_.Push(rtsDown);
//				}
//			}
//		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//		}
//	}
//	catch (const exception& e)
//	{
//		ERRORLOG("Build Down Rte or Rts Failed:{}", e.what());
//	}
//}
//
//bool CamTopic::proto_to_json(const google::protobuf::Message& message, std::string& json) {
//	google::protobuf::util::JsonPrintOptions options;
//	options.add_whitespace = true;
//	options.always_print_primitive_fields = true;
//	options.preserve_proto_field_names = true;
//	return MessageToJsonString(message, &json, options).ok();
//}

CamTopic::CamTopic(const TopicInfo& topic_info)
    : topic_info_(topic_info) {
    db_ = DbBase::getDbInstance();
    g_sf_ = new SnowFlake(topic_info.datacenterId_,topic_info.machineId_);
}

CamTopic::~CamTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void CamTopic::start_writer(bool wait) {
    if (!writer_thread_) {
        writer_running_ = true;
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&CamTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }
}

void CamTopic::start_reader(bool wait) {
    if (!reader_thread_) {
        reader_running_ = true;
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&CamTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }
}


void CamTopic::writer_worker() {
    DbBase* db = DbBase::getDbInstance();
    //创建数据下发发送端，回执数据接收端
    try {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize cam writer base ZenohBase");
            return;
        }

        //创建监控发送端
        //ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者
        if (!node_publisher_) {
            node_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topic_)
            );
        }

        //// 暂且不添加边缘侧数据回执接收端
        //// 回执数据接收端创建，并入库回执数据
        // // 初始化回复订阅者
        //zenoh::KeyExpr reply_key(topic_info_.topicReply_);
        //auto on_reply = [this](const zenoh::Sample& sample) {
        //    handle_reply(sample);
        //    };

        //auto on_drop_reply = []() {
        //    DEBUGLOG("Node msg reply subscriber dropped");
        //    };

        //reply_subscriber_.emplace(session_->declare_subscriber(
        //    reply_key,
        //    std::move(on_reply),
        //    std::move(on_drop_reply)
        //));
    }
    catch (const std::exception& e) {
        ERRORLOG("Init cam writer failed: {}", e.what());
        return;
    }

    while (is_writer_) {
        //下面数据需要从配置文件或者数据库中获取,这里为了测试,暂时写死
        Cam camMsg;
        CamData cam;
        string camStr = "";
        uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        if (camSendQueue_.WaitAndTryPop(cam, std::chrono::milliseconds(1000)))
        {
            DEBUGLOG("camData Ptclist Size:{} camData Trafficflow Size:{} camData :{}", cam.ptclist_size(), cam.trafficflow_size(),cam.DebugString());
            cam.SerializeToString(&camStr);

            // 构建camMsg消息结构
            camMsg.set_msgid(utility::uuid::generate());
            camMsg.set_srcnodeid(utility::node::nodeInfo_.nodeId_);
            camMsg.set_tonodeid(topic_info_.monNodeId_);
            camMsg.set_msgtype(11);
            camMsg.set_length(camStr.size());
            camMsg.set_data(camStr);
            camMsg.set_createdat(now);
            camMsg.set_updatedat(now);

            try {
                if (send_cam(camMsg))
                {
                    INFOLOG("Cam:send msg susscess at:{} ", now);
                }
                else
                {
                    ERRORLOG("Cam:send msg failed at:{} ", now);
                }
            }
            catch (const exception& e) {
                ERRORLOG("Cam writer:error:{}!", e.what());
            }
        }
    }
}

void CamTopic::reader_worker() {
    //创建数据接收端、回执发送端连接 
    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize cam msg reader base ZenohBase");
            return;
        }

        //创建监控发送端
        //ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（NodeTopic）
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_node(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("Cam msg subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));

        ////云端回执发送端暂不创建
        ////回执发送者创建（NodeReplyTopic）
        //if (!reply_publisher_) {
        //    reply_publisher_ = std::make_shared<zenoh::Publisher>(
        //        session_->declare_publisher(topic_info_.topicReply_)
        //    );
        //}
    }
    catch (const std::exception& e) {
        ERRORLOG("Init cam reader failed: {}", e.what());
        return;
    }


    while (is_reader_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}


bool CamTopic::send_cam(const cn::seisys::dds::Cam& node_msg) {
    try {
        std::vector<uint8_t> serialized_msg(node_msg.ByteSize() + 1);
        node_msg.SerializeToArray(serialized_msg.data(), node_msg.ByteSize());
        node_publisher_->put(serialized_msg);

        ////构建统计发送端监控数据
        //stat_writer_mon_data(node_msg.ByteSize(), node_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send cam failed: {}", e.what());
        return false;
    }
}



bool CamTopic::send_reply(const CamReply& reply_msg) {
    try {
        std::vector<uint8_t> serialized_msg(reply_msg.ByteSize());
        reply_msg.SerializeToArray(serialized_msg.data(), reply_msg.ByteSize());
        reply_publisher_->put(serialized_msg);

        ////构建回执发送监控数据
        //stat_reply_writer_mon_data(reply_msg.ByteSize(), reply_msg.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send reply failed: {}", e.what());
        return false;
    }
}

void CamTopic::handle_node(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        Cam cam_msg;
        cam_msg.ParseFromArray(data.data(), data.size());

        //添加下发rte构建线程（云->边缘）
        //std::thread threadId(&CamTopic::build_down_rte, camTopic);
        //threadId.detach();
        CamData camData;
        camData.ParseFromString(cam_msg.data());
        //CamData数据入库操作
        DEBUGLOG("camData:{}", camData.DebugString());
        startTime_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        data_insert_.camInsert(camData, db_, g_sf_);
        endTime_ = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        INFOLOG("cam_data insert use:{}ms", endTime_-startTime_);
        }
    catch (const std::exception& e) {
        ERRORLOG("Cam  data insert failed: {}", e.what());
    }
}

void CamTopic::handle_reply(const zenoh::Sample& sample) {
    try {
        std::vector<uint8_t> data = sample.get_payload().as_vector();
        CamReply reply;
        reply.ParseFromArray(data.data(), data.size());

    }
    catch (const std::exception& e) {
        ERRORLOG("Handle reply failed: {}", e.what());
    }
}



