#include "RDSClient_wrapper.h"
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/rds/RDSClient.h>
#include <aws/core/Aws.h>
#include <atomic>
#include <mutex>

extern "C" {

Aws::SDKOptions sdk_options;
std::atomic<int> sdk_reference_count{ 0 };
std::mutex sdk_mutex;

RDSClientHandle CreateRDSClient() {
    // Init AWS API
    if (1 == ++sdk_reference_count) {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        Aws::InitAPI(sdk_options);
    }

    Aws::Auth::DefaultAWSCredentialsProviderChain credentials_provider;
    Aws::Auth::AWSCredentials credentials = credentials_provider.GetAWSCredentials();

    Aws::RDS::RDSClientConfiguration client_config;
    return static_cast<RDSClientHandle>(new Aws::RDS::RDSClient(credentials, client_config));
}

char* GenerateConnectAuthToken(RDSClientHandle handle, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName) {
    Aws::RDS::RDSClient* cppClass = static_cast<Aws::RDS::RDSClient*>(handle);
    Aws::String token = cppClass->GenerateConnectAuthToken(dbHostName, dbRegion, port, dbUserName);
    size_t tokenSize = token.size();

    char* retval = static_cast<char*>(malloc(tokenSize+1)); 
    memcpy(retval, token.c_str(), tokenSize);
    retval[tokenSize] = 0;
    
    return retval;
}

void DestroyRDSClient(RDSClientHandle handle) {
    delete static_cast<Aws::RDS::RDSClient*>(handle);

    // Shut down AWS API
    if (0 == --sdk_reference_count) {
        std::lock_guard<std::mutex> lock(sdk_mutex);
        Aws::ShutdownAPI(sdk_options);
    }
}

} // extern "C"
