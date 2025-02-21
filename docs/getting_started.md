# Getting Started

## Obtaining the AWS ODBC Driver for PostgreSQL

You will find installers for Windows, MacOS and Linux that can be downloaded directly from [GitHub Releases](https://github.com/aws/aws-pgsql-odbc/releases) to install the AWS ODBC Driver for PostgreSQL.

## Installing the AWS ODBC Driver for PostgreSQL

### Windows

Download the `.msi` Windows installer for your system; execute the installer and follow the onscreen instructions. The default target installation location for the driver files is `C:\Program Files\AWSpsqlODBC`.
Four driver will be installed:
- AWS ANSI ODBC Driver for PostgreSQL
- AWS ANSI ODBC Driver for PostgreSQL (x64)
- AWS Unicode ODBC Driver for PostgreSQL
- AWS Unicode ODBC Driver for PostgreSQL (x64)
To uninstall the ODBC driver, open the same installer file, select the option to uninstall the driver and follow the onscreen instructions to successfully uninstall the driver.

### MacOS

In order to use the AWS ODBC Driver for PostgreSQL, [iODBC Driver Manager](http://www.iodbc.org/dataspace/doc/iodbc/wiki/iodbcWiki/Downloads) must be installed. `iODBC Driver Manager` contains the required libraries to install, configure the driver and DSN configurations.

// TODO

### Linux

In order to use the AWS ODBC Driver for PostgreSQL, [unixODBC](http://www.unixodbc.org/) must be installed.

For **Ubuntu 64 bit**:

```bash
sudo apt update
sudo apt install -y unixodbc
```

For **Amazon Linux using Graviton**:

```bash
sudo dnf update -y
sudo dnf install -y unixODBC
```

Once `unixODBC` is installed, download the `.tar.gz` file, and extract the contents to your desired location. For a Linux system, additional steps are required to configure the driver and Data Source Name (DSN) entries before the driver(s) can be used. For more information, see [Configuring the Driver and DSN settings](#configuring-the-driver-and-dsn-entries). There is no uninstaller at this time, but all the driver files can be removed by deleting the target installation directory.

### Configuring the Driver and DSN Entries
To configure the driver on Windows, use the `ODBC Data Source Administrator` tool to add or configure a DSN for either the `AWS ANSI ODBC Driver for PostgreSQL` or `AWS Unicode ODBC Driver for PostgreSQL`. With this DSN you can specify the options for the desired connection. Additional configuration properties are available by clicking the `Details >>` button.

To use the driver on MacOS or Linux systems, you need to create two files (`odbc.ini` and `odbcinst.ini`), that will contain the configuration for the driver and the Data Source Name (DSN).

You can modify the files manually, or through tools with a GUI such as `iODBC Administrator` (available for MacOS). In the following sections, we show samples of `odbc.ini` and `odbcinst.ini` files that describe how an ANSI driver could be set up for a MacOS system. In a MacOS system, the `odbc.ini` and `odbcinst.ini` files are typically located in the `/Library/ODBC/` directory.

For a Linux system, the files would be similar, but the driver file would have the `.la` extension instead of the `.dylib` extension shown in the sample. On a Linux system, the `odbc.ini` and `odbcinst.ini` files are typically located in the `/etc` directory.

#### odbc.ini
```bash
[ODBC Data Sources]
awspsqlodbca = AWS Unicode ODBC Driver for PostgreSQL
awspsqlodbcw = AWS ANSI ODBC Driver for PostgreSQL

[awspsqlodbcw]
Driver                             = AWS Unicode ODBC Driver for PostgreSQL
SERVER                             = localhost
TOPOLOGY_REFRESH_RATE              = 30000
FAILOVER_TIMEOUT                   = 60000
FAILOVER_TOPOLOGY_REFRESH_RATE     = 5000
FAILOVER_WRITER_RECONNECT_INTERVAL = 5000
FAILOVER_READER_CONNECT_TIMEOUT    = 30000
CONNECT_TIMEOUT                    = 30
NETWORK_TIMEOUT                    = 30

[awspsqlodbca]
Driver                             = AWS ANSI ODBC Driver for PostgreSQL
SERVER                             = localhost
FAILOVER_MODE                      = 30000
FAILOVER_TIMEOUT                   = 60000
CONNECT_TIMEOUT                    = 30
NETWORK_TIMEOUT                    = 30
```

#### odbcinst.ini
```bash
[ODBC Drivers]
AWS Unicode ODBC Driver for PostgreSQL = Installed
AWS ANSI ODBC Driver for PostgreSQL   = Installed

[AWS Unicode ODBC Driver for PostgreSQL]
Driver = awspsqlodbcw.la

[AWS ANSI ODBC Driver for PostgreSQL]
Driver = awspsqlodbca.la
```
