# Getting Started

## Obtaining the AWS ODBC Driver for PostgreSQL

You can download the AWS ODBC Driver for PostgreSQL installers and binaries for Windows and MacOS that directly from [GitHub Releases](https://github.com/aws/aws-pgsql-odbc/releases).

## Installing the AWS ODBC Driver for PostgreSQL

### Windows

Download the `.msi` Windows installer for your system; execute the installer and follow the onscreen instructions. The default target installation location for the driver files is `C:\Program Files\AWSpsqlODBC`.
Four drivers will be installed:
- AWS ANSI ODBC Driver for PostgreSQL
- AWS ANSI ODBC Driver for PostgreSQL (x64)
- AWS Unicode ODBC Driver for PostgreSQL
- AWS Unicode ODBC Driver for PostgreSQL (x64)

To uninstall the ODBC driver, open the same installer file, select the option to uninstall the driver and follow the onscreen instructions to successfully uninstall the driver.

### MacOS
> [!WARNING]\
> This driver currently only supports MacOS with Silicon chips.

In order to use the AWS ODBC Driver for PostgreSQL, the following dependencies must be installed:
- PostgreSQL
- GFlags
- unixODBC

You can install them using `Homebrew`, e.g. `brew install postgresql gflags unixodbc`.

#### Known Limitations
- This driver currently has compatibility issues with the `iODBC Driver Manager` on MacOS. DSN need to be configured by manually modifying the `odbc.ini` and `odbcinist.ini` files.
- This driver currently does not support [Amazon Aurora Global Database](https://aws.amazon.com/rds/aurora/global-database/) and [RDS Multi-AZ Cluster Deployments](https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/multi-az-db-clusters-concepts.html).

### Configuring the Driver and DSN Entries
To configure the driver on Windows, use the `ODBC Data Source Administrator` tool to add or configure a DSN for either the `AWS ANSI ODBC Driver for PostgreSQL` or `AWS Unicode ODBC Driver for PostgreSQL`.
With this DSN you can specify the options for the desired connection. Additional configuration properties are available by clicking the `Details >>` button.

To use the driver on MacOS, you need to create two files: `odbc.ini` and `odbcinst.ini`, that will contain the configuration for the driver and the Data Source Name (DSN), see the following examples:

#### odbc.ini
```bash
[ODBC Data Sources]
awspsqlodbca = AWS Unicode ODBC Driver for PostgreSQL
awspsqlodbcw = AWS ANSI ODBC Driver for PostgreSQL

[awspsqlodbcw]
Driver                           = AWS Unicode ODBC Driver for PostgreSQL
SERVER                           = database-pg-name.cluster-XYZ.us-east-2.rds.amazonaws.com
DATABASE                         = postgres

[awspsqlodbca]
Driver                            = AWS ANSI ODBC Driver for PostgreSQL
SERVER                            = database-pg-name.cluster-XYZ.us-east-2.rds.amazonaws.com
DATABASE                          = postgres
```

#### odbcinst.ini
```bash
[ODBC Drivers]
AWS Unicode ODBC Driver for PostgreSQL = Installed
AWS ANSI ODBC Driver for PostgreSQL    = Installed

[AWS Unicode ODBC Driver for PostgreSQL]
Driver = awspsqlodbcw.so

[AWS ANSI ODBC Driver for PostgreSQL]
Driver = awspsqlodbca.so
```
