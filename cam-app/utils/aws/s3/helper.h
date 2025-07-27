// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
// SPDX - License - Identifier: Apache - 2.0 

#pragma once

#define USE_S3 1
#ifdef USE_S3

//#include "FloderWatchMap.h"

#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/BucketLocationConstraint.h>
#include "../../log/BaseLog.h"



namespace AwsDoc {

    namespace S3 {
	class helper{
    public:
        helper(Aws::Client::ClientConfiguration config, Aws::String  appKey, Aws::String  sercetKey);
        ~helper();
    public:
        bool CopyObject(const Aws::String &objectKey, const Aws::String &fromBucket,
            const Aws::String &toBucket,
            const Aws::Client::ClientConfiguration &clientConfig);

        bool CreateBucket(const Aws::String &bucketName,
                          const Aws::Client::ClientConfiguration &clientConfig);

        bool DeleteObjects(const std::vector<Aws::String> &objectKeys,
                           const Aws::String &fromBucket,
                           const Aws::Client::ClientConfiguration &clientConfig);

        bool DeleteBucket(const Aws::String &bucketName,
                          const Aws::Client::ClientConfiguration &clientConfig);

        bool DeleteBucketPolicy(const Aws::String &bucketName,
                                const Aws::Client::ClientConfiguration &clientConfig);

        bool DeleteObject(const Aws::String &objectKey,
                          const Aws::String &fromBucket,
                          const Aws::Client::ClientConfiguration &clientConfig);

        bool DeleteBucketWebsite(const Aws::String &bucketName,
                                 const Aws::Client::ClientConfiguration &clientConfig);

        bool GetBucketAcl(const Aws::String &bucketName,
                          const Aws::Client::ClientConfiguration &clientConfig);

        bool GetBucketPolicy(const Aws::String &bucketName,
                             const Aws::Client::ClientConfiguration &clientConfig);

        bool GetObjectAcl(const Aws::String &bucketName,
                          const Aws::String &objectKey,
                          const Aws::Client::ClientConfiguration &clientConfig);

        bool GetObject(const Aws::String &objectKey,
                       const Aws::String &fromBucket,
                       const Aws::Client::ClientConfiguration &clientConfig);

        bool GetWebsiteConfig(const Aws::String &bucketName,
                              const Aws::Client::ClientConfiguration &clientConfig);

        bool ListBuckets(const Aws::Client::ClientConfiguration &clientConfig);


        bool ListObjects(const Aws::String &bucketName,
                         const Aws::Client::ClientConfiguration &clientConfig);


        bool PutBucketAcl(const Aws::String &bucketName,
                          const Aws::String &ownerID,
                          const Aws::String &granteePermission,
                          const Aws::String &granteeType,
                          const Aws::String &granteeID,
                          const Aws::Client::ClientConfiguration &clientConfig,
                          const Aws::String &granteeDisplayName = "",
                          const Aws::String &granteeEmailAddress = "",
                          const Aws::String &granteeURI = "");

        bool PutBucketPolicy(const Aws::String &bucketName,
                             const Aws::String &policyBody,
                             const Aws::Client::ClientConfiguration &clientConfig);

        bool PutObject(const Aws::String &bucketName,
                       const Aws::String &fileName,
                       const Aws::Client::ClientConfiguration &clientConfig);

        bool PutObjectAcl(const Aws::String &bucketName,
                          const Aws::String &objectKey,
                          const Aws::String &ownerID,
                          const Aws::String &granteePermission,
                          const Aws::String &granteeType,
                          const Aws::String &granteeID,
                          const Aws::Client::ClientConfiguration &clientConfig,
                          const Aws::String &granteeDisplayName = "",
                          const Aws::String &granteeEmailAddress = "",
                          const Aws::String &granteeURI = "");

        bool PutObjectAsync(const Aws::S3::S3Client &s3Client,
                            const Aws::String &bucketName,
                            const Aws::String &fileName);

        bool PutObjectBuffer(const Aws::String &bucketName,
                             const Aws::String &objectName,
                             const std::string &objectContent,
                             const Aws::Client::ClientConfiguration &clientConfig);

        bool PutWebsiteConfig(const Aws::String &bucketName,
                              const Aws::String &indexPage, const Aws::String &errorPage,
                              const Aws::Client::ClientConfiguration &clientConfig);



        Aws::String GetGranteeTypeString(const Aws::S3::Model::Type& type);

        Aws::String GetPermissionString(const Aws::S3::Model::Permission& permission);

        Aws::S3::Model::Permission SetGranteePermission(const Aws::String& access);

        Aws::S3::Model::Type SetGranteeType(const Aws::String& type);

        bool uploadfile(std::string BucketName, std::string objectKey, std::string pathkey);
        bool downloadfile(std::string BucketName, std::string objectKey, std::string pathkey);

        //bool starWatch(std::string BucketName, std::string nodeid, std::string pathkey);
        //bool stopWatch(std::string BucketName, std::string nodeid, std::string pathkey);
        //bool listWatch();

    public:

        Aws::String  app_key;

        Aws::String  sercet_key;

        Aws::Client::ClientConfiguration config;

        Aws::S3::S3Client *client;

        Aws::SDKOptions options;

        //static FloderWatchMap * floderWatchMap_;

        

    public:

        static void PutObjectAsyncFinished(const Aws::S3::S3Client* s3Client,
            const Aws::S3::Model::PutObjectRequest& request,
            const Aws::S3::Model::PutObjectOutcome& outcome,
            const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context);

        static std::mutex upload_mutex;

        static std::condition_variable upload_variable;
	};
    }; // class S3
}; // namespace AwsDoc

#endif
