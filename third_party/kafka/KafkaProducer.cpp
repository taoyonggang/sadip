#include "KafkaProducer.h"

//("192.168.0.105:9092", "topic_demo", 0)
KafkaProducer::KafkaProducer(const std::string &brokers, const std::string &topic,
    const std::string user, const std::string password)
{
    m_brokers = brokers;
    m_topicStr = topic;
    m_user = user;
    m_password = password;
   

    // 先填充构造生产者客户端的参数配置：
    m_producerConfig = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if (m_producerConfig == nullptr)
    {
        std::cout << "Create Rdkafka Global Conf Failed." << std::endl;
    }

    m_topicConfig = RdKafka::Conf::create(RdKafka::Conf::CONF_TOPIC);
    if (m_topicConfig == nullptr)
    {
        std::cout << "Create Rdkafka Topic Conf Failed." << std::endl;
    }

    // 下面开始配置各种需要的配置项：
    RdKafka::Conf::ConfResult result;
    std::string error_str;

    //result = m_producerConfig->set("group.id", "foobar", error_str);
    //if (result != RdKafka::Conf::CONF_OK)
    //{
    //    std::cout << "Global Conf set bootstrap.servers failed: " << error_str << std::endl;
    //}

    // 设置生产者待发送服务器的地址: "ip:port" 格式
    result = m_producerConfig->set("metadata.broker.list", m_brokers, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set bootstrap.servers failed: " << error_str << std::endl;
    }

    result = m_producerConfig->set("statistics.interval.ms", "10000", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'statistics.interval.ms' failed: " << error_str << std::endl;
    }


    result = m_producerConfig->set("sasl.mechanisms", "PLAIN", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'sasl.mechanisms=PLAIN' failed: " << error_str << std::endl;
    }

    result = m_producerConfig->set("sasl.username", m_user, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'sasl.username' failed: " << error_str << std::endl;
    }

    result = m_producerConfig->set("sasl.password", m_password, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'sasl.password' failed: " << error_str << std::endl;
    }

    result = m_producerConfig->set("api.version.request", "true", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'api.version.request=true' failed: " << error_str << std::endl;
    }
    

    // 设置发送端发送的最大字节数，如果发送的消息过大则返回失败
    result = m_producerConfig->set("message.max.bytes", "10240000", error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'message.max.bytes' failed: " << error_str << std::endl;
    }

    m_dr_cb = new ProducerDeliveryReportCb;
    result = m_producerConfig->set("dr_cb", m_dr_cb, error_str); // 设置每个消息发送后的发送结果回调
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'dr_cb' failed: " << error_str << std::endl;
    }

    m_event_cb = new ProducerEventCb;
    result = m_producerConfig->set("event_cb", m_event_cb, error_str);
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Global Conf set 'event_cb' failed: " << error_str << std::endl;
    }

    m_partitioner_cb = new HashPartitionerCb;
    result = m_topicConfig->set("partitioner_cb", m_partitioner_cb, error_str); // 设置自定义分区器
    if (result != RdKafka::Conf::CONF_OK)
    {
        std::cout << "Topic Conf set 'partitioner_cb' failed: " << error_str << std::endl;
    }

    // 创建Producer生产者客户端：
    // RdKafka::Producer::create(const RdKafka::Conf *conf, std::string &errstr);
    m_producer = RdKafka::Producer::create(m_producerConfig, error_str);
    if (m_producer == nullptr)
    {
        std::cout << "Create Producer failed: " << error_str << std::endl;
    }

    // 创建Topic对象，后续produce发送消息时需要使用
    // RdKafka::Topic::create(Hanle *base, const std::string &topic_str, const Conf *conf, std::string &errstr);
    m_topic = RdKafka::Topic::create(m_producer, m_topicStr, m_topicConfig, error_str);
    if (m_topic == nullptr)
    {
        std::cout << "Create Topic failed: " << error_str << std::endl;
    }
}

void KafkaProducer::pushMessage(const std::string &msg, const std::string &key)
{
    int32_t len = msg.length();
    void *payload = (void *)msg.c_str();
    int partition = RdKafka::Topic::PARTITION_UA;
    RdKafka::ErrorCode error_code = m_producer->produce(m_topic,
        partition,
                                RdKafka::Producer::RK_MSG_COPY,
                                payload, len, &key, NULL);
    m_producer->poll(0); // poll()参数为0意味着不阻塞；poll(0)主要是为了触发应用程序提供的回调函数
    if (error_code != RdKafka::ErrorCode::ERR_NO_ERROR)
    {
        std::cerr << "Produce failed: " << RdKafka::err2str(error_code) << std::endl;
        if (error_code == RdKafka::ErrorCode::ERR__QUEUE_FULL)
        {
            m_producer->poll(1000); // 如果发送失败的原因是队列正满，则阻塞等待一段时间
        }
        else if (error_code == RdKafka::ErrorCode::ERR_MSG_SIZE_TOO_LARGE)
        {

            // 如果发送消息过大，超过了max.size，则需要裁减后重新发送
        }
        else
        {
            std::cerr << "ERR_UNKNOWN_PARTITION or ERR_UNKNOWN_TOPIC" << std::endl;
        }
    }
}

KafkaProducer::~KafkaProducer()
{
    while (m_producer->outq_len() > 0)
    {
        // 当 Handle->outq_len() 客户端的“出队列” 的长度大于0
        std::cerr << "Waiting for: " << m_producer->outq_len() << std::endl;
        m_producer->flush(5000);
    }

    delete m_producerConfig;
    delete m_topicConfig;
    delete m_topic;
    delete m_producer;
    delete m_dr_cb;
    delete m_event_cb;
    delete m_partitioner_cb;
}
