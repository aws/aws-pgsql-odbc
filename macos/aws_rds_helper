#!/bin/bash
#
# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

true=1
false=0

# ---------------- Security Group Operations ----------------------
function add_ip_to_db_sg {
    LocalIp=$1
    ClusterId=$2
    Region=$3

    # Get the security group associated with the DB cluster using AWS CLI
    dbClusterInfo=$(aws rds describe-db-clusters --db-cluster-identifier $ClusterId --region $Region)

    if [ $? -ne 0 ]; then
        echo "Failed to get DB cluster information."
        exit 1
    fi

    # Check if we got the DB Cluster information
    dbClustersCount=$(echo "$dbClusterInfo" | jq -r '.DBClusters | length')
    if [ $dbClustersCount -eq 0 ]; then
        echo "Error: DB Cluster with ID '$ClusterId' not found in region '$Region'."
        exit 1
    fi

    securityGroupId=$(echo "$dbClusterInfo" | jq -r '.DBClusters[0].VpcSecurityGroups[0].VpcSecurityGroupId')
    cidrBlock="$LocalIp/32"

    # Allow inbound traffic from the local IP address to the DB security group using AWS CLI
    aws ec2 authorize-security-group-ingress --group-id $securityGroupId --protocol tcp --cidr $cidrBlock --port 5432 --region $Region || true
} # function add_ip_to_db_sg
export -f add_ip_to_db_sg

function remove_ip_from_db_sg {
    LocalIp=$1
    ClusterId=$2
    Region=$3

    # Get the security group associated with the DB cluster using AWS CLI
    dbClusterInfo=$(aws rds describe-db-clusters --db-cluster-identifier $ClusterId --region $Region)

    if [ $? -ne 0 ]; then
        echo "Failed to get DB cluster information."
        exit 1
    fi

    # Check if we got the DB Cluster information
    dbClustersCount=$(echo "$dbClusterInfo" | jq -r '.DBClusters | length')
    if [ $dbClustersCount -eq 0 ]; then
        echo "Error: DB Cluster with ID '$ClusterId' not found in region '$Region'."
        exit 1
    fi

    # Extract the Security Group ID
    securityGroupId=$(echo "$dbClusterInfo" | jq -r '.DBClusters[0].VpcSecurityGroups[0].VpcSecurityGroupId')

    # Define the CIDR block for the local IP address (allowing all ports and all traffic)
    cidrBlock="$LocalIp/32"

    # Revoke Inbound traffic
    aws ec2 revoke-security-group-ingress --group-id $securityGroupId --protocol tcp --cidr $cidrBlock --port 5432 --region $Region || true

    # Check if revoked successfully
    if [ $? -eq 0 ]; then
        echo "Removed IP: $LocalIp from security group $securityGroupId."
    else
        echo "Failed to remove ip $LocalIp from security group $securityGroupId."
    fi
} # function remove_ip_from_db_sg
export -f remove_ip_from_db_sg

# ---------------- Create DB operations ----------------------
function create_aurora_rds_cluster {
    TestUsername=$1
    TestPassword=$2
    TestDatabase=$3
    ClusterId=$4
    NumInstances=$5
    Engine=$6
    EngineVersion=$7
    Region=$8

    echo "Creating RDS Cluster"

    # Create RDS Cluster
    ClusterInfo=$(
        aws rds create-db-cluster\
            --db-cluster-identifier $ClusterId\
            --database-name $TestDatabase\
            --master-username $TestUsername\
            --master-user-password $TestPassword\
            --source-region $Region\
            --enable-iam-database-authentication\
            --engine  $Engine\
            --engine-version $EngineVersion\
            --storage-encrypted\
            --tags "Key=env,Value=test-runner"
    )

    if [ $? -ne 0 ]; then
        echo "Failed to create RDS Cluster."
        exit 1
    fi

    i=1

    while [ $i -le $NumInstances ]
    do
        aws rds create-db-instance\
            --db-cluster-identifier $ClusterId\
            --db-instance-identifier  "$ClusterId-$i"\
            --db-instance-class "db.r5.large"\
            --engine $Engine\
            --engine-version $EngineVersion\
            --publicly-accessible\
            --tags "Key=env,Value=test-runner"

        if [ $? -ne 0 ]; then
            echo "Failed to create DB Instance."
            exit 1
        fi

        ((i++))
    done

    aws rds wait db-instance-available\
        --filters "Name=db-cluster-id,Values=${ClusterId}"
} # function create_aurora_rds_cluster
export -f create_aurora_rds_cluster

