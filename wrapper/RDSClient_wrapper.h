#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// If AWS SDK dynamic link libraries are used, this macro needs to be enabled.
// It is an AWS SDK bug, see https://github.com/aws/aws-sdk-cpp/issues/2421
//#define USE_IMPORT_EXPORT

typedef void* RDSClientHandle;

RDSClientHandle CreateRDSClient();
void DestroyRDSClient(RDSClientHandle handle);
char* GenerateConnectAuthToken(RDSClientHandle handle, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName);

#ifdef __cplusplus
}
#endif
