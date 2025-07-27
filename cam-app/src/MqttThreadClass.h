#ifndef _MQTT_THREAD_CLASS_
#define _MQTT_THREAD_CLASS_

#include<string>
#include<vector>

using namespace std;

typedef struct mqttConf
{
	string mqttAddr;
	string clientId;
	bool sessionFlag;
	string user;
	string pwd;
	string camTopic;
	//vector<string> topicVec;
}MQTTCONF;

class MqttThreadClass
{
public:
	void mqtt_start_sub(MQTTCONF& mqttConfig);
	void mqtt_sub_work(MQTTCONF& mqttConfig);

	void mqtt_start_pub(MQTTCONF& mqttConfig);
	void mqtt_pub_work(MQTTCONF& mqttConfig);
};
#endif // !_MQTT_THREAD_CLASS_