function create_limitless_rds_cluster {
    TestUsername=$1
    TestPassword=$2
    TestDatabase=$3
    ClusterId=$4
    ShardId=$5
    Engine=$6
    EngineVersion=$7
    AwsRdsMonitoringRoleArn=$8
    Region=$9

    echo "Creating Limitless RDS Cluster"

    # Create Limitless RDS Cluster
    ClusterInfo=$(
        aws rds create-db-cluster\
            --cluster-scalability-type "limitless"\
            --db-cluster-identifier $ClusterId\
            --master-username $TestUsername\
            --master-user-password $TestPassword\
            --region $Region\
            --engine $Engine\
            --engine-version $EngineVersion\
            --enable-cloudwatch-logs-export "postgresql"\
            --enable-iam-database-authentication\
            --enable-performance-insights\
            --monitoring-interval 5\
            --performance-insights-retention-period 31\
            --monitoring-role-arn $AwsRdsMonitoringRoleArn\
            --storage-type "aurora-iopt1"\
            --tags "Key=env,Value=test-runner"
    )

    if [ $? -ne 0 ]; then
        echo "Failed to create Limitless RDS Cluster."
        exit 1
    fi

    echo "Creating Limitless Shard Group"

    ShardInfo=$(
        aws rds create-db-shard-group\
            --db-cluster-identifier $ClusterId\
            --db-shard-group-identifier $ShardId\
            --min-acu 28.0\
            --max-acu 601.0\
            --publicly-accessible\
            --tags "Key=env,Value=test-runner"
    )

    if [ $? -ne 0 ]; then
        echo "Failed to create Limitless Shard Group."
        exit 1
    fi

    # Wait for availability for limitless
    maxRetries=2
    attempt=0
    waitSuccessful=$false

    # Retry logic
    while [[ $attempt -le $maxRetries && $waitSuccessful -ne $true ]]
    do
        echo "Attempt $((attempt + 1)): Checking DB cluster availability..."

        # Call the AWS CLI to wait for DB cluster to be available
        aws rds wait db-cluster-available --db-cluster-identifier "$ClusterId"

        if [ $? -eq 0 ]; then
            # If the command succeeds, it will return nothing, so we set the success flag
            waitSuccessful=$true
            echo "DB Cluster is now available."
            break
        else
            echo "Error: DB cluster is not available. Attempt $((attempt + 1)) failed."
            ((attempt++))
            if [ $attempt -le $maxRetries ]; then
                echo "Retrying... ($((maxRetries - attempt)) retries left)"
                sleep 30 # Wait for 30 seconds before retrying
            fi
        fi
    done

    if [ $waitSuccessful -ne $true ]; then
        echo "Failed to wait for DB cluster availability after $((maxRetries + 1)) attempts."
        exit 1
    else
        echo "Successfully detected DB cluster availability."
    fi
} # function create_limitless_rds_cluster
export -f create_limitless_rds_cluster

# ---------------- Db Deletion operations ----------------------
function delete_dbshards {
    ShardId=$1

    # AWS CLI command to delete a DB shard
    aws rds delete-db-shard-group --db-shard-group-identifier $ShardId
} # delete_dbshards
export -f delete_dbshards

function delete_dbcluster {
    ClusterId=$1
        
    # AWS CLI command to delete the DB cluster
    aws rds delete-db-cluster --db-cluster-identifier $ClusterId --skip-final-snapshot
} # delete_dbcluster
export -f delete_dbcluster

function delete_dbinstances {
    ClusterId=$1
    NumInstances=$2
    
    echo "Deleting DB"
    
    i=1

    while [ $i -le $NumInstances ]
    do
        aws rds delete-db-instance --skip-final-snapshot --db-instance-identifier "$ClusterId-$i"
        ((i++))
    done
} # delete_dbinstances
export -f delete_dbinstances

function delete_aurora_db_cluster {
    ClusterId=$1
    Region=$2
    NumInstances=$3

    delete_dbinstances $ClusterId $NumInstances
    delete_dbcluster $ClusterId
} # delete_aurora_db_cluster
export -f delete_aurora_db_cluster

