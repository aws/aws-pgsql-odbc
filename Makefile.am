#-------------------------------------------------------------------------
#
# Makefile.am for psqlodbc30w (PostgreSQL ODBC driver)
#
#-------------------------------------------------------------------------

AUTOMAKE_OPTIONS = 1.8 foreign

ACLOCAL_AMFLAGS = -I config

pkglib_LTLIBRARIES = awspsqlodbcw.la awspsqlodbca.la

awspsqlodbcw_la_CPPFLAGS = -DUNICODE_SUPPORT -DSQL_WCHART_CONVERT

if WITH_AWSRDSODBC
# -module MUST be part of _LDFLAGS to prevent libtool reporting
# the following error: libtool library 'awspsqlodbcw.la' must begin with 'lib'
# See https://www.gnu.org/software/automake/manual/html_node/Libtool-Modules.html.
awspsqlodbcw_la_LDFLAGS = -module -L@AWS_RDS_ODBC_PATH@/build_unicode -laws-rds-odbc-w
awspsqlodbca_la_LDFLAGS = -module -L@AWS_RDS_ODBC_PATH@/build_ansi -laws-rds-odbc-a
else
awspsqlodbcw_la_LDFLAGS = -module
awspsqlodbca_la_LDFLAGS = -module
endif

AM_CFLAGS = @CFLAGS@
AM_CPPFLAGS = @CPPFLAGS@
AM_CXXFLAGS = @CXXFLAGS@
AM_LDFLAGS = @LDFLAGS@ -no-undefined -avoid-version -export-symbols-regex '^SQL'

awspsqlodbca_la_SOURCES = \
	info.c bind.c columninfo.c connection.c convert.c drvconn.c \
	environ.c execute.c lobj.c misc.c options.c \
	pgtypes.c psqlodbc.c qresult.c results.c parse.c \
	statement.c tuple.c dlg_specific.c \
	multibyte.c odbcapi.c descriptor.c \
	odbcapi30.c pgapi30.c mylog.c \
	secure_sscanf.c \
\
	bind.h catfunc.h columninfo.h connection.h convert.h \
	descriptor.h dlg_specific.h environ.h unicode_support.h \
	lobj.h misc.h multibyte.h pgapifunc.h pgtypes.h \
	psqlodbc.h qresult.h resource.h statement.h tuple.h \
	version.h pgenlist.h mylog.h xalibname.h \
	secure_sscanf.h

awspsqlodbcw_la_SOURCES = $(awspsqlodbca_la_SOURCES) \
	odbcapi30w.c odbcapiw.c win_unicode.c


EXTRA_DIST = license.txt readme.txt readme_winbuild.txt \
	psqlodbc.def psqlodbca.def editConfiguration.bat BuildAll.bat \
	windows-defaults.mak buildInstallers.bat regress.bat \
	pgenlist.def pgenlista.def connexp.h \
	dlg_wingui.c loadlib.c loadlib.h inouealc.c win_setup.h \
        psqlsetup.c setup.c win_unicode.c psqlodbc.rc win64.mak \
        psqlodbc.reg psqlodbc.dsp psqlodbc.vcproj \
        psqlodbc.sln msdtc_enlist.cpp pgxalib.cpp xalibname.c \
        pgxalib.def psqlsetup.def  psqlsetupa.def odbc.sql odbc-drop.sql \
\
	docs/build-and-test-aws-driver/build.md \
	docs/build-and-test-aws-driver/test.md \
	docs/img/ConfigurePostgreSQLDirectories.png \
	docs/img/adfs.png \
	docs/img/connection_pool.png \
	docs/img/database.png \
	docs/img/failover_behavior.png \
	docs/img/failover_diagram.png \
	docs/img/failover_dsn.png \
	docs/img/iam.png \
	docs/img/limitless.png \
	docs/img/okta.png \
	docs/img/secrets_manager.png \
	docs/using-the-aws-driver/authentication/adfs_authentication_sample.cpp \
	docs/using-the-aws-driver/authentication/adfs_authentication.md \
	docs/using-the-aws-driver/authentication/authentication.md \
	docs/using-the-aws-driver/authentication/database_authentication.md \
	docs/using-the-aws-driver/authentication/iam_authentication_sample.cpp \
	docs/using-the-aws-driver/authentication/iam_authentication.md \
	docs/using-the-aws-driver/authentication/okta_authentication_sample.cpp \
	docs/using-the-aws-driver/authentication/okta_authentication.md \
	docs/using-the-aws-driver/authentication/secrets_manager_authentication_sample.cpp \
	docs/using-the-aws-driver/authentication/secrets_manager_authentication.md \
	docs/using-the-aws-driver/authentication/simple_database_connection_sample.cpp \
	docs/using-the-aws-driver/failover/failover_sample_code.cpp \
	docs/using-the-aws-driver/failover/failover.md \
	docs/using-the-aws-driver/limitless/limitless_sample.cpp \
	docs/using-the-aws-driver/limitless/limitless.md \
	docs/using-the-aws-driver/using_the_aws_driver.md \
	docs/config-opt.html \
	docs/config.html \
	docs/getting_started.md \
	docs/msdtc_pgxalib_tracing_disable.reg \
	docs/msdtc_pgxalib_tracing_enable.reg \
	docs/msdtc_pgxalib_tracing_README.txt \
	docs/readme.md \
\
	installer/background.bmp \
	installer/banner.bmp \
	installer/buildInstallers.ps1 \
	installer/installer.mak \
	installer/lgpl.rtf \
	installer/modify_msi.vbs \
	installer/productcodes.mak \
	installer/psqlodbc-setup \
	installer/README.txt \
	installer/psqlodbcm_cpu.wxs \
	installer/psqlodbc_cpu.wxs \
	installer/psqlodbc-setup/buildBootstrapper.ps1 \
	installer/psqlodbc-setup/Bundle.wxs \
	installer/psqlodbc-setup/Make.bat \
	installer/psqlodbc-setup/psqlodbc-setup.wixproj \
	installer/psqlodbc-setup/vcredist.wxs \
\
	winbuild/BuildAll.ps1 \
	winbuild/platformbuild.vcxproj \
	winbuild/MSProgram-Get.psm1 \
	winbuild/psqlodbc.Cpp.props \
	winbuild/Psqlodbc-config.psm1 \
	winbuild/psqlodbc.vcxproj \
	winbuild/configuration_template.xml \
	winbuild/psqlsetup.vcxproj \
	winbuild/editConfiguration.ps1 \
	winbuild/readme.txt \
	winbuild/pgenlist.vcxproj \
	winbuild/regress.ps1 \
	winbuild/pguser.Cpp.props \
	winbuild/regress_one.vcxproj \
	winbuild/pgxalib.vcxproj \
	winbuild/std_dbmsname.Cpp.props \
\
	test/Makefile.in \
	test/odbcini-gen.sh \
	test/README.txt \
	test/RegisterRegdsn.c \
	test/reset-db.c \
	test/runsuite.c \
	test/sampletables.sql \
	test/tests \
	test/win.mak \
	test/expected \
	test/src

MAINTAINERCLEANFILES = \
	Makefile.in config/config.guess config.h.in config/config.sub configure \
	config/install-sh config/missing aclocal.m4 config/ltmain.sh \
	config/depcomp
