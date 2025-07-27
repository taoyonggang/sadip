#include "../../utils/pch.h"
#include "MachineStateTopic.h"
#include "../utils/uuid.h"
#include "../utils/log/BaseLog.h"
#include "../db/DbBase.h"
#include "NodeCmdWorker.h"
#include <chrono>
#include <thread>
#include <hwinfo/hwinfo.h>
#include <hwinfo/utils/PCIMapper.h>

using namespace std::chrono;
using namespace cn::seisys::dds;

MachineStateTopic::MachineStateTopic(const TopicInfo& topic_info) :
    topic_info_(topic_info) {
}

MachineStateTopic::~MachineStateTopic() {
    is_writer_ = false;
    is_reader_ = false;
}


void MachineStateTopic::start_writer(bool wait) {
    
    if (!writer_thread_) {
        is_writer_ = true;
        writer_thread_ = std::make_unique<std::thread>(&MachineStateTopic::writer_worker, this);
    }

    if (wait && writer_thread_ && writer_thread_->joinable()) {
        writer_thread_->join();
    }

    
}

void MachineStateTopic::writer_worker() {

    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize machstate writer base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化管理节点发布者
        if (!state_publisher_) {
            state_publisher_ = std::make_shared<zenoh::Publisher>(
                session_->declare_publisher(topic_info_.topic_)
            );
        }
    }
    catch (const std::exception& e)
    {
        ERRORLOG("Init machstate writer failed: {}", e.what());
        return;
    }

    while (is_writer_) {
        try {
            MachineState state_msg;
            uint64_t now = duration_cast<milliseconds>(
                system_clock::now().time_since_epoch()).count();

            //DEBUGLOG("Collecting machine state at {}", now);

            state_msg.set_src_node_id(topic_info_.nodeId_);
            state_msg.set_to_node_id(topic_info_.monNodeId_);

            if (get_system_info(state_msg) == 0) {
                if (send_machine_state(state_msg)) {
                    INFOLOG("Machine state sent successfully at {}", now);
                }
            }
        }
        catch (const std::exception& e) {
            ERRORLOG("Writer worker error: {}", e.what());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(30000));
    }
}

void MachineStateTopic::start_reader(bool wait) {

    if (!reader_thread_) {
        is_reader_ = true;
        reader_thread_ = std::make_unique<std::thread>(&MachineStateTopic::reader_worker, this);
    }

    if (wait && reader_thread_ && reader_thread_->joinable()) {
        reader_thread_->join();
    }

}

void MachineStateTopic::reader_worker() {
    try
    {
        // 首先调用父类的init初始化连接信息
        if (!ZenohBase::init(topic_info_)) {
            ERRORLOG("Failed to initialize  machstate reader base ZenohBase");
            return;
        }

        //创建监控发送端
        ZenohBase::create_writer_mon_pub();

        // 初始化订阅者（NodeTopic）只有订阅，不需要回执
        zenoh::KeyExpr key(topic_info_.topic_);
        auto on_node = [this](const zenoh::Sample& sample) {
            handle_state_data(sample);
            };

        auto on_drop_node = []() {
            DEBUGLOG("MachineState Subscriber dropped");
            };

        subscriber_.emplace(session_->declare_subscriber(
            key,
            std::move(on_node),
            std::move(on_drop_node)
        ));
    }
    catch (const std::exception& e)
    {
        ERRORLOG("Init machinestate reader worker error: {}", e.what());
    }


    while (is_reader_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10000));
    }
}

bool MachineStateTopic::send_machine_state(const MachineState& state) {
    try {
        std::vector<uint8_t> serialized(state.ByteSizeLong() + 1);
        if (!state.SerializeToArray(serialized.data(), state.ByteSizeLong())) {
            ERRORLOG("Failed to serialize machine state");
            return false;
        }    
        state_publisher_->put(serialized);

        //构建统计发送端监控数据
        stat_writer_mon_data(state.ByteSize(), state.to_node_id());

        return true;
    }
    catch (const std::exception& e) {
        ERRORLOG("Send machine state failed: {}", e.what());
        return false;
    }
}

