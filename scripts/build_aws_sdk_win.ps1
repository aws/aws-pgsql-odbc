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

$CURRENT_DIR = (Get-Location).Path
$SRC_DIR = "${PSScriptRoot}\..\aws_sdk\aws_sdk_cpp"
$BUILD_DIR = "${SRC_DIR}\..\build"
$INSTALL_DIR = "${BUILD_DIR}\..\install"
$AWS_SDK_CPP_TAG = "1.11.407"

$WIN_ARCH = $args[0]
$CONFIGURATION = $args[1]
$BUILD_SHARED_LIBS = $args[2]
$GENERATOR = $args[3]

Write-Host $args

# Make AWS SDK source directory
New-Item -Path $SRC_DIR -ItemType Directory -Force | Out-Null
# Clone the AWS SDK CPP repo
git clone --recurse-submodules -b "$AWS_SDK_CPP_TAG" "https://github.com/aws/aws-sdk-cpp.git" $SRC_DIR

# Make and move to build directory
New-Item -Path $BUILD_DIR -ItemType Directory -Force | Out-Null
Set-Location $BUILD_DIR

# Configure and build 
cmake $SRC_DIR `
    -A $WIN_ARCH `
    -G $GENERATOR `
    -D TARGET_ARCH="WINDOWS" `
    -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR `
    -D CMAKE_BUILD_TYPE=$CONFIGURATION `
    -D BUILD_ONLY="sts;rds" `
    -D ENABLE_TESTING="OFF" `
    -D BUILD_SHARED_LIBS=$BUILD_SHARED_LIBS `
    -D CPP_STANDARD="17"

# Build AWS SDK and install to $INSTALL_DIR 
msbuild ALL_BUILD.vcxproj /m /p:Configuration=$CONFIGURATION
msbuild INSTALL.vcxproj /m /p:Configuration=$CONFIGURATION

Set-Location $CURRENT_DIR
