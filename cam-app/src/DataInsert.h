#pragma once
#include<stdlib.h>
#include<iostream>
#include "proto/v2x.pb.h"
#include "proto/MultiPathDatas.pb.h"
#include "../db/DbBase.h"
#include "rd.h"
#include "../utils/SnowFlake.h"
//还需引入rd.cpp相关的数据库头文件

using namespace cn::seisys::v2x::pb;
using namespace cn::seisys::rbx::comm::bean::multi;
using namespace std;


class DataInsert
{
public:
    std::tm* gettm(long long timestamp);
    void camInsert(CamData camData, DbBase* db, SnowFlake* g_sf);//还需要数据连接后的一个类型参数
    void cameraInsert(MultiPathDatas cameraData, DbBase* db, SnowFlake* g_sf);
    void radarInsert(MultiPathDatas radarData, DbBase* db, SnowFlake* g_sf);
private:
    SnowFlake* g_sf_;
};

class DataMap
{
public:
    map<string,string>* deviceCodeMap(std::map<string, string>& devMap, DbBase* db);
    map<string, string>* nodeMatchMap(std::map<string, string>& nodeMap, DbBase* db);
};