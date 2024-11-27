# Authentication types supported by PostgreSQL ODBC driver

The driver supports 5 types of authentication. 
1. [Database authentication](./authentication/database_authentication.md)
1. [IAM database authentication](./authentication/iam_authentication.md)
1. [Secret Manager authentication](./authentication/secret_manager_authentication.md)
1. [ADFS authentication](./authentication/adfs_authentication.md)
1. [OKTA authentication](./authentication/okta_authentication.md)

You can choose the authentication method by specifying the `AuthType` connection option.

| Authentication                | AuthType      |
|-------------------------------|---------------|
| Database authentication       | `database`    |
| IAM authentication            | `iam`         |
| Secret Manager authentication | `secret`      |
| ADFS authentication           | `adfs`        |
| OKTA authentication           | `okta`        |
