# Amazon Web Services (AWS) ODBC Driver for PostgreSQL

[![License](https://img.shields.io/badge/license-LGPLv2-blue)](LICENSE)

**The Amazon Web Services (AWS) ODBC Driver for PostgreSQL** allows an application to take advantage of the features of clustered PostgreSQL databases. It is based on the [PostgreSQL ODBC driver](https://odbc.postgresql.org/).
This driver is compatible with the following platforms:
- Windows
- MacOS (Silicon)

## End of Support
Support for this project will end on May 31st 2026 as per our [maintenance policy](./Maintenance.md). We recommend migrating to the new [AWS Advanced ODBC Wrapper](https://github.com/aws/aws-advanced-odbc-wrapper) for additional features and support.

## Table of Contents

- [Amazon Web Services (AWS) ODBC Driver for PostgreSQL](#amazon-web-services-aws-odbc-driver-for-postgresql)
  - [Table of Contents](#table-of-contents)
  - [About the Driver](#about-the-driver)
    - [Benefits of the AWS ODBC Driver for PostgreSQL](#benefits-of-the-aws-odbc-driver-for-postgresql)
  - [Getting Started](#getting-started)
  - [Using the Driver](#build-and-test-the-driver)
  - [Documentation](#documentation)
  - [Getting Help and Opening Issues](#getting-help-and-opening-issues)
  - [License](#license)

## About the Driver

### Benefits of the AWS ODBC Driver for PostgreSQL

The official open-source [PostgreSQL ODBC driver](https://odbc.postgresql.org/) does not natively support RDS PostgreSQL or Aurora PostgreSQL specific features, such as the various [AWS authentication methods](./docs/using-the-aws-driver/authentication/authentication.md), [failover](./docs/using-the-aws-driver/failover/failover.md) or [Aurora Limitless Databases](./docs/using-the-aws-driver/limitless/limitless.md).
The AWS ODBC Driver for PostgreSQL allows client applications to take advantage of the extra RDS/Aurora database features by implementing support for:
1. [IAM database authentication](./docs/using-the-aws-driver/authentication/iam_authentication.md)
1. [Secrets Manager authentication](./docs/using-the-aws-driver/authentication/secrets_manager_authentication.md)
1. [ADFS authentication](./docs/using-the-aws-driver/authentication/adfs_authentication.md)
1. [Okta authentication](./docs/using-the-aws-driver/authentication/okta_authentication.md)
1. [Failover](./docs/using-the-aws-driver/failover/failover.md)
1. [Limitless](./docs/using-the-aws-driver/limitless/limitless.md)

## Getting Started

For more information on how to start using the AWS ODBC Driver for PostgreSQL, please visit the [Getting Started page](./docs/getting_started.md).

## Build and Test the Driver

Please refer to the AWS Driver's [documentation](./docs/readme.md) for details on how to use, build, and test the AWS ODBC Driver for PostgreSQL.

## Documentation

Technical documentation regarding the functionality of the AWS ODBC Driver for PostgreSQL will be maintained in this GitHub repository. For additional documentation, [please refer to the documentation for the open-source PostgreSQL ODBC driver that the AWS ODBC Driver for PostgreSQL is based on](https://odbc.postgresql.org/).

## Getting Help and Opening Issues

If you encounter an issue or bug with the AWS ODBC Driver for PostgreSQL, we would like to hear about it. Please search the [existing issues](https://github.com/aws/aws-pgsql-odbc/issues) and see if others are also experiencing the same problem before opening a new issue. When opening a new issue, please make sure to provide:

- the version of the AWS ODBC Driver for PostgreSQL
- the OS platform and version
- the PostgreSQL database version you are running

Please include a reproduction case for the issue when appropriate. Also please [include driver logs](./docs/using-the-aws-driver/using_the_aws_driver.md#logging) if possible, as they help us diagnose problems quicker.

The GitHub issues are intended for bug reports and feature requests. Keeping the list of open issues lean will help us respond in a timely manner.

## License

This software is released under version 2 of the GNU Lesser General Public License (LGPLv2).
