#ifndef __KAFKAPRODUCER_H_
#define __KAFKAPRODUCER_H_

#include <string>
#include <iostream>
#include <librdkafka/rdkafkacpp.h>

class KafkaProducer
{
public:
    // explicit：禁止隐式转换，例如不能通过string的构造函数转换出一个broker
    explicit KafkaProducer(const std::string &brokers, const std::string &topic,
        const std::string user, const std::string password);
    ~KafkaProducer();

    void pushMessage(const std::string &msg, const std::string &key);

protected:
    std::string m_brokers;
    std::string m_topicStr;
    std::string m_user;
    std::string m_password;

    RdKafka::Conf *m_producerConfig; // RdKafka::Conf 配置接口类，用来设置对生产者、消费者、broker的各项配置值
    RdKafka::Conf *m_topicConfig;

    RdKafka::Producer *m_producer;
    RdKafka::Topic *m_topic;

    RdKafka::DeliveryReportCb *m_dr_cb;       // RdKafka::DeliveryReportCb 用于在调用 RdKafka::Producer::produce() 后返回发送结果，RdKafka::DeliveryReportCb是一个类，需要自行填充其中的回调函数及处理返回结果的方式
    RdKafka::EventCb *m_event_cb;             // RdKafka::EventCb 用于从librdkafka向应用程序传递errors,statistics,logs 等信息的通用接口
    RdKafka::PartitionerCb *m_partitioner_cb; // Rdkafka::PartitionerCb 用于设定自定义分区器
};

class ProducerDeliveryReportCb : public RdKafka::DeliveryReportCb
{
public:
    void dr_cb(RdKafka::Message &message)
    { // 重载基类RdKafka::DeliveryReportCb中的虚函数dr_cb()
        if (message.err() != 0)
        { // 发送出错
            std::cerr << "Message delivery failed: " << message.errstr() << std::endl;
        }
        else
        { // 发送成功
            std::cerr << "Message delivered to topic: " << message.topic_name()
                      << " [" << message.partition()
                      << "] at offset " << message.offset() << std::endl;
        }
    }
};

class ProducerEventCb : public RdKafka::EventCb
{
public:
    void event_cb(RdKafka::Event &event)
    {

        switch (event.type())
        {
        case RdKafka::Event::EVENT_ERROR:
            std::cout << "RdKafka::EVENT::EVENT_ERROR: " << RdKafka::err2str(event.err()) << std::endl;
            break;
        case RdKafka::Event::EVENT_STATS:
            std::cout << "RdKafka::EVENT::EVENT_STATS: " << event.str() << std::endl;
            break;
        case RdKafka::Event::EVENT_LOG:
            std::cout << "RdKafka::EVENT::EVENT_LOG: " << event.fac() << std::endl;
            break;
        case RdKafka::Event::EVENT_THROTTLE:
            std::cout << "RdKafka::EVENT::EVENT_THROTTLE: " << event.broker_name() << std::endl;
            break;
        }
    }
};

class HashPartitionerCb : public RdKafka::PartitionerCb
{
    // 自定义生产者分区器，作用就是返回一个分区id。  对key计算Hash值，得到待发送的分区号（其实这跟默认的分区器计算方式是一样的）
public:
    int32_t partitioner_cb(const RdKafka::Topic *topic, const std::string *key,
                           int32_t partition_cnt, void *msg_opaque)
    {
        char msg[128] = {0};
        sprintf(msg, "HashPartitionCb:[%s][%s][%d]", topic->name().c_str(), key->c_str(), partition_cnt);
        std::cout << msg << std::endl;

        // 前面的操作只是为了在分区器回调中打印出一行打印
        // 分区器真正的操作是在下面generate_hash，生成一个待发送的分区ID
        return generate_hash(key->c_str(), key->size()) % partition_cnt;
    }

private:
    static inline unsigned int generate_hash(const char *str, size_t len)
    {
        unsigned int hash = 5381;
        for (size_t i = 0; i < len; i++)
        {
            hash = ((hash << 5) + hash) + str[i];
        }
        //返回值必须在0到partition_cnt之间。如果出错则发回PARTITION_UA（-1）
        return hash; 
    }
};

#endif
