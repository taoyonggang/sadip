#pragma once
#include <string>
#include<vector>

typedef struct process {
	int state;
	std::string path;
	std::string cmd;
	std::string paras;
	std::string add_paras1;
	std::string add_paras2;
	bool keep_live;
}PROCESS;

typedef struct s3WatchStruct
{
	std::string bucket;
	std::string directory;
	int sleep = 30;
	bool keepFile = 1;
	bool zipFlag = 1;
	bool watchSubDir = 1;
}S3WATCH;
class NodeInfo
{
public:
	//数据域
	int domain_;
	//节点id
	std::string nodeId_;	
	//节点名
	std::string nodeName_;
	//节点组
	std::string group_;
	//节点位置
	std::string location_;
	//版本 
	std::string version_;
	// 架构
	std::string archType_;
	// 操作系统
	std::string osType_;
	// 文件发送最大失败次数
	int fileMaxReTry_;
	// 文件块超时（非最后一块）
	int fileBlockTimeout_;
	// 文件最后一块超时
	int fileBlockLastTimeout_;
	// 文件分块大小
	int fileSplitSize_;

	//mec的基础配置信息
	int camType_;
	// cam消息中版本号
	std::string camVer_;
	// mec_device_id
	std::string deviceId_;
	// mec_map_device_id;
	std::string mapDevId_;
	// 道路场地类型
	int sceneType_;
	// mec_lat 纬度
	int lat_;
	// mec_lon 经度
	int lon_;
	//mec_ele 高度
	int ele_;
	//发现服务配置
	std::string dcps_global_transport_config_ = "config1";
	//#服务发现角色, 1为服务端, 2为客户端,3=rtsp-sed
	int dcps_role_ = 3;
	//#发现服务id
	int dcps_server_id_ = 1;
	//#服务发现类型,0 = local, 1 = wan, 3 = internet
	int dcps_info_location_type_ = 0;
	std::string dcps_info_repo_ = "44.53.00.5f.45.50.52.4f.53.49.4d.41";
	std::string dcps_info_ip_ = "10.10.3.30";
	int dcps_info_port_ = 14521;
	std::string dcps_info_remote_ip_ = "10.10.3.30";
	int dcps_info_remote_port_ = 14520;
	int dcps_duration_ = 250000000;

	// s3的endpointOverride
	std::string endpointOverride;
	//s3的access_key
	std::string accessKey;
	// s3的secret_key
	std::string secretKey;

	std::vector<PROCESS> processVec_;

	std::vector<S3WATCH> s3WatchVec_;

	std::string programName_;


	
};

