/* File:			rsd_api.h
 *
 * Description:		This file contains APIs that are written in C++ for C code to use.
 *					The APIs are for authentication token generation and cache use.
 *
 * Comments:		Copyright Amazon.com Inc. or its affiliates. See "readme.txt" for copyright and license information.
 *
 */

#ifndef __RDS_API_H__
#define __RDS_API_H__
#include "psqlodbc.h"

#ifdef __cplusplus
extern "C" {
#endif

// If AWS SDK dynamic link libraries are used, this macro needs to be enabled.
// It is an AWS SDK bug, see https://github.com/aws/aws-sdk-cpp/issues/2421
//#define USE_IMPORT_EXPORT

char* GetCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName);
void UpdateCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, const char* token, ConnInfo* ci);

char* GenerateConnectAuthToken(ConnInfo* ci, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName);

#ifdef __cplusplus
}
#endif

#endif  //__RDS_API_H__
