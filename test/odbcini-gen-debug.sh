#!/bin/sh
#
#	This isn't a test application.
#	Initial setting of odbc(inst).ini with debug and trace enabled
#   Debug and trace logs are written to the ./logs directory
#
outini=odbc.ini
outinstini=odbcinst.ini

drvr=../.libs/psqlodbcw
driver=${drvr}.so
if test ! -e $driver ; then
	driver=${drvr}.dll
	if test ! -e $driver ; then
		echo Failure:driver ${drvr}.so\(.dll\) not found
		exit 2
	fi
fi

drvra=../.libs/psqlodbca
drivera=${drvra}.so
if test ! -e $drivera ; then
	drivera=${drvra}.dll
	if test ! -e $drivera ; then
		echo Failure:driver ${drvra}.so\(.dll\) not found
		exit 2
	fi
fi

mkdir -p ./logs

echo creating $outinstini
cat << _EOT_ > $outinstini
[ODBC]
Trace = on
TraceFile = ./logs/aws-pgsql-odbc-trace.log
[AWS ODBC Driver PostgreSQL ANSI]
Logdir = /users/kwedinger/logs
[AWS ODBC Driver PostgreSQL Unicode]
Logdir = /users/kwedinger/logs
[PostgreSQL Unicode]
Description     = AWS ODBC Driver PostgreSQL (Unicode version), for regression tests
Driver          = $driver
[PostgreSQL ANSI]
Description     = AWS ODBC Driver PostgreSQL (ANSI version), for regression tests
Driver          = $drivera
_EOT_

echo creating $outini: $@
# Unicode
cat << _EOT_ > $outini
[psqlodbc_test_dsn]
Description             = awspsqlodbc regression test DSN
Driver          		= PostgreSQL Unicode
Debug       			= 1
Trace       			= Yes
TraceFile   			= ./logs/psqlodbc_test_dsn_trace.log
Database                = contrib_regression
Servername              =
Username                =
Password                =
Port                    =
ReadOnly                = No
RowVersioning           = No
ShowSystemTables                = No
ShowOidColumn           = No
FakeOidIndex            = No
ConnSettings            = set lc_messages='C'
_EOT_

# Add any extra options from the command line
for opt in "$@"
do
    echo "${opt}" >> $outini
done

# ANSI
cat << _EOT_ >> $outini
[psqlodbc_test_dsn_ansi]
Description             = awspsqlodbc ansi regression test DSN
Driver          		= PostgreSQL ANSI
Debug       			= 1
Trace       			= Yes
TraceFile   			= ./logs/psqlodbc_test_dsn_ansi_trace.log
Database                = contrib_regression
Servername              =
Username                =
Password                =
Port                    =
ReadOnly                = No
RowVersioning           = No
ShowSystemTables                = No
ShowOidColumn           = No
FakeOidIndex            = No
ConnSettings            = set lc_messages='C'
_EOT_

# Add any extra options from the command line
for opt in "$@"
do
    echo "${opt}" >> $outini
done
