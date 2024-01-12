/*------
 * Module:			rds_api.cpp
 *
 * Description:		This module contains implementation of token generation and cache 
 *                  APIs for C++ functions that are compiled for calling from C code. 
 *
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *-------
 */

#include "rds_api.h"
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/rds/RDSClient.h>
#include <aws/core/Aws.h>
#include <atomic>
#include <mutex>
#include "adfs.h"
#include "dlg_specific.h"
#include "mylog.h"
#include <chrono>
#include <unordered_map>

Aws::SDKOptions sdkOptions;
std::atomic<int> sdkRefCount{ 0 };
std::mutex sdkMutex;

typedef struct tokenInfo
{
	std::string token;
	long        expiration;  // the expiration time in second
} TokenInfo;

// cached TokenInfo map
std::unordered_map<std::string, TokenInfo> cachedTokens;

// Create an AWS RDSClient object based on ConnInfo
Aws::RDS::RDSClient* CreateRDSClient(ConnInfo* ci) {
    if (stricmp(ci->authtype, ADFS_MODE) != 0 && stricmp(ci->authtype, IAM_MODE) != 0) {
        MYLOG(0, "Unsupported authtype %s for RDS\n", ci->authtype);
        return nullptr;
    }

    Aws::Auth::AWSCredentials credentials;
    if (stricmp(ci->authtype, ADFS_MODE) == 0) {
        // ADFS mode
        Aws::Client::ClientConfiguration clientCfg;
        clientCfg.requestTimeoutMs = atol(ci->federation_cfg.http_client_socket_timeout);
        clientCfg.connectTimeoutMs = atol(ci->federation_cfg.http_client_connect_timeout);
        clientCfg.verifySSL = true;
        std::shared_ptr< Aws::Http::HttpClient > httpClient = Aws::Http::CreateHttpClient(clientCfg);
        std::shared_ptr< Aws::STS::STSClient > stsClient = std::make_shared< Aws::STS::STSClient >();
        AdfsCredentialsProvider adfs(ci->federation_cfg, httpClient, stsClient);

        bool ret = adfs.GetAWSCredentials(credentials);
        if (!ret) {
            MYLOG(0, "Failed to get credentials from Adfs credentials provider\n");

            return nullptr;
        }
    } else {
        // IAM mode
        Aws::Auth::DefaultAWSCredentialsProviderChain credProvider;
        credentials = credProvider.GetAWSCredentials();
    }

    Aws::RDS::RDSClientConfiguration clientCfg;
    return new Aws::RDS::RDSClient(credentials, clientCfg);
}

// Free allocated AWS resource
void FreeAwsResource(Aws::RDS::RDSClient* client) {
    if (client) {
        delete static_cast<Aws::RDS::RDSClient*>(client);
    }

    // Shut down AWS API
    if (0 == --sdkRefCount) {
        std::lock_guard<std::mutex> lock(sdkMutex);
        Aws::ShutdownAPI(sdkOptions);
    }
}

// Generate a key for cached TokenInfo and current time in seconds
void GenKeyAndTime(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, std::string& key, long& currentTimeInSeconds) {
    key = std::string(dbHostName);
    key += "-";
    key += std::string(dbRegion);
    key += "-";
    key += std::string(port);
    key += "-";
    key += std::string(dbUserName);
    MYLOG(0, "key is %s\n", key.c_str());

    // Get the current time point
    auto currentTimePoint = std::chrono::system_clock::now();

    // Convert the time point to seconds
    currentTimeInSeconds = std::chrono::duration_cast<std::chrono::seconds>(currentTimePoint.time_since_epoch()).count();
    MYLOG(0, "currentTimeInSeconds is %ld\n", currentTimeInSeconds);
}

extern "C" {

// Get the cached token from the map cachedTokens
char* GetCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName) {
    std::string key("");
    long currentTimeInSeconds = 0;
    GenKeyAndTime(dbHostName, dbRegion, port, dbUserName, key, currentTimeInSeconds);

    char* retval = nullptr;
    auto itr = cachedTokens.find(key);
    if (itr == cachedTokens.end() || currentTimeInSeconds > itr->second.expiration) {
        MYLOG(0, "no cached token\n");
    } else {
        size_t tokenSize = itr->second.token.size();
        MYLOG(0, "Token size is %d\n", tokenSize);

        retval = static_cast<char*>(malloc(tokenSize + 1));
        memcpy(retval, itr->second.token.c_str(), tokenSize);
        retval[tokenSize] = 0;
    }
    return retval;
}

// Create or update a cached token
void UpdateCachedToken(const char* dbHostName, const char* dbRegion, const char* port, const char* dbUserName, const char* token, ConnInfo* ci) {
    std::string key("");
    long currentTimeInSeconds = 0;
    GenKeyAndTime(dbHostName, dbRegion, port, dbUserName, key, currentTimeInSeconds);

    TokenInfo ti;
    ti.token = std::string(token);
    ti.expiration = currentTimeInSeconds + atol(ci->token_expiration);
    cachedTokens[key] = ti;
}

// Generate a RDS token for authentication
char* GenerateConnectAuthToken(ConnInfo* ci, const char* dbHostName, const char* dbRegion, unsigned port, const char* dbUserName) {
    // Init AWS API
    if (1 == ++sdkRefCount) {
        std::lock_guard<std::mutex> lock(sdkMutex);
        Aws::InitAPI(sdkOptions);
    }

    Aws::RDS::RDSClient* client = CreateRDSClient(ci);
    if (!client) {
        FreeAwsResource(nullptr);
        return nullptr;
    }

    Aws::String token = client->GenerateConnectAuthToken(dbHostName, dbRegion, port, dbUserName);
    size_t tokenSize = token.size();
    MYLOG(0, "RDS Client generated token length is %d\n", tokenSize);

    // copy the token to allocated memory on heap to avoid return value loss
    char* retval = static_cast<char*>(malloc(tokenSize+1)); 
    memcpy(retval, token.c_str(), tokenSize);
    retval[tokenSize] = 0;
    
    FreeAwsResource(client);

    return retval;
}

} // extern "C"
