# Testing the AWS ODBC Driver for PostgreSQL

## Table of Contents
- [Prerequisites](#prerequisites)
- [Unit Tests](#unit-tests)
    - [Windows](#windows)
    - [macOS](#macos)
- [Integration Tests](#integration-tests)
    - [Prerequisites](#prerequisites-1)
    - [Community Tests](#community-tests)

## Prerequisites
Follow the instructions inside [Building the AWS ODBC Driver for PostgreSQL](./Build.md) to build the driver.

## Unit Tests

### Windows

Inside PowerShell, run the following.
```PowerShell
.\winbuild\regress.ps1 -Platform x64 -ExpectMimalloc -ReinstallDriver
```

After the tests are built, complete the prompts to configure the connection to the PostgreSQL server.
Accepting the defaults will use the locally installed PostgreSQL server. After completing the prompts,
click on "Yes" when prompted to make changes to your device.

Each successful test will report `ok` following by the test number and its name.
Example: `ok 47 - fetch-refcursors`

Output for each will be in the `winbuild\test_x64\results` directory.

### macOS

Inside a terminal, change to the `test` subdirectory and run the following,
replacing `<host>`, `<user>` and `<password>` with the host, user and password for the
locally installed PostgreSQL instance.
```bash
PGHOST=<host> PGUSER=<user> PGPASSWORD=<password> make installcheck
```
Example:
```bash
PGHOST=localhost PGUSER=postgres PGPASSWORD=postgres make installcheck
```

Successful test output will look like this.
```
connect .................... ok
stmthandles ................ ok
select ..................... ok
update ..................... ok
commands ................... ok
multistmt .................. ok
getresult .................. ok
colattribute ............... ok
result-conversions ......... ok
prepare .................... ok
premature .................. ok
params ..................... ok
param-conversions .......... ok
parse ...................... ok
identity ................... ok
notice ..................... ok
arraybinding ............... ok
insertreturning ............ ok
dataatexecution ............ ok
boolsaschar ................ ok
cvtnulldate ................ ok
alter ...................... ok
quotes ..................... ok
cursors .................... ok
cursor-movement ............ ok
cursor-commit .............. ok
cursor-name ................ ok
cursor-block-delete ........ ok
bookmark ................... ok
declare-fetch-commit ....... ok
declare-fetch-block ........ ok
positioned-update .......... ok
bulkoperations ............. ok
catalogfunctions ........... ok
bindcol .................... ok
lfconversion ............... ok
cte ........................ ok
errors ..................... ok
error-rollback ............. ok
diagnostic ................. ok
numeric .................... ok
large-object ............... ok
large-object-data-at-exec .. ok
odbc-escapes ............... ok
wchar-char ................. ok
params-batch-exec .......... ok
fetch-refcursors ........... ok
descrec .................... ok
All tests successful.
```

## Integration Tests

### Prerequisites
1. Download and install Docker Desktop using one of the links below.
    * [Install Docker Desktop on Windows](https://docs.docker.com/desktop/setup/install/windows-install/)
    * [Install Docker Desktop on macOS](https://docs.docker.com/desktop/setup/install/mac-install/)
1. Download and install [Amazon Corretto OpenJDK 17](https://docs.aws.amazon.com/corretto/latest/corretto-17-ug/downloads-list.html).
1. Clone a fresh copy of the `aws-pgsql-odbc` repository and its submodules using the command below. This will be used
   to build the driver used during the integration tests.
    ```
    git clone --recurse-submodules git@github.com:aws/aws-pgsql-odbc.git
    ```
1. Set the following environment variables.
    |Environment Variable|Description|Example|
    |-|-|-|
    |AWS_ACCESS_KEY_ID|AWS access key ID used for authentication and authorization||
    |AWS_SECRET_ACCESS_KEY|AWS secret access key used for authentication and authorization||
    |AWS_RDS_MONITORING_ROLE_ARN|ARN for the IAM role that permits RDS to send Enhanced Monitoring metrics to Amazon CloudWatch Logs|arn:aws:iam:123456789012:role/emaccess|
    |AWS_SESSION_TOKEN|AWS session token used for authentication and authorization||
    |DRIVER_PATH|Path to clone of `aws-pgsql-odbc` repository created above|/Users/dev/projects/aws-pgsql-odbc|
    |TEST_USERNAME|PostgreSQL username, used to create user in Docker PostgreSQL instance|postgres|
    |TEST_PASSWORD|PostgreSQL password, used to create user in Docker PostgreSQL instance|test|
1. On Windows, start PowerShell. On macOS, start Terminal.
1. Change to the `testframework` subdirectory under the `aws-pgsql-odbc` repository cloned above.
1. **macOS only**: When running the `gradlew` commands below, replace `.\` with `./`.

### Community Tests
Run `.\gradlew --no-parallel test-community --info`

#### Notes
Running the community tests will take several minutes to complete. The latest timings
are below.

|System|Time to complete|
|-|-|
|Apple M3 Max Macbook Pro with 36 GB of memory|17m 43s|
|HP EliteBook with Intel Core Ultra 1.7GHz and 32 GB of memory|33m 25s|

The `descrec` test is currently failing. The test diff is below.
```
*** ./expected/descrec.out	Tue Nov 19 16:49:02 2024
--- results/descrec.out	Tue Nov 19 17:08:52 2024
***************
*** 19,25 ****
  -- Column 3 --
  SQL_DESC_NAME: col3
  SQL_DESC_TYPE: 12
! SQL_DESC_OCTET_LENGTH: 40
  SQL_DESC_PRECISION: 0
  SQL_DESC_SCALE: 0
  SQL_DESC_NULLABLE: 0
--- 19,25 ----
  -- Column 3 --
  SQL_DESC_NAME: col3
  SQL_DESC_TYPE: 12
! SQL_DESC_OCTET_LENGTH: 10
  SQL_DESC_PRECISION: 0
  SQL_DESC_SCALE: 0
  SQL_DESC_NULLABLE: 0
```
Column 3 above inside the test is created via `SQLExecDirect` with the following which seems to align with the action test result above (`SQL_DESC_OCTET_LENGTH: 10`).
```
col3 varchar(10) not null
```
This is not something introduced as part of [this pull request](https://github.com/aws/aws-pgsql-odbc/pull/33).
This behaviour is only seen in Dockerized test environments. This test is passing when run as part of the GitHub CIs.

### Integration Tests
1. Set additional environment variables
|Environment Variable|Description|Example|
|-|-|-|
|TEST_IAM_USER|IAM username, used to create a user in the database with IAM authentication|my_iam_user|
1. Run `.\gradlew --no-parallel test-integration --info`
