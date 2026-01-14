# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/), and this project adheres to [Semantic Versioning](https://semver.org/#semantic-versioning-200).

## [1.0.2] - 2026-01-20
### :bug: Fixed
- Crash from double-initialization of glog ([PR #160](https://github.com/aws/aws-pgsql-odbc/pull/160)).
- Log to STDERR when logging is disabled ([PR #162](https://github.com/aws/aws-pgsql-odbc/pull/162))

### :crab: Changed
- Change default `RdsLoggingEnabled` to 0 in GUI ([PR #159](https://github.com/aws/aws-pgsql-odbc/pull/159)).

## [1.0.1] - 2025-10-17
### :bug: Fixed
- Fix and format Japanese resources ([PR #150](https://github.com/aws/aws-pgsql-odbc/pull/150)).

### :crab: Changed
- Refactor PG SQL queries to be fully qualified ([PR #153](https://github.com/aws/aws-pgsql-odbc/pull/153)).
- Update documentation specifying authentication modes supported by different AWS database engines ([PR #151](https://github.com/aws/aws-pgsql-odbc/pull/151)).

## [1.0.0] - 2025-05-02
The Amazon Web Services (AWS) ODBC Driver for PostgreSQL allows an application to take advantage of AWS authentication, failover feature of the Aurora databases, and provides support for Aurora Limitless databases.


### :magic_wand: Added
- [AWS IAM Authentication Support](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/authentication/iam_authentication.md)
- [AWS Secrets Manager Authentication Support](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/authentication/secrets_manager_authentication.md)
- AWS Federated Authentication with [OKTA](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/authentication/okta_authentication.md) and [ADFS](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/authentication/adfs_authentication.md)
- [Limitless Support](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/limitless/limitless.md)
- [Failover Support](https://github.com/aws/aws-pgsql-odbc/blob/main/docs/using-the-aws-driver/failover/failover.md)

[1.0.2]: https://github.com/aws/aws-pgsql-odbc/compare/1.0.1...1.0.2
[1.0.1]: https://github.com/aws/aws-pgsql-odbc/compare/1.0.0...1.0.1
[1.0.0]: https://github.com/aws/aws-pgsql-odbc/releases/tag/1.0.0
