#ifndef __KAFKACONSUMER_H_
#define __KAFKACONSUMER_H_
#pragma execution_character_set("utf-8")

#include <string>
#include <iostream>
#include <vector>
#include <stdio.h>
#include <librdkafka/rdkafkacpp.h>

class KafkaConsumer
{
public:
    explicit KafkaConsumer(const std::string& brokers, const std::string& groupID,
        const std::vector<std::string>& topics, int partition,
        const std::string user,const std::string password);
    ~KafkaConsumer();

    std::string pullMessage();

protected:
    std::string m_brokers;
    std::string m_groupId;
    std::string m_user;
    std::string m_password;
    std::vector<std::string> m_topicVector; // 一个消费者可以同时订阅多个主题，所有用vector
    int m_partition;

    RdKafka::Conf *m_config;      // GLOBAL 级别的配置（Consumer客户端级别）
    RdKafka::Conf *m_topicConfig; // TOPIC 级别的配置

    RdKafka::KafkaConsumer *m_consumer; // 消费者客户端实例

    RdKafka::EventCb *m_event_cb;         // Event事件回调
    RdKafka::RebalanceCb *m_rebalance_cb; // 再均衡 回调


};

class ConsumerEventCb : public RdKafka::EventCb
{
public:
    void event_cb(RdKafka::Event &event)
    {
        switch (event.type())
        {
        case RdKafka::Event::EVENT_ERROR:
            std::cerr << "ERROR (" << RdKafka::err2str(event.err()) << "): " << event.str() << std::endl;
            break;
        case RdKafka::Event::EVENT_STATS:
            std::cerr << "STATS: " << event.str() << std::endl;
            break;
        case RdKafka::Event::EVENT_LOG:
            fprintf(stderr, "LOG-%i-%s: %sn", event.severity(), event.fac().c_str(), event.str().c_str());
            break;
        case RdKafka::Event::EVENT_THROTTLE:
            std::cerr << "THROTTLED: " << event.throttle_time() << "ms by " << event.broker_name() << " id " << (int)event.broker_id() << std::endl;
            break;
        default:
            std::cerr << "EVENT " << event.type() << " (" << RdKafka::err2str(event.err()) << "): " << event.str() << std::endl;
            break;
        }
    }
};

class ConsumerRebalanceCb : public RdKafka::RebalanceCb
{
public:
    void rebalance_cb(RdKafka::KafkaConsumer *consumer, RdKafka::ErrorCode err,
                      std::vector<RdKafka::TopicPartition *> &partitions) // Kafka服务端通过 err参数传入再均衡的具体事件（发生前、发生后），通过partitions参数传入再均衡 前/后，旧的/新的 分区信息
    {
        std::cerr << "RebalanceCb: " << RdKafka::err2str(err) << ": ";
        printTopicPartition(partitions);

        if (err == RdKafka::ERR__ASSIGN_PARTITIONS)
        {                                 // ERR__ASSIGN_PARTITIONS: 表示“再均衡发生之后，消费者开始消费之前”，此时消费者客户端可以从broker上重新加载offset
            consumer->assign(partitions); // 再均衡后，重新 assign() 订阅这些分区
            partition_count = (int)partitions.size();
        }
        else if (err == RdKafka::ERR__REVOKE_PARTITIONS)
        {                         // ERR__REVOKE_PARTITIONS: 表示“消费者停止消费之后，再均衡发生之前”，此时应用程序可以在这里提交 offset
            consumer->unassign(); // 再均衡前，unassign() 退订这些分区
            partition_count = 0;  // 退订所有分区后，清0
        }
        else
        {
            std::cerr << "Rebalancing error: " << RdKafka::err2str(err) << std::endl;
        }
    }

private:
    static void printTopicPartition(const std::vector<RdKafka::TopicPartition *> &partitions)
    { // 打印出所有的主题、分区信息
        for (unsigned int i = 0; i < partitions.size(); i++)
        {
            std::cerr << partitions[i]->topic() << "[" << partitions[i]->partition() << "], ";
        }
        std::cerr << "n";
    }

private:
    int partition_count; // 保存consumer消费者客户端 当前订阅的分区数
};

#endif
