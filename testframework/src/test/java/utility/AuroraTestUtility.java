// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, version 2.0
// (GPLv2), as published by the Free Software Foundation, with the
// following additional permissions:
//
// This program is distributed with certain software that is licensed
// under separate terms, as designated in a particular file or component
// or in the license documentation. Without limiting your rights under
// the GPLv2, the authors of this program hereby grant you an additional
// permission to link the program and your derivative works with the
// separately licensed software that they have included with the program.
//
// Without limiting the foregoing grant of rights under the GPLv2 and
// additional permission as to separately licensed software, this
// program is also subject to the Universal FOSS Exception, version 1.0,
// a copy of which can be found along with its FAQ at
// http://oss.oracle.com/licenses/universal-foss-exception.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License, version 2.0, for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see
// http://www.gnu.org/licenses/gpl-2.0.html.

package utility;

import org.jetbrains.annotations.NotNull;
import org.json.JSONObject;
import software.amazon.awssdk.auth.credentials.AwsBasicCredentials;
import software.amazon.awssdk.auth.credentials.AwsCredentialsProvider;
import software.amazon.awssdk.auth.credentials.AwsSessionCredentials;
import software.amazon.awssdk.auth.credentials.DefaultCredentialsProvider;
import software.amazon.awssdk.auth.credentials.StaticCredentialsProvider;
import software.amazon.awssdk.core.waiters.WaiterOverrideConfiguration;
import software.amazon.awssdk.core.waiters.WaiterResponse;
import software.amazon.awssdk.regions.Region;
import software.amazon.awssdk.services.ec2.Ec2Client;
import software.amazon.awssdk.services.ec2.model.DescribeSecurityGroupsResponse;
import software.amazon.awssdk.services.ec2.model.Ec2Exception;
import software.amazon.awssdk.services.rds.RdsClient;
import software.amazon.awssdk.services.rds.RdsClientBuilder;
import software.amazon.awssdk.services.rds.model.*;
import software.amazon.awssdk.services.rds.waiters.RdsWaiter;
import software.amazon.awssdk.services.secretsmanager.SecretsManagerClient;
import software.amazon.awssdk.services.secretsmanager.model.CreateSecretRequest;
import software.amazon.awssdk.services.secretsmanager.model.CreateSecretResponse;
import software.amazon.awssdk.services.secretsmanager.model.DeleteSecretRequest;
import software.amazon.awssdk.services.secretsmanager.model.SecretsManagerException;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.net.URI;
import java.net.URISyntaxException;
import java.net.URL;
import java.net.UnknownHostException;
import java.time.Duration;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.TimeUnit;

/**
 * Creates and destroys AWS RDS Cluster and Instances AWS Credentials is loaded using
 * DefaultAWSCredentialsProviderChain Environment Variable > System Properties > Web Identity Token
 * > Profile Credentials > EC2 Container To specify which to credential provider, use
 * AuroraTestUtility(String region, AWSCredentialsProvider credentials) *
 * <p>
 * If using environment variables for credential provider Required - AWS_ACCESS_KEY -
 * AWS_SECRET_KEY
 */
public class AuroraTestUtility {
  private static final double MIN_ACU = 28.0;
  private static final double MAX_ACU = 601.0;

  private static final int MAX_WAIT_RETRIES = 90;

  private static final String LIMITLESS_ENGINE = "aurora-postgresql";
  private static final String LIMITLESS_ENGINE_VERSION = "16.4-limitless";
  private static final String MONITORING_ROLE_ARN = "arn:aws:iam::851725167871:role/rds-monitoring-role";

  // Default values
  private final Region dbRegion;
  private final String dbEngine = LIMITLESS_ENGINE;
  private final String dbSecGroup = "default";

  private final RdsClient rdsClient;
  private final Ec2Client ec2Client;
  private SecretsManagerClient smClient = null;

  private static final String DUPLICATE_IP_ERROR_CODE = "InvalidPermission.Duplicate";

