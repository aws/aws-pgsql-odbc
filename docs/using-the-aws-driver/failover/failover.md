# Failover

## Failover Process

// TODO

## Connection Strings and Configuring the Driver

To set up a connection, the driver uses an ODBC connection string. An ODBC connection string specifies a set of semicolon-delimited connection options. Typically, a connection string will either:

1. specify a Data Source Name containing a preconfigured set of options (`DSN=xxx;`) or
2. configure options explicitly (`SERVER=xxx;PORT=xxx;...;`). This option will override values set inside the DSN.

## Failover Specific Options

// TODO

### Driver Behaviour During Failover For Different Connection URLs

// TODO

### Host Pattern

// TODO

## Failover Exception Codes

// TODO

#### Sample Code

// TODO

> [!WARNING]\
> Warnings About Proper Usage of the AWS ODBC Driver for PostgreSQL
> It is highly recommended that you use the cluster and read-only cluster endpoints instead of the direct instance endpoints of your Aurora cluster, unless you are confident about your application's use of instance endpoints.
> Although the driver will correctly failover to the new writer instance when using instance endpoints, use of these endpoints is discouraged because individual instances can spontaneously change reader/writer status when failover occurs.
> The driver will always connect directly to the instance specified if an instance endpoint is provided, so a write-safe connection cannot be assumed if the application uses instance endpoints.
