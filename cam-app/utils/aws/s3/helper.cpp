#define USE_S3 1
#include "../../utils/pch.h"
#include "helper.h"
//#ifdef USE_S3

#include <iostream>
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/s3/model/CopyObjectRequest.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/CreateBucketRequest.h>
#include <aws/s3/model/DeleteBucketRequest.h>
#include <aws/s3/model/DeleteBucketPolicyRequest.h>
#include <aws/s3/model/BucketLocationConstraint.h>
#include <aws/s3/model/DeleteObjectRequest.h>
#include <aws/s3/model/DeleteObjectsRequest.h>
#include <aws/s3/model/DeleteBucketWebsiteRequest.h>
#include <aws/s3/model/GetBucketAclRequest.h>
#include <aws/s3/model/GetObjectAclRequest.h>
#include <aws/s3/model/GetBucketPolicyRequest.h>
#include <aws/s3/model/GetBucketWebsiteRequest.h>
#include <aws/s3/model/ListObjectsRequest.h>
#include <aws/s3/model/Object.h>
#include <aws/s3/model/Owner.h>
#include <aws/s3/model/Grantee.h>
#include <aws/s3/model/Grant.h>
#include <aws/s3/model/AccessControlPolicy.h>
#include <aws/s3/model/PutBucketAclRequest.h>
#include <aws/s3/model/GetBucketAclRequest.h>
#include <aws/s3/model/PutBucketPolicyRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/s3/model/Permission.h>
#include <aws/s3/model/IndexDocument.h>
#include <aws/s3/model/ErrorDocument.h>
#include <aws/s3/model/WebsiteConfiguration.h>
#include <aws/s3/model/PutBucketWebsiteRequest.h>
#include <aws/s3/model/PutObjectAclRequest.h>
#include <aws/core/utils/UUID.h>
#include <aws/core/utils/StringUtils.h>
#include <fstream>
#include <string>



std::mutex AwsDoc::S3::helper::upload_mutex;
std::condition_variable AwsDoc::S3::helper::upload_variable;

AwsDoc::S3::helper::helper(Aws::Client::ClientConfiguration config, Aws::String  appKey, Aws::String  sercetKey) {

    this->app_key = appKey;
    this->sercet_key = sercetKey;

    //Aws::Client::ClientConfiguration config;

    //config.endpointOverride = "192.168.1.70:9001";
    //config.scheme = Aws::Http::Scheme::HTTP;
    //config.verifySSL = false;

    //if (!region.empty())
    //{
    //    config.region = region;
    //}
    this->config = config;

    this->client = new Aws::S3::S3Client(Aws::Auth::AWSCredentials(this->app_key, this->sercet_key), this->config,
        Aws::Client::AWSAuthV4Signer::PayloadSigningPolicy::Never, false);


}


//! Routine which demonstrates deleting multiple objects in an Amazon S3 bucket.
/*!
  \sa DeleteObjects()
  \param objectKeys: Vector of object keys.
  \param fromBucket: Name of a bucket with an object to delete.
  \param clientConfig: AWS client configuration.
*/

bool AwsDoc::S3::helper::DeleteObjects(const std::vector<Aws::String>& objectKeys,
    const Aws::String& fromBucket,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);
    Aws::S3::Model::DeleteObjectsRequest request;

    Aws::S3::Model::Delete deleteObject;
    for (const Aws::String& objectKey : objectKeys)
    {
        deleteObject.AddObjects(Aws::S3::Model::ObjectIdentifier().WithKey(objectKey));
    }

    request.SetDelete(deleteObject);
    request.SetBucket(fromBucket);

    Aws::S3::Model::DeleteObjectsOutcome outcome =
        client.DeleteObjects(request);

    if (!outcome.IsSuccess()) {
        auto err = outcome.GetError();
        ERRORLOG("Error deleting objects.{}: {}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error deleting objects. " <<
        //    err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Successfully deleted the objects.");
        //std::cout << "Successfully deleted the objects.";
        for (size_t i = 0; i < objectKeys.size(); ++i)
        {
            std::cout << objectKeys[i];
            if (i < objectKeys.size() - 1)
            {
                std::cout << ", ";
            }
        }

        INFOLOG(" from bucket:{}.", fromBucket);
        //std::cout << " from bucket " << fromBucket << "." << std::endl;
    }

    return outcome.IsSuccess();
}

AwsDoc::S3::helper::~helper() {
    delete this->client;
}
 //! Routine which demonstrates copying an object between two S3 buckets.
 /*!
   \sa CopyObject()
   \param objectKey Key of object in from bucket.
   \param fromBucket Name of from bucket.
   \param toBucket Name of to bucket.
   \param clientConfig Aws client configuration.
 */

