# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/#semantic-versioning-200).

## [1.0.0] - 2025-03-03
The Amazon Web Services (AWS) ODBC Driver for PostgreSQL allows an application to take advantage of AWS authentication and failover feature of the Aurora databases, and provide support for Aurora Limitless databases.

### :magic_wand: Added
- The [AWS IAM Authentication method](docs/using-the-aws-driver/authentication/authentication.md#iam-authentication)
- [AWS Secrets Manager authentication support](docs/using-the-aws-driver/authentication/authentication.md#secret-manager-authentication)
- AWS Federated authentication method with [OKTA](docs/using-the-aws-driver/authentication/authentication.md#okta-authentication) and [ADFS](docs/using-the-aws-driver/authentication/authentication.md#adfs-authentication)
- [Limitless Support](docs/using-the-aws-driver/limitless/limitless.md)
- [Failover Support](docs/using-the-aws-driver/failover/failover.md)
