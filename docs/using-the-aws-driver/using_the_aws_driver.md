# Using the AWS ODBC Driver for PostgreSQL

The AWS ODBC Driver for PostgreSQL is drop-in compatible, so its usage is identical to the [psqlodbc](https://github.com/postgresql-interfaces/psqlodbc).
This driver currently supports the following features:
- [AWS Authentication](./authentication/authentication.md)
- [Limitless](./limitless/limitless.md)
- [Failover](./failover/failover.md)

## Logging

### Logging on Windows

Logs from the AWS ODBC Driver for PostgreSQL are automatically generated.
By default, they are outputted to `C:\Users\<username>\AppData\Local\Temp\aws-rds-odbc\`.
