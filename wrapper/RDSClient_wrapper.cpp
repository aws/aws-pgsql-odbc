#include "RDSClient_wrapper.h"
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/rds/RDSClient.h>

extern "C" {

RDSClientHandle createRDSClient() {
    Aws::Auth::DefaultAWSCredentialsProviderChain credentials_provider;
    Aws::Auth::AWSCredentials credentials = credentials_provider.GetAWSCredentials();

    Aws::RDS::RDSClientConfiguration client_config;
    //client_config.region = ds_get_utf8attr(ds->auth_region, &ds->auth_region8);
    return static_cast<RDSClientHandle>(new Aws::RDS::RDSClient(credentials, client_config));
}

const char* GenerateConnectAuthToken(RDSClientHandle handle, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName) {
    Aws::RDS::RDSClient* cppClass = static_cast<Aws::RDS::RDSClient*>(handle);
    return cppClass->GenerateConnectAuthToken(dbHostName, dbRegion, port, dbUserName).c_str();
}

void destroyRDSClient(RDSClientHandle handle) {
    delete static_cast<Aws::RDS::RDSClient*>(handle);
}

} // extern "C"