  /**
   * Initializes an AmazonRDS & AmazonEC2 client.
   *
   * @param region define AWS Regions, refer to https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html
   * @param endpoint overrides the endpoint for the RDS client, e.g. "https://rds-int.amazon.com"
   */
  public AuroraTestUtility(String region, String endpoint) {
    this(getRegionInternal(region), endpoint, DefaultCredentialsProvider.create());
  }

  public AuroraTestUtility(
      String region, String rdsEndpoint, String awsAccessKeyId, String awsSecretAccessKey, String awsSessionToken) {

    this(
        getRegionInternal(region),
        rdsEndpoint,
        StaticCredentialsProvider.create(
            StringUtils.isNullOrEmpty(awsSessionToken)
                ? AwsBasicCredentials.create(awsAccessKeyId, awsSecretAccessKey)
                : AwsSessionCredentials.create(awsAccessKeyId, awsSecretAccessKey, awsSessionToken)));
  }

  /**
   * Initializes an AmazonRDS & AmazonEC2 client.
   *
   * @param region              define AWS Regions, refer to https://docs.aws.amazon.com/AmazonRDS/latest/UserGuide/Concepts.RegionsAndAvailabilityZones.html
   * @param credentialsProvider Specific AWS credential provider
   */
  public AuroraTestUtility(Region region, String rdsEndpoint, AwsCredentialsProvider credentialsProvider) {
    dbRegion = region;
    final RdsClientBuilder rdsClientBuilder = RdsClient.builder()
        .region(dbRegion)
        .credentialsProvider(credentialsProvider);

    if (!StringUtils.isNullOrEmpty(rdsEndpoint)) {
      try {
        rdsClientBuilder.endpointOverride(new URI(rdsEndpoint));
      } catch (URISyntaxException e) {
        throw new RuntimeException(e);
      }
    }

    rdsClient = rdsClientBuilder.build();
    ec2Client = Ec2Client.builder()
        .region(dbRegion)
        .credentialsProvider(credentialsProvider)
        .build();
    smClient = SecretsManagerClient.builder()
        .region(dbRegion)
        .build();
  }

  protected static Region getRegionInternal(String rdsRegion) {
    final String region = StringUtils.isNullOrEmpty(rdsRegion) ? "us-east-2" : rdsRegion;

    Optional<Region> regionOptional =
        Region.regions().stream().filter(r -> r.id().equalsIgnoreCase(region)).findFirst();

    if (regionOptional.isPresent()) {
      return regionOptional.get();
    }
    throw new IllegalArgumentException(String.format("Unknown AWS region '%s'.", region));
  }

  /**
   * Creates RDS Cluster/Instances and waits until they are up, and proper IP whitelisting for
   * databases.
   *
   * @param username          Master username for access to database
   * @param password          Master password for access to database
   * @param clusterIdentifier Database cluster identifier
   * @param shardIdentifier   Database shard identifier
   * @return An endpoint for one of the instances
   * @throws InterruptedException when clusters have not started after 30 minutes
   */
  public AuroraClusterInfo createLimitlessCluster(String username, String password, String clusterIdentifier, String shardIdentifier)
      throws InterruptedException {
    final Tag testRunnerTag = Tag.builder().key("env").value("test-runner").build();

    // Create the limitless cluster
    final CreateDbClusterRequest dbClusterRequest =
        CreateDbClusterRequest.builder()
            .clusterScalabilityType(ClusterScalabilityType.LIMITLESS)
            .databaseName(null)
            .dbClusterIdentifier(clusterIdentifier)
            .enableCloudwatchLogsExports("postgresql")
            .enableIAMDatabaseAuthentication(true)
            .enablePerformanceInsights(true)
            .engine(LIMITLESS_ENGINE)
            .engineVersion(LIMITLESS_ENGINE_VERSION)
            .masterUsername(username)
            .masterUserPassword(password)
            .monitoringInterval(5)
            .monitoringRoleArn(MONITORING_ROLE_ARN)
            .performanceInsightsRetentionPeriod(31)
            .storageType("aurora-iopt1")
            .tags(testRunnerTag)
            .build();
    rdsClient.createDBCluster(dbClusterRequest);

    // Create the limitless shard
    final CreateDbShardGroupRequest shardGroupRequest =
        CreateDbShardGroupRequest.builder()
            .dbClusterIdentifier(clusterIdentifier)
            .dbShardGroupIdentifier(shardIdentifier)
            .minACU(MIN_ACU)
            .maxACU(MAX_ACU)
            .build();
    rdsClient.createDBShardGroup(shardGroupRequest);

    DescribeDbClustersRequest describeDbClustersRequest = DescribeDbClustersRequest.builder()
        .dbClusterIdentifier(clusterIdentifier)
        .build();
    WaiterOverrideConfiguration waiterConfig = WaiterOverrideConfiguration.builder()
        .maxAttempts(MAX_WAIT_RETRIES)
        .waitTimeout(Duration.ofMinutes(MAX_WAIT_RETRIES))
        .build();
    final RdsWaiter waiter = rdsClient.waiter();
    WaiterResponse<DescribeDbClustersResponse> waiterResponse = waiter.waitUntilDBClusterAvailable(describeDbClustersRequest, waiterConfig);

    if (waiterResponse.matched().exception().isPresent()) {
      deleteLimitlessCluster(clusterIdentifier, shardIdentifier);
      throw new InterruptedException(
          "Unable to start AWS RDS Cluster & Instances after waiting for 90 minutes");
    }

    return getClusterInfo(clusterIdentifier);
  }

