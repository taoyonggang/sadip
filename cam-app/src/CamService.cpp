

#include "UserTcpServer.h"
#include "../db/DbBase.h"
#include "../utils/ini/INIReader.h"
#include "../utils/split.h"
//#include "../db/DbBase.h"
#include "CamTopic.h"
#include "../utils/nodeInfo.h"
#include "../utils/log/BaseLog.h"
#include "../utils/config/ConfigMap.h"
//#include "MqttSubscriber.h"
#include "MqttThreadClass.h"
#include "../utils/common.h"
#include "DataInsert.h"
//#include "RteDistributionTopic.h"
//#include "RtsDistributionTopic.h"
//#include"SimplifiedCamTopic.h"

#include <iostream>
#include <set>

//#include "../../third_party/kafka/KafkaConsumer.h"
//#include "../../third_party/kafka/KafkaProducer.h"


using namespace std;

map<string, string> deviceIdMap;
map<string, string> nodeIdMap;
DbBase* DbBase::dbInstance_ = NULL;

NodeInfo utility::node::nodeInfo_;



int main(int argc, char** argv) {


     INIReader nodeConfig(argv[2]);

     if (nodeConfig.ParseError() < 0) {
         std::cout << "Can't load ini:" << argv[2] << endl;
         return -1;
     }

     spdlog::level::level_enum mode = spdlog::level::info;
     int logMode = nodeConfig.GetInteger("base", "log_level", 2);
     mode = (spdlog::level::level_enum)logMode;

     INITLOG("logs/cam-app.log", "cam-sink",mode);


     INFOLOG("Start Cam App");

    MapWrapper confMap;
    DataMap dataMap;

    int groupCount = 0;
    int domain = nodeConfig.GetInteger("base", "domain", 0);
    string name=nodeConfig.GetString("base", "name", "");
    string nodeId = nodeConfig.GetString("base", "node_id", "");
    string group = nodeConfig.GetString("base", "group", "");
    string location = nodeConfig.GetString("base", "location", "");
    string version = nodeConfig.GetString("base", "version", "");
    string progarmName = nodeConfig.GetString("base", "program_name", "cam-app");
    string camTopic= nodeConfig.GetString("route", "cam_topic", "memory/shuangzhi/v2x/edge/000000000000/comprehensive/camdata/up");
    int camType = nodeConfig.GetInteger("mec", "type", 1);
    string camVer = nodeConfig.GetString("mec", "version", "01");
    int sceneType = nodeConfig.GetInteger("mec", "scene_type", 0);
    string deviceId = nodeConfig.GetString("mec", "device_id", "");
    string mapDevId = nodeConfig.GetString("mec", "map_dev_id", "");
    int mecLat = nodeConfig.GetInteger("mec", "lat", 0);
    int mecLon = nodeConfig.GetInteger("mec", "lon", 0);
    int mecEle = nodeConfig.GetInteger("mec", "ele", 0);


    //初始化configMap
    groupCount = nodeConfig.GetInteger("group", "count", 1);
    for (int i = 0; i < groupCount; i++)
    {
        string groupName = "group_name_" + to_string(i);
        string groupMember = "group_member_" + to_string(i);
        string mapKey = nodeConfig.GetString("group", groupName, "");
        string mapValue = nodeConfig.GetString("group", groupMember, "");
        confMap.addElement(mapKey, mapValue);
    }

    string zenohConf = nodeConfig.GetString("route", "zenoh_conf", "config/config1.json5");
    string mqttSubAddress = nodeConfig.GetString("mqtt_sub", "address", MQTT_SERVER_ADDRESS);
    string subClientId = nodeConfig.GetString("mqtt_sub", "sub_client_id", CLIENT_ID);
    string persist = nodeConfig.GetString("mqtt_sub", "persist", PERSIST_DIR);
    string camV2xTopic = nodeConfig.GetString("mqtt_sub", "cam_topic", TOPIC);
    string subUserName = nodeConfig.GetString("mqtt_sub", "namename", "admin");
    string subPwd = nodeConfig.GetString("mqtt_sub", "password", "public");
    bool mqttCleanSession = nodeConfig.GetBoolean("mqtt_sub", "cleansession", true);
    int mqttRecvFlag = nodeConfig.GetInteger("mqtt_sub", "recv_flag", 0);
    string monNodeId = nodeConfig.GetString("monitor", "node_id", "");
    string monTopic= nodeConfig.GetString("monitor", "topic", "DataMonitor");
    int monSendSleep = nodeConfig.GetInteger("monitor", "send_sleep", 60);
    int matchineId = nodeConfig.GetInteger("gid","machine_id",1);
    int dataCenterId= nodeConfig.GetInteger("gid", "datacenter_id", 1);


    utility::node::nodeInfo_.domain_ = domain;
    utility::node::nodeInfo_.group_ = group;
    utility::node::nodeInfo_.location_ = location;
    utility::node::nodeInfo_.nodeId_ = nodeId;
    utility::node::nodeInfo_.nodeName_ = name;
    utility::node::nodeInfo_.version_ = version;
    utility::node::nodeInfo_.camType_ = camType;
    utility::node::nodeInfo_.camVer_ = camVer;
    utility::node::nodeInfo_.sceneType_ = sceneType;
    utility::node::nodeInfo_.deviceId_ = deviceId;
    utility::node::nodeInfo_.mapDevId_ = mapDevId;
    utility::node::nodeInfo_.lat_ = mecLat;
    utility::node::nodeInfo_.lon_ = mecLon;
    utility::node::nodeInfo_.ele_ = mecEle;

    //管理节点数据库连接

    if (location == "mgr")
    {
        int dbType = nodeConfig.GetInteger("db", "type", 1);
        string dbIp = nodeConfig.GetString("db", "ip", "10.4.1.203");
        string dbUser = nodeConfig.GetString("db", "user", "root");
        string dbPwd = nodeConfig.GetString("db", "pwd", "Seisys*77889900");
        string dbName = nodeConfig.GetString("db", "name", "databus-dev");
        int dbPort = nodeConfig.GetInteger("db", "port", 3306);
        int dbPoolNum = nodeConfig.GetInteger("db", "pool_num", 10);
        DbBase* db = DbBase::getDbInstance(dbType, dbIp, dbUser, dbPwd, dbName, dbPort, dbPoolNum);

        ////下发设备映射表
        //dataMap.deviceCodeMap(deviceIdMap, db);
        ////节点设备匹配映射表
        //dataMap.nodeMatchMap(nodeIdMap, db);
    }

    //云端只负责接收数据,不需要回执,边缘节点只负责发,不需要回执
    CamTopic* cam = NULL;
    TopicInfo recvCam(utility::node::nodeInfo_);
    recvCam.nodeName_ = name;
    recvCam.nodeId_ = nodeId;
    recvCam.domain_ = domain;
    recvCam.needReply_ = true;
    recvCam.topic_ = camTopic;
    recvCam.machineId_ = matchineId;
    recvCam.datacenterId_ = dataCenterId;
    recvCam.configPath_ = zenohConf;

    TopicInfo sendCam(utility::node::nodeInfo_);
    sendCam.nodeName_ = name;
    sendCam.nodeId_ = nodeId;
    sendCam.domain_ = domain;
    sendCam.needReply_ = false;
    sendCam.topic_ = camTopic;
    sendCam.configPath_ = zenohConf;

    MqttThreadClass mqttSub;
    MQTTCONF mqttSubConf;
    mqttSubConf.mqttAddr = mqttSubAddress;
    mqttSubConf.clientId = subClientId;
    mqttSubConf.sessionFlag = mqttCleanSession;
    mqttSubConf.user = subUserName;
    mqttSubConf.pwd = subPwd;
    mqttSubConf.camTopic = camV2xTopic;
    if (location == "mgr")
    {
            cam = new CamTopic(recvCam);
            cam->start_reader(false);
         
    }
    else
    {
        //边缘节点接收融合数据,并发送管理节点
        // mqtt接收数据
        if (mqttRecvFlag > 0)
        {
            mqttSub.mqtt_start_sub(mqttSubConf);
        }
        // zenoh发送tcp接收以及mqtt接收融合感知数据
        cam = new CamTopic(sendCam);
        cam->start_writer(false);

        //tcp接收融合数据
        int server_port = nodeConfig.GetInteger("tcp", "port", 18089);
        std::string server_ip=nodeConfig.GetString("tcp","address","127.0.0.1");//"localhost"
        INFOLOG("server_ip: {}, server_port: {}", server_ip, server_port);
        Poco::Net::SocketAddress socket_addr(server_ip.c_str(), server_port);

        UserTcpServer app(socket_addr);
        return app.run(1, argv);
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));

        INFOLOG("Cam App is running ...");
    }
    return 0;
}
