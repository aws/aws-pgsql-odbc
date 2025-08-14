# Authentication Types Supported By the AWS ODBC Driver for PostgreSQL

The driver supports 5 types of authentication.
1. [Database authentication](database_authentication.md)
1. [IAM database authentication](iam_authentication.md)
1. [Secrets Manager authentication](secrets_manager_authentication.md)
1. [ADFS authentication](adfs_authentication.md)
1. [OKTA authentication](okta_authentication.md)

You can choose the authentication method by specifying the `AuthType` connection option.

| Authentication                    | AuthType          |
|-----------------------------------|-------------------|
| Database authentication           | `database`        |
| IAM authentication                | `iam`             |
| Secrets Manager authentication    | `secrets-manager` |
| ADFS authentication               | `adfs`            |
| OKTA authentication               | `okta`            |

### Authentication Support by AWS Database Engines

|                                           | Database  | IAM   | Secrets Manager   | ADFS  | OKTA  |
|-------------------------------------------|:---------:|:-----:|:-----------------:|:-----:|:-----:|
| Aurora PostgreSQL                         | ✅        | ✅   | ✅                |✅    | ✅   |
| Aurora PostgreSQL Global Database         | ✅        | ✅   | ✅                |✅    | ✅   |
| PostgreSQL Single Availability Zone (AZ)  | ✅        | ✅   | ✅                |✅    | ✅   |
| PostgreSQL Multi Availability Zone (AZ)   | ✅        | ✅   | ✅                |✅    | ✅   |
| PostgreSQL Three Availability Zone (AZ)   | ✅        | ✅   | ✅                |✅    | ✅   |

For **IAM**, **ADFS**, and **Okta**, ensure that `region` is set to the **Database Instance's** region.

An additional note for **Aurora PostgreSQL Global Database**, if using the global endpoint to connect, the region may change when server-sided failover occurs. Please ensure region is updated to the correct database instance region when re-establishing connections.

For **Secrets Manager**, ensure that `region` is set to the **Secret's** region
