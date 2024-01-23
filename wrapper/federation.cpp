/*------
 * Module:			federation.cpp
 *
 * Description:		This module contains methods of base class for federation authentication.
 *
 *
 * Comments:		Copyright Amazon.com Inc. or its affiliates. See "readme.txt" for copyright and license information.
 *-------
 */

#include "federation.h"
#include "mylog.h"

bool FederationCredentialProvider::FetchCredentialsWithSAMLAssertion(
    Aws::STS::Model::AssumeRoleWithSAMLRequest& samlRequest,
    Aws::Auth::AWSCredentials& awsCredentials) {
  MYLOG(0, "Enter FederationCredentialProvider::FetchCredentialsWithSAMLAssertion\n");
  Aws::STS::Model::AssumeRoleWithSAMLOutcome outcome =
      stsClient->AssumeRoleWithSAML(samlRequest);

  bool retval = false;
  if (outcome.IsSuccess()) {
    const Aws::STS::Model::Credentials& credentials =
        outcome.GetResult().GetCredentials();

    MYLOG(0, "Access key is %s, secret key length is %zu\n", 
        credentials.GetAccessKeyId().c_str(), credentials.GetSecretAccessKey().size());

    awsCredentials.SetAWSAccessKeyId(credentials.GetAccessKeyId());
    awsCredentials.SetAWSSecretKey(credentials.GetSecretAccessKey());
    awsCredentials.SetSessionToken(credentials.GetSessionToken());

    retval = true;
  } else {
    auto error = outcome.GetError();
    std::string errInfo = "Failed to fetch credentials, ERROR: " + error.GetExceptionName()
              + ": " + error.GetMessage();
    MYLOG(0, "errInfo in FetchCredentialsWithSAMLAssertion is %s\n", errInfo.c_str());
  }

  return retval;
}

bool FederationCredentialProvider::GetAWSCredentials(Aws::Auth::AWSCredentials& credentials) {
  MYLOG(0, "Enter FederationCredentialProvider::GetAWSCredentials\n", "");
  std::string errInfo;
  std::string samlAsseration = GetSAMLAssertion(errInfo);

  bool retval = false;
  if (samlAsseration.empty()) {
    MYLOG(0, "errInfo in GetAWSCredentials is %s\n", errInfo.c_str());
  } else {
    MYLOG(0, "samlAsseration is %s\n", samlAsseration.c_str());

    Aws::STS::Model::AssumeRoleWithSAMLRequest samlRequest;
    samlRequest.WithRoleArn(roleArn)
        .WithSAMLAssertion(samlAsseration)
        .WithPrincipalArn(idpArn);

    retval =
        FetchCredentialsWithSAMLAssertion(samlRequest, credentials);
  }

  return retval;
}
