#include "MqttPublisher.h"
#include "../../utils/log/BaseLog.h"



void pub_callback::connection_lost(const string& cause)
{
    INFOLOG("Connection lost");
    if (!cause.empty())
        INFOLOG("cause: {}", cause);
}

void pub_callback::delivery_complete(mqtt::delivery_token_ptr tok)
{

    INFOLOG("Delivery complete for token:{}", (tok ? tok->get_message_id() : -1));
}

void pub_action_listener::on_failure(const mqtt::token& tok)
{
    ERRORLOG("Listener failure for token:{}", tok.get_message_id());
}

void pub_action_listener::on_success(const mqtt::token& tok)
{
    INFOLOG("Listener success for token:{}", tok.get_message_id());
}

void pub_delivery_action_listener::on_failure(const mqtt::token& tok)
{
    //atomic<bool> done_;
    pub_action_listener::on_failure(tok);
    done_ = true;
}

void pub_delivery_action_listener::on_success(const mqtt::token& tok)
{
    pub_action_listener::on_success(tok);
    done_ = true;
}
