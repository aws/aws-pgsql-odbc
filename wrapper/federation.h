/* File:			federation.h
 *
 * Description:		This file contains defines and declarations that are related to
 *					the federation authentication base class.
 *
 * Comments:		Copyright Amazon.com Inc. or its affiliates. See "readme.txt" for copyright and license information.
 *
 */

#ifndef __FEDERATION_H__
#define __FEDERATION_H__

#include <aws/core/Aws.h>
#include <aws/core/auth/AWSCredentials.h>
#include <aws/core/http/HttpClient.h>
#include <aws/sts/model/AssumeRoleWithSAMLRequest.h>
#include <aws/sts/STSClient.h>

class FederationCredentialProvider {
 public:
     FederationCredentialProvider(
      const std::string& idpArn,
      const std::string& roleArn,
      std::shared_ptr< Aws::Http::HttpClient > httpClient,
      std::shared_ptr< Aws::STS::STSClient > stsClient)
      : idpArn(idpArn), roleArn(roleArn), httpClient(httpClient), stsClient(stsClient) {
    // No-op.
  }

  bool GetAWSCredentials(Aws::Auth::AWSCredentials& credentials);

 protected:
  virtual std::string GetSAMLAssertion(std::string& errInfo) = 0;

  bool FetchCredentialsWithSAMLAssertion(
      Aws::STS::Model::AssumeRoleWithSAMLRequest& samlRequest,
      Aws::Auth::AWSCredentials& credentials);

  std::string idpArn;
  std::string roleArn;
  std::shared_ptr< Aws::STS::STSClient > stsClient;
  std::shared_ptr< Aws::Http::HttpClient > httpClient;
};

#endif  //__FEDERATION_H__
