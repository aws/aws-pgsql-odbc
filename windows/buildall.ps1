<#
Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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
$AWS_RDS_ODBC_DIR = "${PSScriptRoot}\..\libs\aws-rds-odbc"

Set-Location $AWS_RDS_ODBC_DIR

# Build the AWS SDK
Write-Host "Building the AWS SDK"
& .\scripts\build_aws_sdk_win.ps1 x64 $Configuration OFF "Visual Studio 17 2022"
if ($LASTEXITCODE -ne 0) {
    Write-Host "AWS SDK build failed"
    Set-Location $CURRENT_DIR
    exit $LASTEXITCODE
}

# Prep the aws-rds-odbc builds
Write-Host "Prepping the ansi aws-rds-odbc build"
if ($Configuration -eq "Debug") {
    cmake -S . -B build_ansi -DCMAKE_BUILD_TYPE=Debug
} else {
    cmake -S . -B build_ansi
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "ansi aws-rds-odbc build prep failed"
    Set-Location $CURRENT_DIR
    exit $LASTEXITCODE
}

Write-Host "Prepping the unicode aws-rds-odbc build"
if ($Configuration -eq "Debug") {
    cmake -S . -B build_unicode -DUNICODE_BUILD=ON -DCMAKE_BUILD_TYPE=Debug
} else {
    cmake -S . -B build_unicode -DUNICODE_BUILD=ON
}

if ($LASTEXITCODE -ne 0) {
    Write-Host "unicode aws-rds-odbc build prep failed"
    Set-Location $CURRENT_DIR
    exit $LASTEXITCODE
}

# Build aws-rds-odbc
Write-Host "Building ansi version of aws-rds-odbc"
cmake --build build_ansi --config $Configuration
if ($LASTEXITCODE -ne 0) {
    Write-Host "ansi aws-rds-odbc build failed"
    Set-Location $CURRENT_DIR
    exit $LASTEXITCODE
}

Write-Host "Building unicode version of aws-rds-odbc"
cmake --build build_unicode --config $Configuration
if ($LASTEXITCODE -ne 0) {
    Write-Host "unicode aws-rds-odbc build failed"
    Set-Location $CURRENT_DIR
    exit $LASTEXITCODE
}

# Build aws-pgsql-odbc
Write-Host "Building aws-pgsql-odbc"
Set-Location $CURRENT_DIR
& .\winbuild\BuildAll.ps1 -P x64 -UseMimalloc -C $Configuration
if ($LASTEXITCODE -ne 0) {
    Write-Host "aws-pgsql-odbc build failed"
    exit $LASTEXITCODE
}

# Build the installer during release builds only
if ($Configuration -ieq "Release") {
    Write-Host "Building the installer"
    & .\installer\buildInstallers.ps1 x64
    if ($LASTEXITCODE -ne 0) {
        Write-Host "installer build failed"
        exit $LASTEXITCODE
    }
}