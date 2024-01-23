/* File:			adfs.h
 *
 * Description:		This file contains defines and declarations that are related to
 *					the Adfs authentication.
 *
 * Comments:		Copyright Amazon.com Inc. or its affiliates. See "readme.txt" for copyright and license information.
 *
 */

#ifndef __ADFS_H__
#define __ADFS_H__

#include "federation.h"
#include "psqlodbc.h"
#include <map>
#include <string>

class AdfsCredentialsProvider : public FederationCredentialProvider {
 public:
  AdfsCredentialsProvider(
      const FederationConfig& config,
      std::shared_ptr< Aws::Http::HttpClient > httpClient,
      std::shared_ptr< Aws::STS::STSClient > stsClient)
      : cfg(config), FederationCredentialProvider(config.iam_idp_arn, config.iam_role_arn, httpClient, stsClient) {
  }

  // constant pattern strings 
  static const std::string FORM_ACTION_PATTERN;
  static const std::string SAML_RESPONSE_PATTERN;
  static const std::string URL_PATTERN;
  static const std::string INPUT_TAG_PATTERN;

 protected:
  virtual std::string GetSAMLAssertion(std::string& errInfo);

 private:
  std::string getSignInPageUrl();
  bool validateUrl(std::string& url);
  std::string escapeHtmlEntity(std::string& html);
  std::vector<std::string> getInputTagsFromHTML(std::string& body);
  std::string getValueByKey(std::string& input, std::string& key);
  std::map<std::string, std::string> getParametersFromHtmlBody(std::string& body);
  std::string getFormActionBody(std::string& url, std::map<std::string, std::string>& params);

  FederationConfig cfg;
};

#endif  //__ADFS_H__
