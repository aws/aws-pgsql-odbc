/*------
 * Module:			adfs.cpp
 *
 * Description:		This module contains methods of class AdfsCredentialsProvider for Adfs authentication.
 *
 *
 * Comments:		Copyright Amazon.com Inc. or its affiliates. See "readme.txt" for copyright and license information.
 *-------
 */

#include "adfs.h"
#include <regex>
#include <unordered_set>
#include "mylog.h"

const std::string AdfsCredentialsProvider::FORM_ACTION_PATTERN = "<form.*?action=\"([^\"]+)\"";
const std::string AdfsCredentialsProvider::SAML_RESPONSE_PATTERN = "SAMLResponse\\W+value=\"(.*)\"( />)";
const std::string AdfsCredentialsProvider::URL_PATTERN = "^(https)://[-a-zA-Z0-9+&@#/%?=~_!:,.']*[-a-zA-Z0-9+&@#/%=~_']";
const std::string AdfsCredentialsProvider::INPUT_TAG_PATTERN = "<input id=(.*)";
const std::string AdfsCredentialsProvider::NAME_KEY = "name";

std::string AdfsCredentialsProvider::GetSAMLAssertion(std::string& errInfo) {
	MYLOG(0, "Enter AdfsCredentialsProvider::GetSAMLAssertion\n");
	std::string url = getSignInPageUrl();
	MYLOG(0, "url is %s\n", url.c_str());

	std::shared_ptr< Aws::Http::HttpRequest > req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_GET, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);
	std::shared_ptr< Aws::Http::HttpResponse > response = httpClient->MakeRequest(req);

	std::string retval("");
	// check response code
	if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
		errInfo = "Adfs signOnPageRequest failed.";
		if (response->HasClientError()) {
			errInfo += "Client error: '" + response->GetClientErrorMessage() + "'.";
		}
		return retval;
	}

	std::istreambuf_iterator< char > eos;
	std::string body(std::istreambuf_iterator< char >(response->GetResponseBody().rdbuf()), eos);
	MYLOG(0, "body is %s\n", body.c_str());

	// retrieve SAMLResponse value
	std::smatch matches;
	std::string action("");
	if (std::regex_search(body, matches, std::regex(FORM_ACTION_PATTERN))) {
		std::string unescapedAction = matches.str(1);
		action = escapeHtmlEntity(unescapedAction);
	} else {
		errInfo = "Could not extract action from the response body";
		return retval;
	}

	MYLOG(0, "action is %s\n", action.c_str());
	if (!action.empty() && action[0]=='/') {
		url = "https://";
		url += std::string(cfg.idp_endpoint);
		url += ":";
		url += std::string(cfg.idp_port);
		url += action;
	}
	MYLOG(0, "new url is %s\n", url.c_str());

	std::map<std::string, std::string> params = getParametersFromHtmlBody(body);
	std::string content = getFormActionBody(url, params);
	MYLOG(0, "content is %s\n", content.c_str());

	if (std::regex_search(content, matches, std::regex(SAML_RESPONSE_PATTERN))) {
		MYLOG(0, "SAMLResponse value is %s\n", matches.str(1).c_str());
		return matches.str(1);
	} else {
		MYLOG(0, "fail to get SAML response\n");
		return retval;
	}
}

std::string AdfsCredentialsProvider::getSignInPageUrl() {
	std::string retval("https://");
	retval += std::string(cfg.idp_endpoint);
	retval += ":";
	retval += std::string(cfg.idp_port);
	retval += "/adfs/ls/IdpInitiatedSignOn.aspx?loginToRp=";
	retval += std::string(cfg.relaying_party_id);
	return retval;
}

bool AdfsCredentialsProvider::validateUrl(std::string& url) {
	MYLOG(0, "Enter AdfsCredentialsProvider::validateUrl\n");
	std::regex pattern(URL_PATTERN);
	
	if (!regex_match(url, pattern)) {
		MYLOG(0, "invalid url %s\n", url.c_str());
		return false; 
	}
	return true;
}

std::string AdfsCredentialsProvider::escapeHtmlEntity(std::string& html) {
	MYLOG(0, "Enter AdfsCredentialsProvider::escapeHtmlEntity\n");
	MYLOG(0, "html %s\n", html.c_str());
	std::string retval("");
	int i = 0;
	int length = html.length();
	while (i < length) {
		char c = html[i];
		if (c != '&') {
			retval.append(1, c);
			i++;
			continue;
		}

		if (html.substr(i, 4) == "&lt;") {
			retval.append(1,'<');
			i += 4;
		} else if (html.substr(i, 4) == "&gt;") {
			retval.append(1, '>');
			i += 4;
		} else if (html.substr(i, 5) == "&amp;") {
			retval.append(1, '&');
			i += 5;
		} else if (html.substr(i, 6) == "&apos;") {
			retval.append(1, '\'');
			i += 6;
		} else if (html.substr(i, 6) == "&quot;") {
			retval.append(1, '"');
			i += 6;
		} else {
			retval.append(1, c);
			++i;
		}
	}
	MYLOG(0, "After escape html is %s\n", retval.c_str());
	return retval;
}

