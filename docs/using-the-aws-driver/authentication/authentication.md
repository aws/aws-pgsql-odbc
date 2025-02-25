# Authentication Types Supported By the AWS ODBC Driver for PostgreSQL

The driver supports 5 types of authentication. 
1. [Database authentication](database_authentication.md)
1. [IAM database authentication](iam_authentication.md)
1. [Secret Manager authentication](secret_manager_authentication.md)
1. [ADFS authentication](adfs_authentication.md)
1. [OKTA authentication](okta_authentication.md)

You can choose the authentication method by specifying the `AuthType` connection option.

| Authentication                | AuthType          |
|-------------------------------|-------------------|
| Database authentication       | `database`        |
| IAM authentication            | `iam`             |
| Secret Manager authentication | `secrets-manager` |
| ADFS authentication           | `adfs`            |
| OKTA authentication           | `okta`            |
