#ifndef MQTT_SUBSCRIBER_H
#define MQTT_SUBSCRIBER_H

#include<stdlib.h>
#include<iostream>
#include "../utils/mqtt/async_client.h"
#include "proto/v2x.pb.h"
#include "proto/MultiPathDatas.pb.h"
#include "../utils/ThreadSafeQueue.h"

using namespace cn::seisys::v2x::pb;
using namespace cn::seisys::rbx::comm::bean::multi;
using namespace std;

class action_listener : public virtual mqtt::iaction_listener
{
	std::string name_;

	void on_failure(const mqtt::token& tok) override;

	void on_success(const mqtt::token& tok) override;

public:
	action_listener(const std::string& name);
};

class callback : public virtual mqtt::callback,
	public virtual mqtt::iaction_listener
{
	// Counter for the number of connection retries
	int nretry_;
	// The MQTT client
	mqtt::async_client& cli_;
	// Options to use if we need to reconnect
	mqtt::connect_options& connOpts_;
	// An action listener to display the result of actions.
	action_listener subListener_;

	list<string> topic_list;

	void reconnection();

	// Re-connection failure
	void on_failure(const mqtt::token& tok) override;

	// (Re)connection success
	// Either this or connected() can be used for callbacks.
	void on_success(const mqtt::token& tok) override {}

	// (Re)connection success
	void connected(const std::string& cause) override;

	// Callback for when the connection is lost.
	// This will initiate the attempt to manually reconnect.
	void connection_lost(const std::string& cause) override;

	// Callback for when a message arrives.
	void message_arrived(mqtt::const_message_ptr msg) override;
	void delivery_complete(mqtt::delivery_token_ptr token) override{}

public:
	callback(mqtt::async_client& cli, mqtt::connect_options& connOpts);

	//static cn::ThreadSafeQueue<cn::seisys::v2x::pb::CamData> camQueue_;
	static cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::MultiPathDatas> cameraQueue_;
	static cn::ThreadSafeQueue<cn::seisys::rbx::comm::bean::multi::MultiPathDatas> radarQueue_;

	void add_topic(string topic);

};


#endif // !MQTT_SUBSCRIBER_H