void MachineStateTopic::handle_state_data(const zenoh::Sample& sample) {
    try {
        //auto data = sample.get_payload().as_vector();

        std::vector<uint8_t> data = sample.get_payload().as_vector();
        size_t size = sample.get_payload().size();
        MachineState state;
        state.ParsePartialFromArray(data.data(), size);

        stat_reader_mon_data(topic_info_.topic_, state.src_node_id(), state.ByteSize());

        //机器状态数据入库处理
        DbBase* db = DbBase::getDbInstance();

        string uuid = utility::uuid::generate();
        uint64_t now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
        int ret = -1;
        char machineStatSql[2048];
        int64_t usedRam = state.rams().size() - state.rams().free();
        sprintf(machineStatSql, "insert into machine_state(node_id,main_board_vendor,main_board_name,main_board_version,main_board_serial_number,operating_system,\
            os_short_name,os_version,os_kernel,os_architecture,os_endianess,uuid,updated_at,ram_vendor,ram_model,ram_name,ram_serial_number,ram_size,ram_free,ram_available,ram_used)value('%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s','%s',%llu,'%s','%s','%s','%s',%ld,%ld,%ld,%ld)", state.src_node_id().c_str(), state.main_board_infos().vendor().c_str(), state.main_board_infos().name().c_str(), \
            state.main_board_infos().version().c_str(), state.main_board_infos().serial_number().c_str(), state.os_infos().operating_system().c_str(), state.os_infos().short_name().c_str(), state.os_infos().version().c_str(), state.os_infos().kernel().c_str(), \
            state.os_infos().architecture().c_str(), state.os_infos().endianess().c_str(), uuid.c_str(), now, state.rams().vendor().c_str(), state.rams().model().c_str(), \
            state.rams().name().c_str(), state.rams().serial_number().c_str(), state.rams().size(), state.rams().free(), state.rams().available(), usedRam);
        ret = db->excuteSql(machineStatSql);
        if (ret < 0)
        {
            ERRORLOG("Insert MachineStatSql:{} Fail", machineStatSql);
        }
        //cpu
        //std::vector<cn::seisys::dds::Cpu> cpus_ = machineState.cpus();
        for (int i = 0; i < state.cpus_size(); ++i)
        {
            DEBUGLOG("Before cpu frequence insert,Cpu max frequence:{},cpu min frequence:{},cpu regular frequence:{},cpu current frequence size:{}", state.cpus(i).max_frequency(), state.cpus(i).min_frequency(), state.cpus(i).regular_frequency(), state.cpus(i).current_frequency().size());
            if (state.cpus(i).current_frequency().size() != 0)
            {
                DEBUGLOG("Cpu first current frequence:{}", state.cpus(i).current_frequency()[0]);
            }
            string curFreq = "";
            int size = state.cpus(i).current_frequency().size();
            for (int j = 0; j < size; j++)
            {
                if (j == 0)
                {
                    curFreq = (to_string)(state.cpus(i).current_frequency()[j]);
                }
                else {
                    curFreq = curFreq + "," + (to_string)(state.cpus(i).current_frequency()[j]);
                }

            }
            // curFreq = curFreq / size;
            char cpuSql[4096];
            sprintf(cpuSql, "insert into node_cpu(node_id,cpu_number,cpu_vendor,cpu_model,cpu_physical_cores,cpu_logical_cores,cpu_min_frequency,cpu_max_frequency,cpu_regular_frequency,\
                cpu_current_frequency,cpu_cache_size,cpu_usage_average,machine_state_uuid,updated_at)value('%s',%d,'%s','%s',%d,%d,%lld,%lld,%lld,'%s',%lld,%lf,'%s',%llu)", state.src_node_id().c_str(), i, state.cpus(i).vendor().c_str(), state.cpus(i).model().c_str(), \
                state.cpus(i).physical_cores(), state.cpus(i).logical_cores(), state.cpus(i).min_frequency(), state.cpus(i).max_frequency(), state.cpus(i).regular_frequency(), curFreq.c_str(), state.cpus(i).cache_size(), state.cpus(i).cpu_usage(), uuid.c_str(), now);

            ret = db->excuteSql(cpuSql);
            if (ret < 0)
            {
                ERRORLOG("Insert cpuSql:{} Fail", cpuSql);
            }
        }

        //gpu
        //std::vector<cn::seisys::dds::Gpu> gpus_ = machineState.gpus();
        for (int i = 0; i < state.gpus_size(); ++i)
        {
            char gpuSql[4096];
            sprintf(gpuSql, "insert into node_gpu(node_id,gpu_number,gpu_vendor,gpu_model,driver_version,gpu_memory,gpu_frequency,machine_state_uuid,updated_at)value('%s',%d,'%s','%s','%s',%lf,%d,'%s',%llu)", state.src_node_id().c_str(), i, state.gpus(i).vendor().c_str(), \
                state.gpus(i).model().c_str(), state.gpus(i).driver_version().c_str(), state.gpus(i).memory(), state.gpus(i).frequency(), uuid.c_str(), now);

            ret = db->excuteSql(gpuSql);
            if (ret < 0)
            {
                ERRORLOG("Insert gpuSql:{} Fail", gpuSql);
            }
        }

        //disk
        //std::vector<cn::seisys::dds::Disk> disk_ = machineState.disks();
        for (int i = 0; i < state.disks_size(); ++i)
        {
            char diskSql[4096];
            sprintf(diskSql, "insert into node_disks(node_id,disks_number,disks_vendor,disks_model,disks_serial_number,disks_size,machine_state_uuid,updated_at)value('%s',%d,'%s','%s','%s',%lld,'%s',%llu)", state.src_node_id().c_str(), i, state.disks(i).vendor().c_str(), \
                state.disks(i).model().c_str(), state.disks(i).serial_number().c_str(), state.disks(i).size(), uuid.c_str(), now);

            ret = db->excuteSql(diskSql);
            if (ret < 0)
            {
                ERRORLOG("Insert diskSql:{} Fail", diskSql);
            }
        }

        // Network
        //std::vector<cn::seisys::dds::Network> networks_ = machineState.networks();
        for (int i = 0; i < state.networks_size(); ++i)
        {
            char networksSql[4096];
            sprintf(networksSql, "insert into node_network(node_id,network_number,network_name,ipv4,ipv6,broadband,broadband_using,machine_state_uuid,updated_at)value('%s',%d,'%s','%s','%s',%lld,%lld,'%s',%llu)", state.src_node_id().c_str(), i, state.networks(i).name().c_str(), \
                state.networks(i).ipv4s().c_str(), state.networks(i).ipv6s().c_str(), state.networks(i).broadband(), state.networks(i).broadband_using(), uuid.c_str(), now);

            ret = db->excuteSql(networksSql);
            if (ret < 0)
            {
                ERRORLOG("Insert networksSql:{} Fail", networksSql);
            }
        }

        //  process
        //std::vector<cn::seisys::dds::Process> processes_ = machineState.processes();
        size_t processNums = state.processes_size();
        DEBUGLOG("processNum:{}", processNums);
        for (int i = 0; i < processNums; i++)
        {
            char processSql[4096];
            DEBUGLOG("processes_[{}].cpuUsage:{},processes_[{}].memUsage:{}", i, state.processes(i).cpu_usage(), i, state.processes(i).mem_usage());
            sprintf(processSql, "insert into node_process(node_id,machine_state_uuid,name,work_path,pid,args,cpu_usage,mem_usage,start_time,stop_time,reboot_count,priority)value('%s','%s','%s','%s',%d,'%s',%lf,%lf,%lld,%lld,%d,%d)", state.src_node_id().c_str(), uuid.c_str(), state.processes(i).name().c_str(), state.processes(i).work_path().c_str(), \
                state.processes(i).pid(), state.processes(i).args().c_str(), state.processes(i).cpu_usage(), state.processes(i).mem_usage(), state.processes(i).start_time(), state.processes(i).stop_time(), state.processes(i).reboot_count(), state.processes(i).priority());

            ret = db->excuteSql(processSql);
            if (ret < 0)
            {
                ERRORLOG("Insert processSql:{} Fail", processSql);
            }
        }


       /* {
            std::lock_guard<std::mutex> lock(queue_mutex_);
            state_queue_.push(std::move(state));
        }*/
    }
    catch (const std::exception& e) {
        ERRORLOG("Handle state data failed: {}", e.what());
    }
}