bool AwsDoc::S3::helper::CopyObject(const Aws::String& objectKey, const Aws::String& fromBucket, const Aws::String& toBucket,
    const Aws::Client::ClientConfiguration& clientConfig) {

    Aws::S3::S3Client client(clientConfig);
    Aws::S3::Model::CopyObjectRequest request;

    request.WithCopySource(fromBucket + "/" + objectKey)
        .WithKey(objectKey)
        .WithBucket(toBucket);

    Aws::S3::Model::CopyObjectOutcome outcome = client.CopyObject(request);
    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: CopyObject: {}:{}", err.GetExceptionName(), err.GetMessage());
      /*  std::cerr << "Error: CopyObject: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;*/

    }
    else {
        INFOLOG("Successfully copied {} from {} to {}.", objectKey, fromBucket,toBucket);
 /*       std::cout << "Successfully copied " << objectKey << " from " << fromBucket <<
            " to " << toBucket << "." << std::endl;*/
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates creating an S3 bucket.
/*!
  \sa CreateBucket()
  \param bucketName Name of bucket to create.
  \param clientConfig Aws client configuration.
*/
bool AwsDoc::S3::helper::CreateBucket(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);
    Aws::S3::Model::CreateBucketRequest request;
    request.SetBucket(bucketName);

    //TODO(user): Change the bucket location constraint enum to your target Region.
    if (clientConfig.region != "us-east-1") {
        Aws::S3::Model::CreateBucketConfiguration createBucketConfig;
        createBucketConfig.SetLocationConstraint(
            Aws::S3::Model::BucketLocationConstraintMapper::GetBucketLocationConstraintForName(
                clientConfig.region));
        request.SetCreateBucketConfiguration(createBucketConfig);
    }

    Aws::S3::Model::CreateBucketOutcome outcome = client.CreateBucket(request);
    if (!outcome.IsSuccess()) {
        auto err = outcome.GetError();
        ERRORLOG("Error: CreateBucket: {}:{}", err.GetExceptionName(), err.GetMessage());
       /* std::cerr << "Error: CreateBucket: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;*/
    }
    else {
        INFOLOG("Created bucket {} in the specified AWS Region.", bucketName);
        //std::cout << "Created bucket " << bucketName <<
        //    " in the specified AWS Region." << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates deleting an S3 bucket.
/*!
  \sa DeleteBucket()
  \param bucketName Name of the bucket to delete.
  \param clientConfig Aws client configuration.
*/

bool AwsDoc::S3::helper::DeleteBucket(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {

    Aws::S3::S3Client client(clientConfig);

    Aws::S3::Model::DeleteBucketRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::DeleteBucketOutcome outcome =
        client.DeleteBucket(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: DeleteBucket: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: DeleteBucket: " <<
        //    err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        INFOLOG("The bucket was deleted");
        //std::cout << "The bucket was deleted" << std::endl;
    }

    return outcome.IsSuccess();
}


//! Routine which demonstrates deleting the policy in an S3 bucket.
/*!
  \sa DeleteBucketPolicy()
  \param toBucket Name of a bucket with the policy to delete.
  \param clientConfig Aws client configuration.
*/

bool AwsDoc::S3::helper::DeleteBucketPolicy(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {

    Aws::S3::S3Client client(this->client);

    Aws::S3::Model::DeleteBucketPolicyRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::DeleteBucketPolicyOutcome outcome = client.DeleteBucketPolicy(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: DeleteBucketPolicy: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: DeleteBucketPolicy: " <<
        //    err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Policy was deleted from the bucket.");
        //std::cout << "Policy was deleted from the bucket." << std::endl;
    }

    return outcome.IsSuccess();
}


//! Routine which demonstrates deleting an object in an S3 bucket.
/*!
  \sa DeleteObject()
  \param objectKey Name of an object.
  \param fromBucket Name of a bucket with an object to delete.
  \param clientConfig Aws client configuration.
*/

bool AwsDoc::S3::helper::DeleteObject(const Aws::String& objectKey,
    const Aws::String& fromBucket,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);
    Aws::S3::Model::DeleteObjectRequest request;

    request.WithKey(objectKey)
        .WithBucket(fromBucket);

    Aws::S3::Model::DeleteObjectOutcome outcome =
        client.DeleteObject(request);

    if (!outcome.IsSuccess()) {
        auto err = outcome.GetError();
        ERRORLOG("Error: DeleteObject: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: DeleteObject: " <<
        //    err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Successfully deleted the object.");
        //std::cout << "Successfully deleted the object." << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates putting an object in an S3 bucket.
/*!
  \fn PutObject()
  \param bucketName Name of the bucket.
  \param fileName Name of the file to put in the bucket.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.put_object.code]
bool AwsDoc::S3::helper::PutObject(const Aws::String& bucketName,
    const Aws::String& fileName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    //We are using the name of the file as the key for the object in the bucket.
    //However, this is just a string and can be set according to your retrieval needs.
    request.SetKey(fileName);

    std::shared_ptr<Aws::IOStream> inputData =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
            fileName.c_str(),
            std::ios_base::in | std::ios_base::binary);

    if (!*inputData) {
        ERRORLOG("Error unable to read file {}", fileName);
        //std::cerr << "Error unable to read file " << fileName << std::endl;
        return false;
    }

    request.SetBody(inputData);

    Aws::S3::Model::PutObjectOutcome outcome =
        s3_client.PutObject(request);

    if (!outcome.IsSuccess()) {
        ERRORLOG("Error: PutObject: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: PutObject: " <<
        //    outcome.GetError().GetMessage() << std::endl;
    }
    else {
        INFOLOG("Added object '{}' to bucket '{}'.", fileName, bucketName);
        //std::cout << "Added object '" << fileName << "' to bucket '"
        //    << bucketName << "'.";
    }

    return outcome.IsSuccess();
}

//! Routine which implements an async task finished callback.
/*!
  \fn PutObjectAsyncFinished()
  \param s3Client Instance of the caller's Amazon S3 client object.
  \param request Instance of the caller's put object request.
  \param outcome Instance of the caller's put object outcome.
  \param context Instance of the caller's put object call context.
*/

// snippet-start:[s3.cpp.put_object_async_finished.code]
void AwsDoc::S3::helper::PutObjectAsyncFinished(const Aws::S3::S3Client* s3Client,
    const Aws::S3::Model::PutObjectRequest& request,
    const Aws::S3::Model::PutObjectOutcome& outcome,
    const std::shared_ptr<const Aws::Client::AsyncCallerContext>& context) {
    if (outcome.IsSuccess()) {
        INFOLOG("Success: PutObjectAsyncFinished: Finished uploading '{}'.", context->GetUUID());
        //std::cout << "Success: PutObjectAsyncFinished: Finished uploading '"
        //    << context->GetUUID() << "'." << std::endl;
    }
    else {
        ERRORLOG("Error: PutObjectAsyncFinished: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: PutObjectAsyncFinished: " <<
        //    outcome.GetError().GetMessage() << std::endl;
    }

    // Unblock the thread that is waiting for this function to complete.
    AwsDoc::S3::helper::upload_variable.notify_one();
}
// snippet-end:[s3.cpp.put_object_async_finished.code]


//! Routine which demonstrates adding an object to an Amazon S3 bucket, asynchronously.
/*!
  \fn GetObjectAcl()
  \param s3Client Instance of the S3 Client.
  \param bucketName Name of the bucket.
  \param fileName Name of the file to put in the bucket.
*/

// snippet-start:[s3.cpp.put_object_async.code]
bool AwsDoc::S3::helper::PutObjectAsync(const Aws::S3::S3Client& s3Client,
    const Aws::String& bucketName,
    const Aws::String& fileName) {
    // Create and configure the asynchronous put object request.
    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    request.SetKey(fileName);

    const std::shared_ptr<Aws::IOStream> input_data =
        Aws::MakeShared<Aws::FStream>("SampleAllocationTag",
            fileName.c_str(),
            std::ios_base::in | std::ios_base::binary);

    if (!*input_data) {
        ERRORLOG("Error: unable to open file {}", fileName);
        //std::cerr << "Error: unable to open file " << fileName << std::endl;
        return false;
    }

    request.SetBody(input_data);

    // Create and configure the context for the asynchronous put object request.
    std::shared_ptr<Aws::Client::AsyncCallerContext> context =
        Aws::MakeShared<Aws::Client::AsyncCallerContext>("PutObjectAllocationTag");
    context->SetUUID(fileName);

    // Make the asynchronous put object call. Queue the request into a 
    // thread executor and call the PutObjectAsyncFinished function when the 
    // operation has finished. 
    s3Client.PutObjectAsync(request, AwsDoc::S3::helper::PutObjectAsyncFinished, context);

    return true;
}

//! Routine which demonstrates putting a string as an object in an S3 bucket.
/*!
  \fn PutObject()
  \param bucketName Name of the bucket.
  \param objectName Name for the object in the bucket.
  \param objectContent String as content for object.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.objects.put_string_into_object_bucket]
bool AwsDoc::S3::helper::PutObjectBuffer(const Aws::String& bucketName,
    const Aws::String& objectName,
    const std::string& objectContent,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::PutObjectRequest request;
    request.SetBucket(bucketName);
    request.SetKey(objectName);

    const std::shared_ptr<Aws::IOStream> inputData =
        Aws::MakeShared<Aws::StringStream>("");
    *inputData << objectContent.c_str();

    request.SetBody(inputData);

    Aws::S3::Model::PutObjectOutcome outcome = s3_client.PutObject(request);

    if (!outcome.IsSuccess()) {
        ERRORLOG("Error: PutObjectBuffer: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: PutObjectBuffer: " <<
        //    outcome.GetError().GetMessage() << std::endl;
    }
    else {
        INFOLOG("Success: Object '{}' with content '{}' uploaded to bucket '{}'.", objectName, objectContent, bucketName);
        //std::cout << "Success: Object '" << objectName << "' with content '"
        //    << objectContent << "' uploaded to bucket '" << bucketName << "'.";
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates configuring a website for an S3 bucket.
/*!
  \sa PutWebsiteConfig()
  \param bucketName Name of S3 bucket.
  \param indexPage Name of index page.
  \param errorPage Name of error page.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.put_website_config.code]
bool AwsDoc::S3::helper::PutWebsiteConfig(const Aws::String& bucketName,
    const Aws::String& indexPage, const Aws::String& errorPage,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);

    Aws::S3::Model::IndexDocument indexDocument;
    indexDocument.SetSuffix(indexPage);

    Aws::S3::Model::ErrorDocument errorDocument;
    errorDocument.SetKey(errorPage);

    Aws::S3::Model::WebsiteConfiguration websiteConfiguration;
    websiteConfiguration.SetIndexDocument(indexDocument);
    websiteConfiguration.SetErrorDocument(errorDocument);

    Aws::S3::Model::PutBucketWebsiteRequest request;
    request.SetBucket(bucketName);
    request.SetWebsiteConfiguration(websiteConfiguration);

    Aws::S3::Model::PutBucketWebsiteOutcome outcome =
        client.PutBucketWebsite(request);

    if (!outcome.IsSuccess()) {
        ERRORLOG("Error: PutBucketWebsite: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: PutBucketWebsite: "
        //    << outcome.GetError().GetMessage() << std::endl;
    }
    else {
        INFOLOG("Success: Set website configuration for bucket '{}'.", bucketName);
        //std::cout << "Success: Set website configuration for bucket '"
        //    << bucketName << "'." << std::endl;
    }

    return outcome.IsSuccess();
}


//! Routine which demonstrates deleting the website configuration for an S3 bucket.
/*!
  \sa DeleteBucketWebsite()
  \param bucketName Name of the bucket containing a website configuration.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.delete_website_config.code]
bool AwsDoc::S3::helper::DeleteBucketWebsite(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);
    Aws::S3::Model::DeleteBucketWebsiteRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::DeleteBucketWebsiteOutcome outcome =
        client.DeleteBucketWebsite(request);

    if (!outcome.IsSuccess()) {
        auto err = outcome.GetError();
        ERRORLOG("Error: DeleteBucketWebsite: {}:{}", err.GetExceptionName(), err.GetMessage());
 /*       std::cerr << "Error: DeleteBucketWebsite: " <<
            err.GetExceptionName() << ": " << err.GetMessage() << std::endl;*/
    }
    else {
        INFOLOG("Website configuration was removed.");
        //std::cout << "Website configuration was removed." << std::endl;
    }

    return outcome.IsSuccess();
}


// snippet-start:[s3.cpp.get_acl_bucket.code]
bool AwsDoc::S3::helper::GetBucketAcl(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::GetBucketAclRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::GetBucketAclOutcome outcome =
        s3_client.GetBucketAcl(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: GetBucketAcl: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: GetBucketAcl: "
        //    << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        Aws::Vector<Aws::S3::Model::Grant> grants =
            outcome.GetResult().GetGrants();

        for (auto it = grants.begin(); it != grants.end(); it++) {
            Aws::S3::Model::Grant grant = *it;
            Aws::S3::Model::Grantee grantee = grant.GetGrantee();

            INFOLOG("For bucket {}", bucketName);
  /*          std::cout << "For bucket " << bucketName << ": "
                << std::endl << std::endl;*/

            if (grantee.TypeHasBeenSet()) {
                INFOLOG("Type:{}", GetGranteeTypeString(grantee.GetType()));
                //std::cout << "Type:          "
                //    << GetGranteeTypeString(grantee.GetType()) << std::endl;
            }

            if (grantee.DisplayNameHasBeenSet()) {
                INFOLOG("Display name: {}", grantee.GetDisplayName());
                //std::cout << "Display name:  "
                //    << grantee.GetDisplayName() << std::endl;
            }

            if (grantee.EmailAddressHasBeenSet()) {
                INFOLOG("Email address: {}", grantee.GetEmailAddress());
                //std::cout << "Email address: "
                //    << grantee.GetEmailAddress() << std::endl;
            }

            if (grantee.IDHasBeenSet()) {
                INFOLOG("ID:{}", grantee.GetID());
 /*               std::cout << "ID:            "
                    << grantee.GetID() << std::endl;*/
            }

            if (grantee.URIHasBeenSet()) {
                INFOLOG("URI:{}", grantee.GetURI());
            /*    std::cout << "URI:           "
                    << grantee.GetURI() << std::endl;*/
            }

            INFOLOG(" Permission: {}", GetPermissionString(grant.GetPermission()));
            //std::cout << "Permission:    " <<
            //    GetPermissionString(grant.GetPermission()) <<
            //    std::endl << std::endl;
        }
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates setting the ACL for an S3 bucket.
/*!
  \sa GetBucketPolicy()
  \param bucketName Name of a bucket.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.get_bucket_policy.code]
bool AwsDoc::S3::helper::GetBucketPolicy(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::GetBucketPolicyRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::GetBucketPolicyOutcome outcome =
        s3_client.GetBucketPolicy(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: GetBucketPolicy: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: GetBucketPolicy: "
        //    << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        Aws::StringStream policy_stream;
        Aws::String line;

        outcome.GetResult().GetPolicy() >> line;
        policy_stream << line;

        INFOLOG("Retrieve the policy for bucket '{}':{}", bucketName, policy_stream.str());
        //std::cout << "Retrieve the policy for bucket '" << bucketName << "':\n\n" <<
        //    policy_stream.str() << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates getting an object in an S3 bucket.
/*!
  \sa GetObject()
  \param objectKey Name of an object in a bucket.
  \param toBucket: Name of a bucket.
  \param clientConfig: Aws client configuration.
*/

// snippet-start:[s3.cpp.get_object.code]
bool AwsDoc::S3::helper::GetObject(const Aws::String& objectKey,
    const Aws::String& fromBucket,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);

    Aws::S3::Model::GetObjectRequest request;
    request.SetBucket(fromBucket);
    request.SetKey(objectKey);

    Aws::S3::Model::GetObjectOutcome outcome =
        client.GetObject(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: GetObject: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: GetObject: " <<
        //    err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Successfully retrieved '{}' from '{}'.", objectKey, fromBucket);
        //std::cout << "Successfully retrieved '" << objectKey << "' from '"
        //    << fromBucket << "'." << std::endl;
    }

    return outcome.IsSuccess();
}


//! Routine which demonstrates getting the website configuration for an S3 bucket.
/*!
  \sa GetWebsiteConfig()
  \param bucketName Name of to bucket containing a website configuration.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.get_website_config.code]
bool AwsDoc::S3::helper::GetWebsiteConfig(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::GetBucketWebsiteRequest request;
    request.SetBucket(bucketName);

    Aws::S3::Model::GetBucketWebsiteOutcome outcome =
        s3_client.GetBucketWebsite(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();

        ERRORLOG("Error: GetBucketWebsite: {}", err.GetMessage());
        //std::cerr << "Error: GetBucketWebsite: "
        //    << err.GetMessage() << std::endl;
    }
    else {
        Aws::S3::Model::GetBucketWebsiteResult websiteResult = outcome.GetResult();

        INFOLOG("Success: GetBucketWebsite: For bucket '{}' Index page :{} Error page:{}", bucketName, websiteResult.GetIndexDocument().GetSuffix(), websiteResult.GetErrorDocument().GetKey());
      /*  std::cout << "Success: GetBucketWebsite: "
            << std::endl << std::endl
            << "For bucket '" << bucketName << "':"
            << std::endl
            << "Index page : "
            << websiteResult.GetIndexDocument().GetSuffix()
            << std::endl
            << "Error page: "
            << websiteResult.GetErrorDocument().GetKey()
            << std::endl;*/
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates listing the buckets in the current account.
/*!
  \fn ListBuckets()
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.list_buckets.code]
bool AwsDoc::S3::helper::ListBuckets(const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client client(clientConfig);

    auto outcome = client.ListBuckets();

    bool result = true;
    if (!outcome.IsSuccess()) {
        //ERRORLOG("Failed with error: {}", outcome.GetError());
        std::cerr << "Failed with error: " << outcome.GetError() << std::endl;
        result = false;
    }
    else {
        INFOLOG("Found {} buckets", outcome.GetResult().GetBuckets().size());
        //std::cout << "Found " << outcome.GetResult().GetBuckets().size() << " buckets\n";
        for (auto&& b : outcome.GetResult().GetBuckets()) {
            std::cout << b.GetName() << std::endl;
        }
    }

    return result;
}

//! Routine which demonstrates listing the objects in an S3 bucket.
/*!
  \fn ListObjects()
  \param bucketName Name of the S3 bucket.
  \param clientConfig Aws client configuration.
 */

 // snippet-start:[s3.cpp.list_objects.code]
bool AwsDoc::S3::helper::ListObjects(const Aws::String& bucketName,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::ListObjectsRequest request;
    request.WithBucket(bucketName);

    auto outcome = s3_client.ListObjects(request);

    if (!outcome.IsSuccess()) {
        ERRORLOG("Error: ListObjects: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: ListObjects: " <<
        //    outcome.GetError().GetMessage() << std::endl;
    }
    else {
        Aws::Vector<Aws::S3::Model::Object> objects =
            outcome.GetResult().GetContents();

        for (Aws::S3::Model::Object& object : objects) {
            std::cout << object.GetKey() << std::endl;
        }
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates setting the ACL for an S3 bucket.
/*!
  \sa PutBucketAcl()
  \param bucketName Name of from bucket.
  \param ownerID The canonical ID of the bucket owner.
   For more information see https://docs.aws.amazon.com/AmazonS3/latest/userguide/finding-canonical-user-id.html.
  \param granteePermission The access level to enable for the grantee.
  \param granteeType The type of grantee.
  \param granteeID The canonical ID of the grantee.
  \param clientConfig Aws client configuration.
  \param granteeDisplayName The display name of the grantee.
  \param granteeEmailAddress The email address associated with the grantee's AWS account.
  \param granteeURI The URI of a built-in access group.
*/

// snippet-start:[s3.cpp.put_bucket_acl.code]
bool AwsDoc::S3::helper::PutBucketAcl(const Aws::String& bucketName,
    const Aws::String& ownerID,
    const Aws::String& granteePermission,
    const Aws::String& granteeType,
    const Aws::String& granteeID,
    const Aws::Client::ClientConfiguration& clientConfig,
    const Aws::String& granteeDisplayName,
    const Aws::String& granteeEmailAddress,
    const Aws::String& granteeURI) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::Owner owner;
    owner.SetID(ownerID);

    Aws::S3::Model::Grantee grantee;
    grantee.SetType(SetGranteeType(granteeType));

    if (!granteeEmailAddress.empty()) {
        grantee.SetEmailAddress(granteeEmailAddress);
    }

    if (!granteeID.empty()) {
        grantee.SetID(granteeID);
    }

    if (!granteeDisplayName.empty()) {
        grantee.SetDisplayName(granteeDisplayName);
    }

    if (!granteeURI.empty()) {
        grantee.SetURI(granteeURI);
    }

    Aws::S3::Model::Grant grant;
    grant.SetGrantee(grantee);
    grant.SetPermission(SetGranteePermission(granteePermission));

    Aws::Vector<Aws::S3::Model::Grant> grants;
    grants.push_back(grant);

    Aws::S3::Model::AccessControlPolicy acp;
    acp.SetOwner(owner);
    acp.SetGrants(grants);

    Aws::S3::Model::PutBucketAclRequest request;
    request.SetAccessControlPolicy(acp);
    request.SetBucket(bucketName);

    Aws::S3::Model::PutBucketAclOutcome outcome =
        s3_client.PutBucketAcl(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& error = outcome.GetError();

        ERRORLOG("Error: PutBucketAcl: {} - {}", error.GetExceptionName(), error.GetMessage());
        //std::cerr << "Error: PutBucketAcl: " << error.GetExceptionName()
        //    << " - " << error.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Successfully added an ACL to the bucket '{}'.", bucketName);
        //std::cout << "Successfully added an ACL to the bucket '" << bucketName
        //    << "'." << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates setting a policy on an S3 bucket.
/*!
  \sa PutBucketPolicy()
  \param bucketName: Name of a bucket.
  \param  policyBody: The bucket policy to add.
  \param clientConfig: Aws client configuration.
*/

// snippet-start:[s3.cpp.put_bucket_policy02.code]
bool AwsDoc::S3::helper::PutBucketPolicy(const Aws::String& bucketName,
    const Aws::String& policyBody,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    std::shared_ptr<Aws::StringStream> request_body =
        Aws::MakeShared<Aws::StringStream>("");
    *request_body << policyBody;

    Aws::S3::Model::PutBucketPolicyRequest request;
    request.SetBucket(bucketName);
    request.SetBody(request_body);

    Aws::S3::Model::PutBucketPolicyOutcome outcome =
        s3_client.PutBucketPolicy(request);

    if (!outcome.IsSuccess()) {
        ERRORLOG("Error: PutBucketPolicy: {}", outcome.GetError().GetMessage());
        //std::cerr << "Error: PutBucketPolicy: "
        //    << outcome.GetError().GetMessage() << std::endl;
    }
    else {
        INFOLOG("Set the following policy body for the bucket '{}':{}", bucketName, policyBody);
        //std::cout << "Set the following policy body for the bucket '" <<
        //    bucketName << "':" << std::endl << std::endl;
        //std::cout << policyBody << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which demonstrates getting the ACL for an object in an S3 bucket.
/*!
  \fn GetObjectAcl()
  \param bucketName Name of the bucket.
  \param objectKey Name of the object in the bucket.
  \param clientConfig Aws client configuration.
*/

// snippet-start:[s3.cpp.get_object_acl.code]
bool AwsDoc::S3::helper::GetObjectAcl(const Aws::String& bucketName,
    const Aws::String& objectKey,
    const Aws::Client::ClientConfiguration& clientConfig) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::GetObjectAclRequest request;
    request.SetBucket(bucketName);
    request.SetKey(objectKey);

    Aws::S3::Model::GetObjectAclOutcome outcome =
        s3_client.GetObjectAcl(request);

    if (!outcome.IsSuccess()) {
        const Aws::S3::S3Error& err = outcome.GetError();
        ERRORLOG("Error: GetObjectAcl: {}:{}", err.GetExceptionName(), err.GetMessage());
        //std::cerr << "Error: GetObjectAcl: "
        //    << err.GetExceptionName() << ": " << err.GetMessage() << std::endl;
    }
    else {
        Aws::Vector<Aws::S3::Model::Grant> grants =
            outcome.GetResult().GetGrants();

        for (auto it = grants.begin(); it != grants.end(); it++) {
            INFOLOG("For object: {}", objectKey);
            //std::cout << "For object " << objectKey << ": "
            //    << std::endl << std::endl;

            Aws::S3::Model::Grant grant = *it;
            Aws::S3::Model::Grantee grantee = grant.GetGrantee();

            if (grantee.TypeHasBeenSet()) {
                INFOLOG("Type:{}", GetGranteeTypeString(grantee.GetType()));
                //std::cout << "Type:          "
                //    << GetGranteeTypeString(grantee.GetType()) << std::endl;
            }

            if (grantee.DisplayNameHasBeenSet()) {
                INFOLOG("Display name:{}", grantee.GetDisplayName());
                //std::cout << "Display name:  "
                //    << grantee.GetDisplayName() << std::endl;
            }

            if (grantee.EmailAddressHasBeenSet()) {
                INFOLOG("Email address: {}", grantee.GetEmailAddress());
                //std::cout << "Email address: "
                //    << grantee.GetEmailAddress() << std::endl;
            }

            if (grantee.IDHasBeenSet()) {
                INFOLOG("ID: {}", grantee.GetID());
                //std::cout << "ID:            "
                //    << grantee.GetID() << std::endl;
            }

            if (grantee.URIHasBeenSet()) {
                INFOLOG("URI:{}", grantee.GetURI());
                //std::cout << "URI:           "
                //    << grantee.GetURI() << std::endl;
            }

            INFOLOG("Permission: {}", GetPermissionString(grant.GetPermission()));
            //std::cout << "Permission:    " <<
            //    GetPermissionString(grant.GetPermission()) <<
            //    std::endl << std::endl;
        }
    }

    return outcome.IsSuccess();
}


// snippet-end:[s3.cpp.get_object_acl.code]

//! Routine which demonstrates setting the ACL for an object in an S3 bucket.
/*!
  \sa PutObjectAcl()
  \param bucketName Name of from bucket.
  \param objectKey Name of object in the bucket.
  \param ownerID The canonical ID of the bucket owner.
   For more information, see https://docs.aws.amazon.com/AmazonS3/latest/userguide/finding-canonical-user-id.html.
  \param granteePermission The access level to enable for the grantee.
  \param granteeType The type of grantee.
  \param granteeID The canonical ID of the grantee.
  \param clientConfig Aws client configuration.
  \param granteeDisplayName The display name of the grantee.
  \param granteeEmailAddress The email address associated with the grantee's AWS account.
  \param granteeURI The URI of a built-in access group.
*/

// snippet-start:[s3.cpp.put_object_acl.code]
bool AwsDoc::S3::helper::PutObjectAcl(const Aws::String& bucketName,
    const Aws::String& objectKey,
    const Aws::String& ownerID,
    const Aws::String& granteePermission,
    const Aws::String& granteeType,
    const Aws::String& granteeID,
    const Aws::Client::ClientConfiguration& clientConfig,
    const Aws::String& granteeDisplayName,
    const Aws::String& granteeEmailAddress,
    const Aws::String& granteeURI) {
    Aws::S3::S3Client s3_client(clientConfig);

    Aws::S3::Model::Owner owner;
    owner.SetID(ownerID);

    Aws::S3::Model::Grantee grantee;
    grantee.SetType(SetGranteeType(granteeType));

    if (!granteeEmailAddress.empty()) {
        grantee.SetEmailAddress(granteeEmailAddress);
    }

    if (!granteeID.empty()) {
        grantee.SetID(granteeID);
    }

    if (!granteeDisplayName.empty()) {
        grantee.SetDisplayName(granteeDisplayName);
    }

    if (!granteeURI.empty()) {
        grantee.SetURI(granteeURI);
    }

    Aws::S3::Model::Grant grant;
    grant.SetGrantee(grantee);
    grant.SetPermission(SetGranteePermission(granteePermission));

    Aws::Vector<Aws::S3::Model::Grant> grants;
    grants.push_back(grant);

    Aws::S3::Model::AccessControlPolicy acp;
    acp.SetOwner(owner);
    acp.SetGrants(grants);

    Aws::S3::Model::PutObjectAclRequest request;
    request.SetAccessControlPolicy(acp);
    request.SetBucket(bucketName);
    request.SetKey(objectKey);

    Aws::S3::Model::PutObjectAclOutcome outcome =
        s3_client.PutObjectAcl(request);

    if (!outcome.IsSuccess()) {
        auto error = outcome.GetError();
        ERRORLOG("Error: PutObjectAcl: {}-{}", error.GetExceptionName(), error.GetMessage());
        //std::cerr << "Error: PutObjectAcl: " << error.GetExceptionName()
        //    << " - " << error.GetMessage() << std::endl;
    }
    else {
        INFOLOG("Successfully added an ACL to the object '{}' in the bucket '{}'.", objectKey, bucketName);
        //std::cout << "Successfully added an ACL to the object '" << objectKey
        //    << "' in the bucket '" << bucketName << "'." << std::endl;
    }

    return outcome.IsSuccess();
}

//! Routine which converts a human-readable string to a built-in type enumeration.
/*!
 \sa SetGranteePermission()
 \param access Human readable string.
*/

Aws::S3::Model::Permission AwsDoc::S3::helper::SetGranteePermission(const Aws::String& access) {
    if (access == "FULL_CONTROL")
        return Aws::S3::Model::Permission::FULL_CONTROL;
    if (access == "WRITE")
        return Aws::S3::Model::Permission::WRITE;
    if (access == "READ")
        return Aws::S3::Model::Permission::READ;
    if (access == "WRITE_ACP")
        return Aws::S3::Model::Permission::WRITE_ACP;
    if (access == "READ_ACP")
        return Aws::S3::Model::Permission::READ_ACP;
    return Aws::S3::Model::Permission::NOT_SET;
}

//! Routine which converts a human-readable string to a built-in type enumeration.
/*!
 \sa SetGranteeType()
 \param type Human readable string.
*/

Aws::S3::Model::Type AwsDoc::S3::helper::SetGranteeType(const Aws::String& type) {
    if (type == "Amazon customer by email")
        return Aws::S3::Model::Type::AmazonCustomerByEmail;
    if (type == "Canonical user")
        return Aws::S3::Model::Type::CanonicalUser;
    if (type == "Group")
        return Aws::S3::Model::Type::Group;
    return Aws::S3::Model::Type::NOT_SET;
}

//! Routine which converts a built-in type enumeration to a human-readable string.
/*!
 \sa GetGranteeTypeString()
 \param type Type enumeration.
*/

Aws::String AwsDoc::S3::helper::GetGranteeTypeString(const Aws::S3::Model::Type& type) {
    switch (type) {
    case Aws::S3::Model::Type::AmazonCustomerByEmail:
        return "Email address of an AWS account";
    case Aws::S3::Model::Type::CanonicalUser:
        return "Canonical user ID of an AWS account";
    case Aws::S3::Model::Type::Group:
        return "Predefined Amazon S3 group";
    case Aws::S3::Model::Type::NOT_SET:
        return "Not set";
    default:
        return "Type unknown";
    }
}

//! Routine which converts a built-in type enumeration to a human-readable string.
/*!
 \sa GetPermissionString()
 \param permission Permission enumeration.
*/

Aws::String AwsDoc::S3::helper::GetPermissionString(const Aws::S3::Model::Permission& permission) {
    switch (permission) {
    case Aws::S3::Model::Permission::FULL_CONTROL:
        return "Can list objects in this bucket, create/overwrite/delete "
            "objects in this bucket, and read/write this "
            "bucket's permissions";
    case Aws::S3::Model::Permission::NOT_SET:
        return "Permission not set";
    case Aws::S3::Model::Permission::READ:
        return "Can list objects in this bucket";
    case Aws::S3::Model::Permission::READ_ACP:
        return "Can read this bucket's permissions";
    case Aws::S3::Model::Permission::WRITE:
        return "Can create, overwrite, and delete objects in this bucket";
    case Aws::S3::Model::Permission::WRITE_ACP:
        return "Can write this bucket's permissions";
    default:
        return "Permission unknown";
    }

    return "Permission unknown";
}

bool AwsDoc::S3::helper::uploadfile(std::string BucketName, std::string objectKey, std::string pathkey)
{
    try {
        Aws::S3::Model::PutObjectRequest putObjectRequest;
        putObjectRequest.WithBucket(BucketName.c_str()).WithKey(objectKey.c_str());
        auto input_data = Aws::MakeShared<Aws::FStream>("PutObjectInputStream",
            pathkey.c_str(), std::ios_base::in | std::ios_base::binary);
        putObjectRequest.SetBody(input_data);
        auto putObjectResult = client->PutObject(putObjectRequest);
        if (putObjectResult.IsSuccess())
        {
            INFOLOG("Put S3 file susscess: BucketName:{},objectKey:{}", BucketName, objectKey);
            return true;
        }
        else
        {
            ERRORLOG( "PutObject error: {} {} with  BucketName:{},objectKey:{}",putObjectResult.GetError().GetExceptionName(), putObjectResult.GetError().GetMessage(), BucketName, objectKey);
            return false;
        }
    }
    catch (...) {
        ERRORLOG( "s3 upload file failed :bucket:{} file:{} pathkey:{}",BucketName,objectKey, pathkey);
    }
    return false;
}
bool AwsDoc::S3::helper::downloadfile(std::string BucketName, std::string objectKey, std::string pathkey)
{
    Aws::S3::Model::GetObjectRequest object_request;
    try {
        object_request.WithBucket(BucketName.c_str()).WithKey(objectKey.c_str());
        auto get_object_outcome = client->GetObject(object_request);
        if (get_object_outcome.IsSuccess())
        {


            Aws::OFStream local_file;
            local_file.open(pathkey.c_str(), std::ios::out | std::ios::binary);
            local_file << get_object_outcome.GetResult().GetBody().rdbuf();
            INFOLOG("Get S3 file susscess: BucketName:{},objectKey:{}", BucketName, objectKey);
            return true;
        }
        else
        {
            INFOLOG("GetObject error: {} {}", get_object_outcome.GetError().GetExceptionName(), get_object_outcome.GetError().GetMessage());
            return false;
        }
    }
    catch (...) {
        ERRORLOG("s3 download file failed :bucket:{} file:{} pathkey:{}", BucketName, objectKey, pathkey);
    }
    return false;
}




/*
#ifndef TESTING_BUILD

int main() {
    Aws::SDKOptions options;
    Aws::InitAPI(options);

    //TODO(user): Name of object already in bucket.
    Aws::String objectKey = "test.txt";

    //TODO(user): Change from_bucket to the name of your bucket that already contains "my-file.txt".
    Aws::String fromBucket = "test1";

    //TODO(user): Change to the name of another bucket in your account.
    Aws::String toBucket = "test2";

    {
        Aws::Client::ClientConfiguration clientConfig;
        // Optional: Set to the AWS Region in which the bucket was created (overrides config file).
        // clientConfig.region = "us-east-1";

        clientConfig.endpointOverride = "192.168.1.70:9001";
        clientConfig.scheme = Aws::Http::Scheme::HTTP;
        clientConfig.verifySSL = false;

        Aws::S3::S3Client s3_client(clientConfig);

        //clientConfig.region = "us-east-1";
        // 
        //TODO(user): Change bucket_name to the name of a bucket in your account.
        const Aws::String bucket_name = "test1";
        //TODO(user): Create a file called "my-file.txt" in the local folder where your executables are built to.
        const Aws::String object_name = "test.txt";

        AwsDoc::S3* s3 = new AwsDoc::S3(clientConfig, "ssjhdjfha", "ssdfafde");
        s3->PutObject(bucket_name, object_name, clientConfig);
        s3->CopyObject(objectKey, fromBucket, toBucket, clientConfig);

        std::unique_lock<std::mutex> lock(s3->upload_mutex);

        s3->PutObjectAsync(s3_client, bucket_name, object_name);

        std::cout << "main: Waiting for file upload attempt..." <<
            std::endl << std::endl;

        // While the put object operation attempt is in progress,
        // you can perform other tasks.
        // This example simply blocks until the put object operation
        // attempt finishes.
        s3->upload_variable.wait(lock);

        std::cout << std::endl << "main: File upload attempt completed."
            << std::endl;
        
    }

    ShutdownAPI(options);
    return 0;
}

#endif // TESTING_BUILD

*/

//#endif  //USE_S3
