// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef __AWS_ERRMSGS_H__
#define __AWS_ERRMSGS_H__

/* AWSpsqlODBC-specific error messages */
#define ERRMSG_SECRETS_NOT_RETRIEVED			"Unable to retrieve credentials from Secrets Manager."
#define ERRMSG_IAM_AUTH_FAILED				"Unable to authenticate using RDS DB IAM."
#define ERRMSG_LIMITLESS_COULD_NOT_ALLOCATE		"There was a memory allocation error while attempting to use Limitless feature."
#define ERRMSG_LIMITLESS_CONNECTION_NOT_ESTABLISHED	"Connection was not established while attempting to use Limitless feature."
#define ERRMSG_LIMITLESS_NOT_LIMITLESS_CLUSTER		"Attempted to use Limitless feature on a non-Limitless cluster. Connection not established. Please disable the Limitless feature or connect to a Limitless-Supported Cluster."

#endif
