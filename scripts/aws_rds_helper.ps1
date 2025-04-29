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

# ---------------- Create DB operations ----------------------
function Create-Aurora-RDS-Cluster {
    [OutputType([String])]
    Param(
        [Parameter(Mandatory=$true)]
        [string]$TestUsername,
        # The path to the signed file
        [Parameter(Mandatory=$true)]
        [string]$TestPassword,
        # Test Database
        [Parameter(Mandatory=$true)]
        [string]$TestDatabase,
        # Cluster Id
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        # NumInstances
        [Parameter(Mandatory=$true)]
        [int]$NumInstances,
        # The name of the signed AWS bucket
        [Parameter(Mandatory=$true)]
        [string]$Engine,
        # Engine version
        [Parameter(Mandatory=$true)]
        [string]$EngineVersion,
        [Parameter(Mandatory=$true)]
        [string]$Region

    )

    Write-Host "Creating RDS Cluster"

    # Create RDS Cluster
    $ClusterInfo = $( aws rds create-db-cluster `
                        --db-cluster-identifier $ClusterId `
                        --database-name $TestDatabase `
                        --master-username $TestUsername `
                        --master-user-password $TestPassword `
                        --source-region $Region `
                        --enable-iam-database-authentication `
                        --engine  $Engine `
                        --engine-version $EngineVersion `
                        --storage-encrypted `
                        --tags "Key=env,Value=test-runner") `

    for($i=1; $i -le $NumInstances; $i++) {
        aws rds create-db-instance `
            --db-cluster-identifier $ClusterId `
            --db-instance-identifier  "${ClusterId}-${i}" `
            --db-instance-class "db.r5.large" `
            --engine $Engine `
            --engine-version $EngineVersion `
            --publicly-accessible `
            --tags "Key=env,Value=test-runner"
    }

    aws rds wait db-instance-available `
        --filters "Name=db-cluster-id,Values=${ClusterId}"

    Add-Ip-To-Db-Sg -ClusterId $ClusterId -Region $Region
}

function Create-Limitless-RDS-Cluster {
    [OutputType([String])]
    Param(
        # The path to the file to sign
        [Parameter(Mandatory=$true)]
        [string]$TestUsername,
        # The path to the signed file
        [Parameter(Mandatory=$true)]
        [string]$TestPassword,
        [Parameter(Mandatory=$true)]
        [string]$TestDatabase,
        # Cluster Id
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        # Shard Id
        [Parameter(Mandatory=$true)]
        [string]$ShardId,
        # The name of the signed AWS bucket
        [Parameter(Mandatory=$true)]
        [string]$Engine,
        # Engine version
        [Parameter(Mandatory=$true)]
        [string]$EngineVersion,
        # Aws Monitoring Arn
        [Parameter(Mandatory=$true)]
        [string]$AwsRdsMonitoringRoleArn,
        [Parameter(Mandatory=$true)]
        [string]$Region

    )

    Write-Host "Creating Limitless RDS Cluster"

    # Create RDS Cluster
    $ClusterInfo = $( aws rds create-db-cluster `
                        --cluster-scalability-type "limitless" `
                        --db-cluster-identifier $ClusterId `
                        --master-username $TestUsername `
                        --master-user-password $TestPassword `
                        --region $Region `
                        --engine  $Engine `
                        --engine-version $EngineVersion `
                        --enable-cloudwatch-logs-export "postgresql" `
                        --enable-iam-database-authentication `
                        --enable-performance-insights `
                        --monitoring-interval 5 `
                        --performance-insights-retention-period 31 `
                        --monitoring-role-arn $AwsRdsMonitoringRoleArn `
                        --storage-type "aurora-iopt1" `
                        --tags "Key=env,Value=test-runner")

    Write-Host "Creating Limitless Shard Group"

    $ShardInfo = $( aws rds create-db-shard-group `
                        --db-cluster-identifier $ClusterId `
                        --db-shard-group-identifier $ShardId `
                        --min-acu 28.0 `
                        --max-acu 601.0 `
                        --publicly-accessible `
                        --tags "Key=env,Value=test-runner")

    # Wait for availability for limitless
    $maxRetries = 2
    $attempt = 0
    $waitSuccessful = $false

    # Retry logic
    while ($attempt -le $maxRetries -and -not $waitSuccessful) {
        try {
            Write-Host "Attempt $($attempt + 1): Checking DB cluster availability..."

            # Call the AWS CLI to wait for DB cluster to be available
            $result = aws rds wait db-cluster-available --db-cluster-identifier $ClusterId

            # If the command succeeds, it will return nothing, so we set the success flag
            $waitSuccessful = $true
            Write-Host "DB Cluster is now available."

        } catch {

            Write-Host "Error: DB cluster is not available. Attempt $($attempt + 1) failed."
            $attempt++
            if ($attempt -le $maxRetries) {
                Write-Host "Retrying... ($($maxRetries - $attempt) retries left)"
                Start-Sleep -Seconds 30  # Wait for 30 seconds before retrying
            }
        }
    }

    if (-not $waitSuccessful) {
        Write-Host "Failed to wait for DB cluster availability after $($maxRetries + 1) attempts."

        throw [System.Exception] "Failed to wait for Cluster availability"

    } else {
        Write-Host "Successfully detected DB cluster availability."
    }

    Add-Ip-To-Db-Sg -ClusterId $ClusterId -Region $Region
}

