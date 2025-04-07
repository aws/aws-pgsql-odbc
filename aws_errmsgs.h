/* File:			aws_errmsgs.h
 *
 * Description:		See "connection.c"
 *
 * Comments:		See "readme.txt" for copyright and license information.
 *
 */

#ifndef __AWS_ERRMSGS_H__
#define __AWS_ERRMSGS_H__

/* AWSpsqlODBC-specific error messages */
#define ERRMSG_SECRETS_NOT_RETRIEVED			"Unable to retrieve credentials from Secrets Manager."
#define ERRMSG_IAM_AUTH_FAILED				"Unable to authenticate using RDS DB IAM."
#define ERRMSG_LIMITLESS_COULD_NOT_ALLOCATE		"There was a memory allocation error while attempting to use Limitless feature."
#define ERRMSG_LIMITLESS_CONNECTION_NOT_ESTABLISHED	"Connection was not established while attempting to use Limitless feature."
#define ERRMSG_LIMITLESS_NOT_LIMITLESS_CLUSTER		"Attempted to use Limitless feature on a non-Limitless cluster. Connection not established. Please disable the Limitless feature or connect to a Limitless-Supported Cluster."

#endif
