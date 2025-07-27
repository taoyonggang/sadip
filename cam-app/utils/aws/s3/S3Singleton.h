#pragma once
#include "../../utils/aws/s3/helper.h"
#include "../utils/log/BaseLog.h"

//#include "FloderWatchMap.h"
//#include "FileWatcher.h"

class S3Singleton   //实现单例模式的类  
{
private:
    S3Singleton() = default;

	~S3Singleton() = default;

	bool hasInit = false;

public:
	S3Singleton(const S3Singleton&) = delete;
	S3Singleton(S3Singleton&&) = delete;
	S3Singleton& operator=(const S3Singleton&) = delete;
	S3Singleton& operator=(S3Singleton&&) = delete;

	bool init(std::string endpointUrl, std::string  accessKey, std::string secretKey);
	void uinit();

public:
	static S3Singleton* getInstance()
	{
		static S3Singleton instance;
		return &instance;
	}

public:

	int sleepSecond = 60;  //间隔时间

#ifdef USE_S3
	Aws::SDKOptions *options;
	Aws::Client::ClientConfiguration *cfg;
    AwsDoc::S3::helper* s3_helper_;
	//static FileWatcher* fileWatcher_;
#endif


};

