#include "KafkaConsumer.h"

KafkaConsumer::KafkaConsumer(const std::string &brokers, const std::string &groupId,
                             const std::vector<std::string> &topics, int partition,
                             const std::string user, const std::string password)
{
    m_brokers = brokers;
    m_groupId = groupId;
    m_topicVector = topics;
    m_partition = partition;
    m_user = user;
    m_password = password;

    // 创建Conf实例：
    m_config = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if (m_config == nullptr)
    {
        std::cout << "Create Rdkafka Global Conf Failed." << std::endl;
    }

    m_topicConfig = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    if (m_topicConfig == nullptr)
    {
        std::cout << "Create Rdkafka Topic Conf Failed." << std::endl;
    }

    // 设置Conf的各个配置参数：
    RdKafka::Conf::ConfResult result;
    std::string error_str;

    result = m_config->set("bootstrap.servers", m_brokers, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'bootstrap.servers' failed: " << error_str << std::endl;
    }

    result = m_config->set("group.id", m_groupId, error_str); // 设置消费组名：group.id（string类型）
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'group.id' failed: " << error_str << std::endl;
    }

    
    result = m_config->set("sasl.mechanisms", "PLAIN", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set ‘sasl.mechanisms=PLAIN’ failed: " << error_str << std::endl;
    }

    result = m_config->set("sasl.username", m_user, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set ‘sasl.username’ failed: " << error_str << std::endl;
    }

    result = m_config->set("sasl.password", m_password, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set ‘sasl.password’ failed: " << error_str << std::endl;
    }

    result = m_config->set("api.version.request", "true", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set ‘api.version.request=true’ failed: " << error_str << std::endl;
    }

    

    result = m_config->set("max.partition.fetch.bytes", "1024000", error_str); // 消费消息的最大大小
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'max.partition.fetch.bytes' failed: " << error_str << std::endl;
    }

    result = m_config->set("enable.partition.eof", "false", error_str); // enable.partition.eof: 当消费者到达分区结尾，发送 RD_KAFKA_RESP_ERR__PARTITION_EOF 事件，默认值 true
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'enable.partition.eof' failed: " << error_str << std::endl;
    }

    m_event_cb = new ConsumerEventCb;
    result = m_config->set("event_cb", m_event_cb, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'event_cb' failed: " << error_str << std::endl;
    }

    m_rebalance_cb = new ConsumerRebalanceCb;
    result = m_config->set("rebalance_cb", m_rebalance_cb, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'rebalance_cb' failed: " << error_str << std::endl;
    }

    // 设置topic_conf的配置项：
    result = m_topicConfig->set("auto.offset.reset", "latest", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Topic Conf set 'auto.offset.reset' failed: " << error_str << std::endl;
    }

    result = m_config->set("default_topic_conf", m_topicConfig, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Conf set 'default_topic_conf' failed: " << error_str << std::endl;
    }

    // 创建消费者客户端：
    m_consumer = RdKafka::KafkaConsumer::create(m_config, error_str);
    if (m_consumer == nullptr)
    {
        std::cout << "Create KafkaConsumer failed: " << error_str << std::endl;
    }
    std::cout << "Create KafkaConsumer succeed, consumer name : " << m_consumer->name() << std::endl;

    // 订阅m_topicVector中的topic
    RdKafka::ErrorCode error_code = m_consumer->subscribe(m_topicVector);
    if (error_code != RdKafka::ErrorCode::ERR_NO_ERROR)
    {
        std::cerr << "Consumer subscribe topics failed: " << RdKafka::err2str(error_code) << std::endl;
    }
}

KafkaConsumer::~KafkaConsumer()
{
    delete m_config;
    delete m_topicConfig;
    delete m_consumer;
    delete m_event_cb;
    delete m_rebalance_cb;
}

std::string KafkaConsumer::pullMessage()
{
    RdKafka::Message* m_message = m_consumer->consume(1000);

    switch (m_message->err())
    {
    case RdKafka::ERR_NO_ERROR:
    {
        // 成功接收到消息
        if (m_message->payload() != nullptr) {
            std::string payload(static_cast<const char*>(m_message->payload()), m_message->len());
            delete m_message;
            return payload;
        }
        else {
            delete m_message;
            return ""; // 返回空字符串表示空消息
        }
    }
    case RdKafka::ERR__TIMED_OUT:
        // 超时，没有接收到新消息
        delete m_message;
        return "";
    case RdKafka::ERR__PARTITION_EOF:
        // 到达分区末尾
        delete m_message;
        return "";
    default:
        // 其他错误
        std::cerr << "Consume error: " << m_message->errstr() << std::endl;
        delete m_message;
        return "";
    }
}