std::vector<std::string> AdfsCredentialsProvider::getInputTagsFromHTML(std::string& body) {
	MYLOG(0, "Enter AdfsCredentialsProvider::getInputTagsFromHTML\n");
	std::unordered_set<std::string> hashSet;
	std::vector<std::string> retval;

	std::smatch matches;
	std::regex pattern(INPUT_TAG_PATTERN);
	std::string source = body;
	while (std::regex_search(source,matches,pattern)) {
		std::string tag = matches.str(0);
		MYLOG(0, "tag is %s\n", tag.c_str());
		std::string tagName = getValueByKey(tag, NAME_KEY);
		MYLOG(0, "tagName is %s\n", tagName.c_str());
		std::transform(tagName.begin(), tagName.end(), tagName.begin(), [](unsigned char c) {
			return std::tolower(c);
		});
		if (!tagName.empty() && hashSet.find(tagName) == hashSet.end()) {
			hashSet.insert(tagName);
 			retval.push_back(tag);
			MYLOG(0, "Saved inputTag is %s\n", tag.c_str());
		}

		source = matches.suffix().str();
	}

	MYLOG(0, "Input tags vector size is %zu\n", retval.size());
	return retval;
}

std::string AdfsCredentialsProvider::getValueByKey(std::string& input, const std::string& key) {
	MYLOG(0, "Enter AdfsCredentialsProvider::getValueByKey\n");
    std::string pattern("(");
    pattern += key;
    pattern += ")\\s*=\\s*\"(.*?)\"";

	std::smatch matches;
    if (std::regex_search(input, matches, std::regex(pattern))) {
		std::string unescapedMatch = matches.str(2);
		return escapeHtmlEntity(unescapedMatch);
    } else {
		return "";
    }
}

std::map<std::string, std::string> AdfsCredentialsProvider::getParametersFromHtmlBody(std::string& body) {
	MYLOG(0, "Enter AdfsCredentialsProvider::getParametersFromHtmlBody\n");
	std::map<std::string, std::string> parameters;
	for (auto& inputTag : getInputTagsFromHTML(body)) {
		MYLOG(0, "inputTag is %s\n", inputTag.c_str());
		std::string name = getValueByKey(inputTag, NAME_KEY);
		std::string value = getValueByKey(inputTag, std::string("value"));
		MYLOG(0, "name is %s, value is %s\n", name.c_str(), value.c_str());
		std::string nameLower = name;
		std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), [](unsigned char c) {
			return std::tolower(c);
		});

		if (nameLower.find("username") != std::string::npos) {
			parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_username)));
		} else if (nameLower.find("authmethod") != std::string::npos) {
			if (!value.empty()) {
				parameters.insert(std::pair<std::string, std::string>(name, value));
			}
		} else if (nameLower.find("password") != std::string::npos) {
			parameters.insert(std::pair<std::string, std::string>(name, std::string(cfg.idp_password.name)));
		} else if (!name.empty()) {
			parameters.insert(std::pair<std::string, std::string>(name, value));
		}
	}

	MYLOG(0, "parameters size is %d\n", (int)parameters.size());
	for (auto& itr : parameters) {
		MYLOG(0, "parameter key is %s, value size is %zu\n", itr.first.c_str(), itr.second.size());
	}
	return parameters;
}

std::string AdfsCredentialsProvider::getFormActionBody(std::string& url, std::map<std::string, std::string>& params) {
	MYLOG(0, "Enter AdfsCredentialsProvider::getFormActionBody\n");
	if (!validateUrl(url)) {
		return "";
	}

	std::shared_ptr< Aws::Http::HttpRequest > req = Aws::Http::CreateHttpRequest(url, Aws::Http::HttpMethod::HTTP_POST, Aws::Utils::Stream::DefaultResponseStreamFactoryMethod);

	// set content
	std::string body("");
	for (auto& itr : params) {
		body += itr.first + "=" + itr.second;
		body += "&";
	}
	body.pop_back();

#ifdef	FORCE_PASSWORD_DISPLAY
	MYLOG(0, "body in getFormActionBody is %s\n", body.c_str());
#else
	char* hide_str = hide_password(body.c_str(), '&');
	MYLOG(0, "body in getFormActionBody is %s\n", hide_str);
	if (hide_str)
		free(hide_str);
#endif /* FORCE_PASSWORD_DISPLAY */
 	std::shared_ptr< Aws::StringStream > ss = std::make_shared< Aws::StringStream >();
	*ss << body;

	req->AddContentBody(ss);
	req->SetContentLength(std::to_string(body.size()));

	// check response code
	std::shared_ptr< Aws::Http::HttpResponse > response = httpClient->MakeRequest(req);
	if (response->GetResponseCode() != Aws::Http::HttpResponseCode::OK) {
		MYLOG(0, "Response code is %d\n", static_cast<int>(response->GetResponseCode()));
		if (response->HasClientError()) {
			MYLOG(0, "Error info is %s\n", response->GetClientErrorMessage().c_str());
		}
		return "";
	}

	std::istreambuf_iterator< char > eos;
	std::string rspBody(std::istreambuf_iterator< char >(response->GetResponseBody().rdbuf()), eos);
	return rspBody;
}