  private @NotNull AuroraClusterInfo getClusterInfo(String clusterIdentifier) {
    DescribeDbClustersRequest describeDbClustersRequest = DescribeDbClustersRequest.builder()
        .dbClusterIdentifier(clusterIdentifier)
        .build();

    final DescribeDbClustersResponse clustersResponse = rdsClient.describeDBClusters(describeDbClustersRequest);
    DBCluster cluster = clustersResponse.dbClusters().get(0);
    String endpoint = cluster.endpoint();
    String readerEndpoint = cluster.readerEndpoint();
    final String suffix = endpoint.substring(endpoint.indexOf('.') + 1);

    final List<String> instances = new ArrayList<>();
    instances.add(cluster.dbClusterIdentifier());
    return new AuroraClusterInfo(
        suffix,
        endpoint,
        readerEndpoint,
        instances
    );
  }

  /**
   * Gets public IP.
   *
   * @return public IP of user
   * @throws UnknownHostException when checkip host isn't available
   */
  public String getPublicIPAddress() throws UnknownHostException {
    String ip;
    try {
      URL ipChecker = new URL("http://checkip.amazonaws.com");
      BufferedReader reader = new BufferedReader(new InputStreamReader(ipChecker.openStream()));
      ip = reader.readLine();
    } catch (Exception e) {
      throw new UnknownHostException("Unable to get IP");
    }
    return ip;
  }

  /**
   * Authorizes IP to EC2 Security groups for RDS access.
   */
  public void ec2AuthorizeIP(String ipAddress) {
    if (StringUtils.isNullOrEmpty(ipAddress)) {
      return;
    }

    if (ipExists(ipAddress)) {
      return;
    }

    try {
      ec2Client.authorizeSecurityGroupIngress(
          (builder) ->
              builder
                  .groupName(dbSecGroup)
                  .cidrIp(ipAddress + "/32")
                  .ipProtocol("-1") // All protocols
                  .fromPort(0) // For all ports
                  .toPort(65535));
    } catch (Ec2Exception exception) {
      if (!DUPLICATE_IP_ERROR_CODE.equalsIgnoreCase(exception.awsErrorDetails().errorCode())) {
        throw exception;
      }
    }
  }

  private boolean ipExists(String ipAddress) {
    final DescribeSecurityGroupsResponse response =
        ec2Client.describeSecurityGroups(
            (builder) ->
                builder
                    .groupNames(dbSecGroup)
                    .filters(
                        software.amazon.awssdk.services.ec2.model.Filter.builder()
                            .name("ip-permission.cidr")
                            .values(ipAddress + "/32")
                            .build()));

    return response != null && !response.securityGroups().isEmpty();
  }

