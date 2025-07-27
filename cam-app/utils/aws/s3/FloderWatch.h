#pragma once
#include <string>
#include "FileWatcher.h"
using namespace std;

class FloderWatch {
public:
    string nodeid;
    string bucket;
    bool keep = 1;
    bool isRuning = 0;

    FileWatcher* fileWatcher_ = nullptr;


    FloderWatch(const string& nodeid, const string& bucket, const bool keep) : nodeid(nodeid), bucket(bucket), keep(keep) {};
    FloderWatch() {};
};
