#ifndef __MQTT_PUBLISHER_H__
#define __MQTT_PUBLISHER_H__


#include <stdio.h>
#include <future>
#include <iostream>
#include <string>
#include "../../utils/mqtt/async_client.h"

using namespace std;
/////////////////////////////////////////////////////////////////////////////

/**
 * A callback class for use with the main MQTT client.
 */
class pub_callback : public virtual mqtt::callback
{
public:
	void connection_lost(const string& cause) override;

	void delivery_complete(mqtt::delivery_token_ptr tok) override;
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A base action listener.
 */
class pub_action_listener : public virtual mqtt::iaction_listener
{
protected:
	void on_failure(const mqtt::token& tok) override;

	void on_success(const mqtt::token& tok) override;
};

/////////////////////////////////////////////////////////////////////////////

/**
 * A derived action listener for publish events.
 */
class pub_delivery_action_listener : public pub_action_listener
{
private:

	atomic<bool> done_;

	void on_failure(const mqtt::token& tok) override;

	void on_success(const mqtt::token& tok) override;

public:
	pub_delivery_action_listener() : done_(false) {}
	bool is_done() const { return done_; }
};


#endif
