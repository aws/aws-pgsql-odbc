# Sign a single file
function Create-Aurora-RDS-Cluster {
    [OutputType([String])]
    Param(
        # The path to the file to sign
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
        [string]$EngineVersion

    )

    Write-Host "Creating RDS Cluster"

    # Create RDS Cluster
    $ClusterInfo = $( aws rds create-db-cluster `
                        --db-cluster-identifier $ClusterId `
                        --database-name $TestDatabase `
                        --master-username $TestUsername `
                        --master-user-password $TestPassword `
                        --source-region $ `
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
                        --storage-type "aurora-iopt1" `
                        --tags "Key=env,Value=test-runner")

    Write-Host "Creating Limitless Shard Group"

    $ShardInfo = $( aws rds create-db-shard-group `
                        --db-cluster-identifier $ClusterId `
                        --db-shard-group-identifier $ShardId `
                        --min-acu 28.0 `
                        --max-acu 601.0 `
                        --publicly-accessible ` )

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

}

# Function to delete DB shards
function Delete-DBShards {
    param([string]$ShardId)
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



# Function to delete DB cluster
function Delete-DBCluster {
    param([string]$ClusterId)
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



# Function to delete DB shards
function Delete-DBInstances {
    param(
        [string]$ClusterId,
        [string]$NumInstances
    )
    
    Write-Host "Deleting DB "
    for($i=1; $i -le $NumInstances; $i++) {
        aws rds delete-db-instance --skip-final-snapshot --db-instance-identifier "$($ClusterId)-$($i)"
    }
}
function Delete-Aurora-Db-Cluster {
    param(
        [string]$ClusterId,  # DB Cluster ID
        [int]$NumInstances # Number of instances
    )

    Delete-DBInstances -ClusterId $ClusterId -NumInstances $NumInstances
    Delete-DBCluster -ClusterId $ClusterId
}
function Delete-Limitless-Db-Cluster {
    param(
        [string]$ClusterId,  # DB Cluster ID
        [string]$ShardId     # DB Shard ID
    )
    
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