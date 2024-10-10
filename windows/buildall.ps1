<#
Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2.0
(GPLv2), as published by the Free Software Foundation, with the
following additional permissions:

This program is distributed with certain software that is licensed
under separate terms, as designated in a particular file or component
or in the license documentation. Without limiting your rights under
the GPLv2, the authors of this program hereby grant you an additional
permission to link the program and your derivative works with the
separately licensed software that they have included with the program.

Without limiting the foregoing grant of rights under the GPLv2 and
additional permission as to separately licensed software, this
program is also subject to the Universal FOSS Exception, version 1.0,
a copy of which can be found along with its FAQ at
http://oss.oracle.com/licenses/universal-foss-exception.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License, version 2.0, for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see 
http://www.gnu.org/licenses/gpl-2.0.html.
/#>

<#
.PARAMETER Configuration
    Specify "Release"(default) or "Debug".
/#>

Param(
    [ValidateSet("Debug", "Release")]
    [String]$Configuration="Release"    
)

$CURRENT_DIR = (Get-Location).Path
$AWS_RDS_ODBC_DIR = "${PSScriptRoot}\..\aws-rds-odbc"

Set-Location $AWS_RDS_ODBC_DIR

# Build the AWS SDK
Write-Host "Building the AWS SDK"
& .\scripts\build_aws_sdk_win.ps1 x64 $Configuration OFF "Visual Studio 17 2022"
if ($LASTEXITCODE -ne 0) {
    Write-Host "AWS SDK build failed"
    exit $LASTEXITCODE
}

# Prep the aws-rds-odbc build
Write-Host "Prepping the aws-rds-odbc build"
cmake -S . -B build
if ($LASTEXITCODE -ne 0) {
    Write-Host "aws-rds-odbc build prep failed"
    exit $LASTEXITCODE
}

# Build aws-rds-odbc
Write-Host "Building aws-rds-odbc"
cmake --build build --config $Configuration
if ($LASTEXITCODE -ne 0) {
    Write-Host "aws-rds-odbc build failed"
    exit $LASTEXITCODE
}

# Build aws-pgsql-odbc
Write-Host "Building aws-pgsql-odbc"
Set-Location $CURRENT_DIR
& .\winbuild\BuildAll.ps1 -P x64 -UseMimalloc -C $Configuration
