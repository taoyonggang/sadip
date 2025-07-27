#ifndef PROCESS_MANAGER_HPP
#define PROCESS_MANAGER_HPP

#ifdef _HAS_STD_BYTE
#undef _HAS_STD_BYTE
#endif
#define _HAS_STD_BYTE 0

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#endif

#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <string>
#include <vector>
#include <iostream>
#include <ctime>


namespace bp = boost::process;

/**
@class ProcessAttribute

@brief 进程属性.

@member
    exception_flag:异常标志.
    latest_stop_time:进程最后停止运行时间.
    launch_cmd:启动进程命令.
    uuid:进程唯一标识.
    reboot_count:重启计数.
    env:环境变量.
    instance:进程的实例.
 **/
struct ProcessAttribute {
    bool except_runnig = false;  //期望运行
    bool exception_flag;
    time_t latest_start_time;
    time_t latest_stop_time;
    std::string launch_cmd;
    std::string uuid;
    std::string work_path;
    uint32_t reboot_count = 0;
    double cpu_usage = 0.0;
    double mem_usage = 0.0;
    bp::environment env;
    bp::child instance;
};

class ProcessManager {
public:
    explicit ProcessManager();

    ~ProcessManager();

    bool add_process(std::string uuid, std::string runPath, std::string runFile, bool keepAlive = true);

    bool remove_process(std::string uuid);

    void start_all_process();

    bool start_process(std::string uuid);

    bool reboot_all_process();

    bool reboot_process(std::string uuid);

    bool listener();

    bool kill_process(const std::string &uuid);

    bool kill_all_process();
    
    bool ls_process();

    double getCpuUsage(int pid);
    double getMemoryUsage(int pid);

private:


    static time_t get_current_sec() {
        time_t cur_time;
        return time(&cur_time);
    }

public:
    std::vector<ProcessAttribute> m_instances{};

};

#endif //PROCESS_MANAGER_HPP

