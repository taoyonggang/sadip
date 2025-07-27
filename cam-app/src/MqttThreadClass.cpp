#include <thread>
#include <chrono>
#include"MqttThreadClass.h"
#include"MqttSubscriber.h"
#include "MqttPublisher.h"
#include "../../utils/log/BaseLog.h"

void MqttThreadClass::mqtt_sub_work(MQTTCONF& mqttConfig)
{
    INFOLOG("Mqtt Sub:Initializing for mqtt server :{}", mqttConfig.mqttAddr);
    mqtt::async_client mqttSubClient(mqttConfig.mqttAddr, mqttConfig.clientId);
    auto connOpts = mqtt::connect_options_builder()
        .clean_session(mqttConfig.sessionFlag)
        .user_name(mqttConfig.user)
        .password(mqttConfig.pwd)
        .automatic_reconnect(true)
        .finalize();


    INFOLOG("Install the sub callback(s) before connecting.");
    callback cb(mqttSubClient, connOpts);
    /*for (int i = 0; i < mqttConfig.topicVec.size(); i++)
    {
        cb.add_topic(mqttConfig.topicVec[i]);
    }*/
    cb.add_topic(mqttConfig.camTopic);
    //cb.add_topic(cameraTopic);
    //cb.add_topic(radarTopic);
    mqttSubClient.set_callback(cb);
    try
    {
        INFOLOG("Sub Client:Connecting to the MQTT server...");
        mqttSubClient.connect(connOpts, nullptr, cb);
    }
    catch (const mqtt::exception& exc)
    {
        ERRORLOG("Sub Client ERROR: Unable to connect to MQTT server: '{}'", mqttConfig.mqttAddr);
        ERRORLOG("ERROR:{}", exc.what());
        return;
        //std::cerr << "ERROR:" << exc;
        //                exit(1);
    }
    //死循环挂住
    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}


void MqttThreadClass::mqtt_start_sub(MQTTCONF& mqttConfig)
{
	INFOLOG("Mqtt Start Subscribe Thread!");
	std::thread threadId(&MqttThreadClass::mqtt_sub_work, this,std::ref(mqttConfig));

	threadId.detach();
}


void MqttThreadClass::mqtt_pub_work(MQTTCONF& mqttConfig)
{
    //mqtt发送端连接配置
    INFOLOG("Mqtt Pub:Initializing for mqtt server :{}", mqttConfig.mqttAddr);
    mqtt::async_client pubClient(mqttConfig.mqttAddr, mqttConfig.clientId);

    // pub callback
    pub_callback cb;
    pubClient.set_callback(cb);

    auto connOpts = mqtt::connect_options_builder()
        .clean_session(mqttConfig.sessionFlag)
        .user_name(mqttConfig.user)
        .password(mqttConfig.pwd)
        .automatic_reconnect(true)
        // .ssl(std::move(sslopts))
        .finalize();
    try
    {
        INFOLOG("Mqtt Pub: Connecting...");
        mqtt::token_ptr conntok = pubClient.connect(connOpts);
        INFOLOG("Mqtt Pub:Waiting for the connection...");
        //LLOG(INFO) << "Waiting for the connection...";
        conntok->wait();
        INFOLOG("  ...OK");
        //LLOG(INFO) << "  ...OK";
    }
    catch (const mqtt::exception& exc)
    {
        ERRORLOG("Mqtt Pub:Error:{}", exc.what());
        return;
        //cerr << exc.what();
        //EXIT_FAILURE;
    }

    //死循环进行发送数据
    while (true)
    {
        //使用Mqtt发送数据
    }




}


void MqttThreadClass::mqtt_start_pub(MQTTCONF& mqttConfig)
{
    INFOLOG("Mqtt Start Publish Thread!");
    std::thread threadId(&MqttThreadClass::mqtt_pub_work, this,std::ref(mqttConfig));

    threadId.detach();
}