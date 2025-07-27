#include "S3Singleton.h"


bool S3Singleton::init(std::string endpointUrl, std::string  accessKey, std::string secretKey) {
	if (!hasInit) {
#ifdef USE_S3
		INFOLOG("s3 sdk init");
		options = new Aws::SDKOptions();
		options->cryptoOptions.initAndCleanupOpenSSL = false;
		//options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Trace;
		Aws::InitAPI(*options);
		// Optional: Set to the AWS Region in which the bucket was created (overrides config file).
		// clientConfig.region = "us-east-1";
		cfg = new Aws::Client::ClientConfiguration();
		cfg->endpointOverride = endpointUrl;//"192.168.1.70:9001"
		cfg->scheme = Aws::Http::Scheme::HTTP;
		cfg->verifySSL = false;
		s3_helper_ = new AwsDoc::S3::helper(*cfg, accessKey, secretKey);// "app-key", "secret-key"
		if (s3_helper_ != nullptr)
			hasInit = true;
#endif
	}
	return hasInit;
}

void S3Singleton::uinit() {
#ifdef USE_S3
	if (hasInit) {
		delete s3_helper_;
		Aws::ShutdownAPI(*options);
		delete options;
		delete cfg;


	}
#endif
}