# Building the AWS ODBC Driver for PostgreSQL

## Table of Contents
- [Windows](#windows)
    - [Prerequisites](#prerequisites)
    - [Build the driver](#build-the-driver)
- [macOS](#macos)
    - [Prerequisites](#prerequisites-1)
    - [Build the driver](#build-the-driver-1)

## Windows

### Prerequisites
1. Download the CMake Windows x64 Installer from https://cmake.org/download/ and install CMake using the installer. When going through the install, ensure that adding CMake to the PATH is selected.
1. Refer to [Install Microsoft Visual Studio](https://github.com/aws/aws-rds-odbc/blob/main/docs/InstallMicrosoftVisualStudio.md) to install Microsoft Visual Studio.
1. Add the path to `dumpbin.exe` to the Path user environment variable by doing the following inside PowerShell.
    1. Change to the Visual Studio installation directory.
    This is usually `C:\Program Files\Microsoft Visual Studio`.
    1. Run `gci -r -fi dumpbin.exe` and note the directory whose path ends with `Hostx64\x64`.
    1. Add this directory for `dumpbin.exe` to the Path user environment variable.
1. Use the following steps inside PowerShell to install WiX.
   ```PowerShell
   dotnet tool install --global wix
   wix extension add --global WixToolset.UI.wixext`
   ```
1. Download and run the latest 17.x Windows x86-64 installer of PostgreSQL from https://www.enterprisedb.com/downloads/postgres-postgresql-downloads. Accept all of the defaults during the installation. During the installation, make note of the directory used during the installation. This will be needed below.
1. Run `.\editConfiguration.bat` to open the GUI to edit the PostgreSQL settings.
1. Specify and save the PostgreSQL client library installation directories as shown below using the PostgreSQL installation directory noted above.

![Configure PostgreSQL directories](../img/ConfigurePostgreSQLDirectories.png?raw=true "Configure PostgreSQL directories")

### Build the driver
Inside PowerShell, run `.\windows\buildall.ps1`. This builds the following.
- AWS SDK for C++
- AWS RDS Library for ODBC Drivers(https://github.com/aws/aws-rds-odbc)
- The driver
- The Windows installer for the driver

## macOS

### Prerequisites
1. Install [Homebrew](https://brew.sh/). If Homebrew is already installed, run the following command to install the latest updates.
   ```bash
   brew update && brew upgrade && brew cleanup
   ```
1. Run the following command to install the build dependencies.
   ```bash
   brew install autoconf automake cmake curl googletest libtool libpq unixodbc zlib
   ```
1. Install the Xcode Command Line tools using one of the following methods.
   1. Install Xcode from the App Store
   1. Run the following command
   ```bash
   xcode-select --install
   ```
1. Download and install the iODBC Driver Manager from
   [iODBC Driver Manager: iODBC Downloads](https://www.iodbc.org/wiki/iodbcWiki/Downloads). When the installation is complete, add `/Library/Application Support/iODBC/bin` to the `PATH` environment variable.
   Example:
   ```Bash
   export PATH=PATH:/Library/Application Support/iODBC/bin`
   ```

### Build the driver
Inside a terminal, run `./macos/buildall Release`. This builds the following.
- AWS SDK for C++
- AWS RDS Library for ODBC Drivers(https://github.com/aws/aws-rds-odbc)
- The driver