function delete_limitless_db_cluster {
    ClusterId=$1
    ShardId=$2
    Region=$3

    # Retry settings
    maxRetries=5
    attempt=0
    deleteShardsSuccessful=$false
    deleteClusterSuccessful=$false

    # Retry logic for deleting DB shards
    while [[ $attempt -lt $maxRetries && $deleteShardsSuccessful -eq $false ]]
    do
        delete_dbshards $ShardId
        if [ $? -ne 0 ]; then
            deleteShardsSuccessful=$false
        else
            deleteShardsSuccessful=$true
        fi
        ((attempt++))

        if [ $deleteShardsSuccessful -ne $true ]; then
            if [ $attempt -lt $maxRetries ]; then
                attemptsLeft=$((maxRetries - attempt))
                echo "Retrying DB shard deletion... ($attemptsLeft retries left)"
                sleep 30  # Wait for 30 seconds before retrying
            fi
        fi
    done

    if [ $deleteShardsSuccessful -ne $true ]; then
        echo "Failed to delete DB shard $ShardId after $maxRetries attempts."
        exit 1
    else
        echo "Successfully deleted DB shard $ShardId."
    fi

    # Reset attempt counter for DB cluster deletion
    attempt=0

    # Retry logic for deleting DB cluster
    while [[ $attempt -lt $maxRetries && $deleteClusterSuccessful -eq $false ]]
    do
        delete_dbcluster $ClusterId
        if [ $? -ne 0]; then
            deleteClusterSuccessful=$false
        else
            deleteClusterSuccessful=$true
        fi
        ((attempt++))

        if [ $deleteClusterSuccessful -ne $true ]; then
            if [ $attempt -lt $maxRetries ]; then
                attemptsLeft=$((maxRetries - attempt))
                echo "Retrying DB cluster deletion... ($attemptsLeft retries left)"
                sleep 30  # Wait for 30 seconds before retrying
            fi
        fi
    done

    if [ $deleteClusterSuccessful -ne $true ]; then
        echo "Failed to delete DB cluster $ClusterId after $maxRetries attempts."
        exit 1
    else
        echo "Successfully deleted DB cluster $ClusterId."
    fi
} # delete_limitless_db_cluster
export -f delete_limitless_db_cluster

# ---------------- Get Cluster endpoint ----------------------
function get_cluster_endpoint {
    ClusterId=$1

    # Get the DB cluster details using AWS CLI and jq
    output=$(aws rds describe-db-clusters --db-cluster-identifier $ClusterId)
    if [ $? -ne 0 ]; then
        echo "Failed to get cluster endpoint."
        exit 1
    fi
    endpoint=$(echo "$output" | jq -r '.DBClusters[0].Endpoint')

    if [ "$endpoint" = "null" ]; then
        echo "Failed to get cluster endpoint."
        exit 1
    else
        echo $endpoint
    fi
} # get_cluster_endpoint
export -f get_cluster_endpoint

# ---------------- Secrets Manager Operations ----------------------
function create_db_secrets {
    Username=$1
    Password=$2
    Engine=$3
    ClusterEndpoint=$4

    # Define the secret name (you can adjust this if you want a different name)
    secretName="AWS-PGSQL-ODBC-Tests-$ClusterEndpoint"

    # Create a dictionary to hold key-value pairs for the secret
    jsonSecretValue=$(
        jq -n \
            --arg username "$Username" \
            --arg password "$Password" \
            --arg engine "$Engine" \
            --arg host "$ClusterEndpoint" \
            '{username: $username, password: $password, engine: $engine, host: $host}'
    )

    jsonResponse=$(
        aws secretsmanager create-secret\
            --name $secretName\
            --description "Secrets created by GH actions for DB auth"\
            --secret-string "$jsonSecretValue"
    )

    # Parse the ARN of the newly created secret from the output
    secretArn=$(echo $jsonResponse | jq -r '.ARN')
    echo $secretArn
} # create_db_secrets
export -f create_db_secrets

function delete_secrets {
    SecretsArn=$1
    aws secretsmanager delete-secret --secret-id $SecretsArn
} # delete_secrets
export -f delete_secrets
