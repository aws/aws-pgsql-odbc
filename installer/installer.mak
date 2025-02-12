

# All the driver files that will be included in the installer
DRIVER_FILES = ../$(TARGET_CPU)_Unicode_$(CFG)/awspsqlodbcw.dll \
	../$(TARGET_CPU)_Unicode_$(CFG)/pgxalib.dll \
	../$(TARGET_CPU)_Unicode_$(CFG)/pgenlist.dll \
	../$(TARGET_CPU)_ANSI_$(CFG)/awspsqlodbca.dll \
	../$(TARGET_CPU)_ANSI_$(CFG)/pgxalib.dll \
	../$(TARGET_CPU)_ANSI_$(CFG)/pgenlista.dll

ALL: $(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msm $(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msi

CANDLE="$(WIX)bin\candle.exe"
LIGHT="$(WIX)bin\light"

!INCLUDE ..\windows-defaults.mak
!IF EXISTS(..\windows-local.mak)
!INCLUDE ..\windows-local.mak
!ENDIF

!MESSAGE determining product code

!INCLUDE productcodes.mak

!MESSAGE Got product code $(PRODUCTCODE)

MSM_OPTS = -dLIBPQBINDIR="$(LIBPQ_BIN)"

# Merge module
$(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msm: psqlodbcm_cpu.wxs $(DRIVER_FILES)
	echo Building Installer Merge Module
        $(CANDLE) -nologo -dLIBPQMEM0="libssl-3-x64.dll" -dLIBPQMEM1="libcrypto-3-x64.dll" -dLIBPQMEM2="libintl-9.dll" -dLIBPQMEM3="libwinpthread-1.dll" -dLIBPQMEM4="libiconv-2.dll" -dLIBPQMEM5="" -dLIBPQMEM6="" -dLIBPQMEM7="" -dLIBPQMEM8="" -dLIBPQMEM9="" -dPlatform="$(TARGET_CPU)" -dVERSION=$(PG_VER) -dLIBPQBINDIR=$(PG_BIN) -dLIBPQMSVCDLL="" -dLIBPQMSVCSYS="" -dPODBCMSVCDLL=$(VCRT_DLL) -dPODBCMSVPDLL=$(MSVCP_DLL) -dPODBCMSVCSYS="" -dPODBCMSVPSYS="" -dNoPDB="False" -dBINBASE=".." -o .\x64\psqlodbcm.wixobj psqlodbcm_cpu.wxs
	$(LIGHT) -nologo -o $(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msm $(TARGET_CPU)\psqlodbcm.wixobj

$(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msi: psqlodbc_cpu.wxs $(DRIVER_FILES)
	echo Building Installer
	$(CANDLE) -nologo -dPlatform="$(TARGET_CPU)" -dVERSION=$(POSTGRESDRIVERVERSION) -dSUBLOC=$(SUBLOC) -dPRODUCTCODE=$(PRODUCTCODE) -o $(TARGET_CPU)\psqlodbc.wixobj psqlodbc_cpu.wxs
	$(LIGHT) -nologo -ext WixUIExtension -cultures:en-us -o $(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msi $(TARGET_CPU)\psqlodbc.wixobj
	cscript modify_msi.vbs $(TARGET_CPU)\awspsqlodbc_$(TARGET_CPU).msi

clean:
	-rd /Q /S x64 x86
