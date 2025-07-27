#pragma once
//#include "SnowFlake.h"
#include <iostream>
//#include "ini/INIReader.h"

using namespace std;

const int    MAX_MSG = 20000;
const string MQTT_SERVER_ADDRESS    { "tcp://localhost:1883" };
const string CLIENT_ID              { "test1" };
const string PERSIST_DIR            { "./persist" };
const string TOPIC                  { "hello" };
const string USER                   { "user" };
const string PASSWORD               { "password " };
//const char* LWT_PAYLOAD = "Last will and testament.";
const int  MQTT_QOS = 1;
const int	N_RETRY_ATTEMPTS = 5;
//int g_datacenter_id = 0;  
//int g_machine_id = 0;
//string g_filename = "";
//SnowFlake *g_sf;
//INIReader *g_reader;
//
//inline int init_config(string filename)
//{
//    //read config
//    g_reader =  new INIReader(filename);
//    if (g_reader->ParseError() != 0) {
//        std::cout << "Can't load 'conf' " << filename << endl;
//        return 1;
//    }
//    g_datacenter_id = g_reader->GetInteger("gid","datacenter_id",0);
//    g_machine_id =  g_reader->GetInteger("gid","machine_id",0);
//    g_sf = new SnowFlake(g_datacenter_id,g_machine_id);
//
//}