# ---------------- Security Group Operations ----------------------
function Add-Ip-To-Db-Sg {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        [Parameter(Mandatory=$true)]
        [string]$Region
    )

    # Get the security group associated with the DB cluster using AWS CLI
    $dbClusterInfo = aws rds describe-db-clusters --db-cluster-identifier $ClusterId --region $Region | ConvertFrom-Json

    # Check if we got the DB Cluster information
    if ($dbClusterInfo.DBClusters.Count -eq 0) {
        Write-Host "Error: DB Cluster with ID '$ClusterId' not found in region '$Region'."
        return
    }

    $securityGroupId = $dbClusterInfo.DBClusters[0].VpcSecurityGroups[0].VpcSecurityGroupId

    # Get the local IP address of the machine running the script
    $localIp = (Invoke-RestMethod -Uri "http://checkip.amazonaws.com").Trim()
    $cidrBlock = "$localIp/32"

    # Allow inbound traffic from the local IP address to the DB security group using AWS CLI
    $authorizeResult = aws ec2 authorize-security-group-ingress --group-id $securityGroupId --protocol tcp --cidr $cidrBlock --port 5432 --region $Region

    # Check if the ingress rule was successfully added
    if ($?) {
        Write-Host "Inbound traffic allowed from IP $localIp to security group $securityGroupId on all ports."
    } else {
        Write-Host "Failed to add inbound rule to security group $securityGroupId."
    }
}

function Remove-Ip-From-Db-Sg {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        [Parameter(Mandatory=$true)]
        [string]$Region
    )

    # Get the security group associated with the DB cluster using AWS CLI
    $dbClusterInfo = aws rds describe-db-clusters --db-cluster-identifier $ClusterId --region $Region | ConvertFrom-Json

    # Check if we got the DB Cluster information
    if ($dbClusterInfo.DBClusters.Count -eq 0) {
        Write-Host "Error: DB Cluster with ID '$ClusterId' not found in region '$Region'."
        return
    }

    # Extract the Security Group ID
    $securityGroupId = $dbClusterInfo.DBClusters[0].VpcSecurityGroups[0].VpcSecurityGroupId

    # Get the local IP address of the machine running the script
    $localIp = (Invoke-RestMethod -Uri "http://checkip.amazonaws.com").Trim()

    # Define the CIDR block for the local IP address (allowing all ports and all traffic)
    $cidrBlock = "$localIp/32"

    # Revoke Inbound traffic
    $authorizeResult = aws ec2 revoke-security-group-ingress --group-id $securityGroupId --protocol tcp --cidr $cidrBlock --port 0-65535 --region $Region

    # Check if revoked successfully
    if ($?) {
        Write-Host "Removed IP: $localIp from security group $securityGroupId."
    } else {
        Write-Host "Failed to remove ip $localIp from security group $securityGroupId."
    }
}

# ---------------- Db Deletion operations ----------------------
function Delete-DBShards {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ShardId
    )
    try {
        Write-Host "Attempt $($attempt + 1): Deleting DB shard $ShardId..."
            
        # AWS CLI command to delete a DB shard
        $result = aws rds delete-db-shard-group --db-shard-group-identifier $ShardId

        Write-Host "DB Shard $ShardId deleted successfully."
        return $true
    } catch {
        Write-Host "Error deleting DB shard $ShardId. Attempt $($attempt + 1) failed."
        return $false
    }
}

function Delete-DBCluster {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId
    )
    try {
        Write-Host "Attempt $($attempt + 1): Deleting DB cluster $ClusterId..."
            
        # AWS CLI command to delete the DB cluster
        $result = aws rds delete-db-cluster --db-cluster-identifier $ClusterId --skip-final-snapshot

        Write-Host "DB Cluster $ClusterId deletion initiated."
        return $true
    } catch {
        Write-Host "Error deleting DB cluster $ClusterId. Attempt $($attempt + 1) failed."
        return $false
    }
}

function Delete-DBInstances {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        [Parameter(Mandatory=$true)]
        [string]$NumInstances
    )
    
    Write-Host "Deleting DB "
    for($i=1; $i -le $NumInstances; $i++) {
        aws rds delete-db-instance --skip-final-snapshot --db-instance-identifier "$($ClusterId)-$($i)"
    }
}

