//#include "MqttSubscriber.h"
#include "../utils/log/BaseLog.h"
#include <chrono>
#include "../utils/common.h"
#include "MqttSubscriber.h"

extern cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData>camSendQueue_;
extern cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData>simplifiedCamQueue_;
//cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData> callback::camQueue_;
cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::MultiPathDatas> callback::cameraQueue_;
cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::MultiPathDatas> callback::radarQueue_;

void action_listener::on_failure(const mqtt::token& tok)
{
	ERRORLOG("action{} failure", name_);
	if (tok.get_message_id() != 0)
		INFOLOG(" for token: {}", tok.get_message_id()); 
}

void action_listener::on_success(const mqtt::token& tok)
{
	INFOLOG("action {} sucess", name_); 
	if (tok.get_message_id() != 0)
		INFOLOG(" for token:{}", tok.get_message_id()); 
	auto top = tok.get_topics();
	if (top && !top->empty())
		INFOLOG("token topic : '{}',...", (*top)[0]);
}

action_listener::action_listener(const std::string& name) : name_(name) {}

void callback::reconnection()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	try
	{
		cli_.connect(connOpts_, nullptr, *this);
	}
	catch (const mqtt::exception& exc)
	{
		ERRORLOG("Error: {}", exc.what());
		//exit(1);
		return;

	}
}

void callback::on_failure(const mqtt::token& tok)
{
	INFOLOG("Connection attempt failed");
	if (++nretry_ > N_RETRY_ATTEMPTS)
		//exit(1);
		nretry_=0;
	reconnection();
}

void callback::connected(const std::string& cause)
{
	INFOLOG("Connection success");
	//INFOLOG("Subscribing to topic:{} for client:{} using QoS:{}", TOPIC, CLIENT_ID, MQTT_QOS);
	//LLOG(INFO) << "Subscribing to topic:'" << TOPIC << "for client:" << CLIENT_ID << " using QoS:" << QOS << "Press Q<Enter> to quit";

	for (std::list<string>::iterator it = topic_list.begin(); it != topic_list.end(); ++it)
	{
		cli_.subscribe(*it, MQTT_QOS, nullptr, subListener_);
	}
}

void callback::connection_lost(const std::string& cause)
{
	INFOLOG("Connection lost");
	if (!cause.empty())
		INFOLOG("cause:{}", cause);

	INFOLOG("Reconnecting...");
	nretry_ = 0;
	reconnection();
}

void callback::message_arrived(mqtt::const_message_ptr msg)
{
	INFOLOG("Message arrived of topic:{}", msg->get_topic());
	try
	{
		string topic = msg->get_topic();
		string::size_type idx = topic.find("camera");
		if (idx != string::npos) // cameraData
		{
			MultiPathDatas cameraData;
			cameraData.ParseFromString(msg->get_payload());
			if (callback::cameraQueue_.Size() > MAX_MSG)
			{
				callback::cameraQueue_.Empty();
			}
			callback::cameraQueue_.Push(cameraData);
		}
		else if (topic.find("cam") != string::npos) // camData
		{
			CamData camData;
			camData.ParseFromString(msg->get_payload());
			uint64_t tocloudtime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			camData.set_tocloudtime(tocloudtime);
			if (camSendQueue_.Size() > MAX_MSG)
			{
				camSendQueue_.Empty();
			}
			// camSendQueue_发送cam数据到云端，simplifiedCamQueue_直接发送简化cam数据显示
			camSendQueue_.Push(camData);
			if (simplifiedCamQueue_.Size() > MAX_MSG)
			{
				simplifiedCamQueue_.Empty();
			}
			simplifiedCamQueue_.Push(camData);
		}
		else if (topic.find("radar") != string::npos) // radarData
		{
			MultiPathDatas radarData;
			radarData.ParseFromString(msg->get_payload());
			if (callback::radarQueue_.Size() > MAX_MSG)
			{
				callback::radarQueue_.Empty();
			}
			callback::radarQueue_.Push(radarData);
		}

	}
		catch (...)
		{
		}
}

callback::callback(mqtt::async_client& cli, mqtt::connect_options& connOpts)
	: nretry_(0), cli_(cli), connOpts_(connOpts), subListener_("Subscription") {}

void callback::add_topic(string topic)
{
	topic_list.push_back(topic);
}