  /**
   * De-authorizes IP from EC2 Security groups.
   */
  public void ec2DeauthorizesIP(String ipAddress) {
    if (StringUtils.isNullOrEmpty(ipAddress.trim())) {
      return;
    }
    try {
      ec2Client.revokeSecurityGroupIngress(
          (builder) ->
              builder
                  .groupName(dbSecGroup)
                  .cidrIp(ipAddress + "/32")
                  .ipProtocol("-1") // All protocols
                  .fromPort(0) // For all ports
                  .toPort(65535));
    } catch (Ec2Exception exception) {
      // Ignore
    }
  }

  /**
   * Destroys the limitless cluster. Removes IP from EC2 whitelist.
   *
   * @param clusterIdentifier Limitless cluster identifier to delete
   * @param shardIdentifier   Limitless cluster shard identifier to delete
   */
  public void deleteLimitlessCluster(String clusterIdentifier, String shardIdentifier) {
    int remainingAttempts = 5;

    // Destroy the cluster's shard
    while (--remainingAttempts > 0) {
      try {
        DeleteDbShardGroupResponse response = rdsClient.deleteDBShardGroup(
            (builder -> builder.dbShardGroupIdentifier(shardIdentifier)));
        if (response.sdkHttpResponse().isSuccessful()) {
          break;
        }
        TimeUnit.SECONDS.sleep(30);
      } catch (Exception ex) {
        // ignore
      }
    }

    // Then, destroy the cluster
    remainingAttempts = 5;
    while (--remainingAttempts > 0) {
      try {
        DeleteDbClusterResponse response = rdsClient.deleteDBCluster(
            (builder -> builder.skipFinalSnapshot(true).dbClusterIdentifier(clusterIdentifier)));
        if (response.sdkHttpResponse().isSuccessful()) {
          break;
        }
        TimeUnit.SECONDS.sleep(30);

      } catch (Exception ex) {
        // ignore
      }
    }
  }

  public String createSecrets(String secretName, String secretValue) {
    try {
      CreateSecretRequest secretRequest = CreateSecretRequest.builder()
          .name(secretName)
          .description("Integration tests credentials for AWS MySQL ODBC Driver")
          .secretString(secretValue)
          .build();

      CreateSecretResponse secretResponse = smClient.createSecret(secretRequest);
      return secretResponse.arn();

    } catch (SecretsManagerException e) {
      System.err.println(e.awsErrorDetails().errorMessage());
      System.exit(1);
    }
    return "";
  }

  public String createSecretValue(String host, String username, String password) {
    return new JSONObject()
        .put("engine", "postgres")
        .put("host", host)
        .put("username", username)
        .put("password", password)
        .toString();
  }

  public void deleteSecrets(String secretsArn) {
    DeleteSecretRequest request = DeleteSecretRequest.builder()
        .secretId(secretsArn)
        .forceDeleteWithoutRecovery(true)
        .build();

    smClient.deleteSecret(request);
  }

  private String getAuroraDbEngineVersion(String engineVersion) {
    if (StringUtils.isNullOrEmpty(engineVersion)) {
      return this.getLTSVersion();
    }
    switch (engineVersion.toLowerCase()) {
      case "lts":
        return this.getLTSVersion();
      case "latest":
        return this.getLatestVersion();
      default:
        return engineVersion;
    }
  }

  private String getLatestVersion() {
    final DescribeDbEngineVersionsResponse versions = this.rdsClient.describeDBEngineVersions(
        DescribeDbEngineVersionsRequest.builder().engine(dbEngine).build()
    );

    return versions.dbEngineVersions()
        .stream()
        .map(DBEngineVersion::engineVersion)
        .max(Comparator.naturalOrder())
        .orElse(null);
  }

  private String getLTSVersion() {
    final DescribeDbEngineVersionsResponse versions = this.rdsClient.describeDBEngineVersions(
        DescribeDbEngineVersionsRequest.builder().defaultOnly(true).engine(dbEngine).build()
    );
    if (!versions.dbEngineVersions().isEmpty()) {
      return versions.dbEngineVersions().get(0).engineVersion();
    }
    throw new RuntimeException("Failed to find LTS version");
  }
}
