# Amazon Web Services (AWS) ODBC Driver for PostgreSQL

[![License](https://img.shields.io/badge/license-GPLv2-blue)](LICENSE)

**The Amazon Web Services (AWS) ODBC Driver for PostgreSQL** allows an application to take advantage of the features of clustered PostgreSQL databases. It is based on the [PostgreSQL ODBC driver](https://odbc.postgresql.org/).
This driver is compatible with the following platforms:
- Windows
- MacOS (Silicon)

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

The official open-source [PostgreSQL ODBC driver](https://git.postgresql.org/gitweb/?p=postgresql.git;a=tree;h=refs/heads/master;hb=refs/heads/master) do not natively support IAM database authentication or federated authentication to RDS PostgreSQL or Aurora PostgreSQL databases. The AWS ODBC Driver for PostgreSQL implements the AWS authentication logic and integrates the AWS C++ SDK, allowing client applications to take advantage of the extra AWS security features.

## Getting Started

For more information on how to start using the AWS ODBC Driver for PostgreSQL, please visit the [Getting Started page](./docs/getting_started.md).

## Build and Test the Driver

Please refer to the AWS Driver's [documentation](./docs/documentation.md) for details on how to use, build, and test the AWS ODBC Driver for PostgreSQL.

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
