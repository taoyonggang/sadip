#pragma once

#include <iostream>
#include <soci/soci.h>
#include <soci/mysql/soci-mysql.h>



using namespace std;
using namespace soci;


class Idb {
public:
    virtual ~Idb() = default;  // 添加虚析构函数

    virtual int init() = 0;
    virtual int uInit() = 0;
    virtual void checkConnections(bool needReconnect = true) = 0;
    virtual soci::rowset<soci::row> select(std::string sql) = 0;
    virtual int excuteSql(std::string sql) = 0;
};