function Delete-Aurora-Db-Cluster {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        [Parameter(Mandatory=$true)]
        [string]$Region,
        [Parameter(Mandatory=$true)]
        [int]$NumInstances
    )
    Remove-Ip-From-Db-Sg -ClusterId $ClusterId -Region $Region
    Delete-DBInstances -ClusterId $ClusterId -NumInstances $NumInstances
    Delete-DBCluster -ClusterId $ClusterId
}

function Delete-Limitless-Db-Cluster {
    param(
        [Parameter(Mandatory=$true)]
        [string]$ClusterId,
        [Parameter(Mandatory=$true)]
        [string]$ShardId,
        [Parameter(Mandatory=$true)]
        [string]$Region
    )
    Remove-Ip-From-Db-Sg -ClusterId $ClusterId -Region $Region
    # Retry settings
    $maxRetries = 5
    $attempt = 0
    $deleteShardsSuccessful = $false
    $deleteClusterSuccessful = $false

    # Retry logic for deleting DB shards
    while ($attempt -lt $maxRetries -and -not $deleteShardsSuccessful) {
        $deleteShardsSuccessful = Delete-DBShards -ShardId $ShardId
        $attempt++
        if (-not $deleteShardsSuccessful) {
            if ($attempt -lt $maxRetries) {
                Write-Host "Retrying DB shard deletion... ($($maxRetries - $attempt) retries left)"
                Start-Sleep -Seconds 30  # Wait for 30 seconds before retrying
            }
        }
    }

    if (-not $deleteShardsSuccessful) {
        Write-Host "Failed to delete DB shard $ShardId after $maxRetries attempts."
    } else {
        Write-Host "Successfully deleted DB shard $ShardId."
    }

    # Reset attempt counter for DB cluster deletion
    $attempt = 0

    # Retry logic for deleting DB cluster
    while ($attempt -lt $maxRetries -and -not $deleteClusterSuccessful) {
        $deleteClusterSuccessful = Delete-DBCluster -ClusterId $ClusterId
        $attempt++
        if (-not $deleteClusterSuccessful) {
            if ($attempt -lt $maxRetries) {
                Write-Host "Retrying DB cluster deletion... ($($maxRetries - $attempt) retries left)"
                Start-Sleep -Seconds 30  # Wait for 30 seconds before retrying
            }
        }
    }

    if (-not $deleteClusterSuccessful) {
        Write-Host "Failed to delete DB cluster $ClusterId after $maxRetries attempts."
    } else {
        Write-Host "Successfully deleted DB cluster $ClusterId."
    }
}

# ---------------- Get Cluster endpoint ----------------------
function Get-Cluster-Endpoint {
    param (
        [Parameter(Mandatory=$true)]
        [string]$ClusterId
    )

    try {
        # Get the DB cluster details using AWS CLI and jq
        $endpoint = aws rds describe-db-clusters --db-cluster-identifier $ClusterId | jq -r '.DBClusters[0].Endpoint'
        
        # Check if the endpoint was retrieved
        if ($endpoint -and $endpoint -ne "null") {
            return $endpoint
        } else {
            Write-Host "No cluster found with ID: $ClusterId"
            return $null
        }
    } catch {
        Write-Host "Error: $_"
        return $null
    }
}

# ---------------- Secrets Manager Operations ----------------------
function Create-Db-Secrets {
    param(
        [Parameter(Mandatory=$true)]
        [string]$Username,
        [Parameter(Mandatory=$true)]
        [string]$Password,
        [Parameter(Mandatory=$true)]
        [string]$Engine,
        [Parameter(Mandatory=$true)]
        [string]$ClusterEndpoint
    )

    $randomNumber = Get-Random -Minimum 1000 -Maximum 9999
    $randomNumber = $randomNumber.ToString()
    # Define the secret name (you can adjust this if you want a different name)
    $secretName = "AWS-PGSQL-ODBC-Tests-$ClusterEndpoint-$randomNumber"

    # Create a dictionary to hold key-value pairs for the secret
    $secretValue = @{
        "username" = $Username
        "password" = $Password
        "engine" = $Engine
        "host" = $ClusterEndpoint
    }

    $jsonSecretValue = $secretValue | ConvertTo-Json
    $createSecretCommand = aws secretsmanager create-secret `
        --name $secretName `
        --description "Secrets created by GH actions for AWS PostgreSQL ODBC" `
        --secret-string $jsonSecretValue

    # Parse the ARN of the newly created secret from the output
    $secretArn = ($createSecretCommand | ConvertFrom-Json).ARN
    return $secretArn
}

function Delete-Secrets {
    param(
        [Parameter(Mandatory=$true)]
        [string]$SecretsArn
    )
    aws secretsmanager delete-secret --secret-id $SecretsArn
}