int MachineStateTopic::get_system_info(MachineState& msg) {
    std::lock_guard<std::mutex> guard(mon_mutex_);

    try {
        // CPU 信息收集
        auto sockets = hwinfo::getAllCPUs();
        for (const auto& cpu : sockets) {
            auto dds_cpu = msg.add_cpus();
            dds_cpu->set_vendor(cpu.vendor());
            dds_cpu->set_model(cpu.modelName());
            dds_cpu->set_physical_cores(cpu.numPhysicalCores());
            dds_cpu->set_logical_cores(cpu.numLogicalCores());
            dds_cpu->set_max_frequency(cpu.maxClockSpeed_MHz());
            dds_cpu->set_regular_frequency(cpu.regularClockSpeed_MHz());
            dds_cpu->set_min_frequency(cpu.regularClockSpeed_MHz());
            if (!cpu.currentClockSpeed_MHz().empty()) {
                // 获取当前频率
                std::vector<int64_t> cur_freq_vec = cpu.currentClockSpeed_MHz();

                // 赋值给current_frequency
                for (size_t i = 0; i < cur_freq_vec.size(); i++) {
                    dds_cpu->add_current_frequency(cur_freq_vec[i]);
                }
                //dds_cpu->set_current_frequency(0,cpu.currentClockSpeed_MHz()[0]);
            }
            
            dds_cpu->set_cache_size(cpu.L1CacheSize_Bytes() +
                cpu.L2CacheSize_Bytes() + cpu.L3CacheSize_Bytes());
            dds_cpu->set_cpu_usage(cpu.currentUtilisation());
        }

        // OS 信息收集
        hwinfo::OS os;
        auto* os_info = msg.mutable_os_infos();
        os_info->set_operating_system(os.name());
        os_info->set_short_name(os.name());
        os_info->set_version(os.version());
        os_info->set_kernel(os.kernel());
        os_info->set_architecture(os.is32bit() ? "32 bit" : "64 bit");
        os_info->set_endianess(os.isLittleEndian() ? "little endian" : "big endian");

        // GPU 信息收集
        auto gpus = hwinfo::getAllGPUs();
        for (const auto& gpu : gpus) {
            auto* dds_gpu = msg.add_gpus();
            dds_gpu->set_vendor(gpu.vendor());
            dds_gpu->set_model(gpu.name());
            dds_gpu->set_driver_version(gpu.driverVersion());
            dds_gpu->set_memory(static_cast<double>(gpu.memory_Bytes()) /
                1024.0 / 1024.0);
            dds_gpu->set_frequency(gpu.frequency_MHz());
        }

        // RAM 信息收集
        hwinfo::Memory ram;
        auto* dds_ram = msg.mutable_rams();
        dds_ram->set_size(ram.total_Bytes() / 1024 / 1024);
        dds_ram->set_free(ram.free_Bytes() / 1024 / 1024);
        dds_ram->set_available(ram.available_Bytes() / 1024 / 1024);

        // 主板信息收集
        hwinfo::MainBoard main_board;
        auto* dds_board = msg.mutable_main_board_infos();
        dds_board->set_vendor(main_board.vendor());
        dds_board->set_name(main_board.name());
        dds_board->set_version(main_board.version());
        dds_board->set_serial_number(main_board.serialNumber());

        // 磁盘信息收集
        auto disks = hwinfo::getAllDisks();
        for (const auto& disk : disks) {
            auto* dds_disk = msg.add_disks();
            dds_disk->set_vendor(disk.vendor());
            dds_disk->set_model(disk.model());
            dds_disk->set_serial_number(disk.serialNumber());
            dds_disk->set_size(disk.size_Bytes());
        }

        // 进程信息收集
        if (NodeCmdWorker::pm_) {
            NodeCmdWorker::pm_->listener();
            for (const auto& process : NodeCmdWorker::pm_->m_instances) {
                auto* p = msg.add_processes();
                p->set_pid(process.instance.id());
                p->set_name(process.uuid);
                p->set_cpu_usage(process.cpu_usage);
                p->set_mem_usage(process.mem_usage);
                p->set_work_path(process.work_path);
                p->set_start_time(process.latest_start_time);
                p->set_stop_time(process.latest_stop_time);
                p->set_reboot_count(process.reboot_count);
            }
        }

        return 0;
    }
    catch (const std::exception& e) {
        ERRORLOG("Get system info failed: {}", e.what());
        return -1;
    }
}

//void MachineStateTopic::NodePubListener::on_matched(
//    const zenoh::PublicationInfo& info) {
//    matched_ = info.matched_subscribers;
//    firstConnected_ = true;
//    INFOLOG("Publisher matched with {} subscribers", matched_);
//}
//
//void MachineStateTopic::NodeReplyPubListener::on_matched(
//    const zenoh::PublicationInfo& info) {
//    matched_ = info.matched_subscribers;
//    firstConnected_ = true;
//    INFOLOG("Reply Publisher matched with {} subscribers", matched_);
//}