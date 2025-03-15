// Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

package host;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.testcontainers.containers.Container;
import org.testcontainers.containers.GenericContainer;
import org.testcontainers.containers.Network;
import org.testcontainers.containers.ToxiproxyContainer;

import utility.AuroraClusterInfo;
import utility.AuroraTestUtility;
import utility.ContainerHelper;
import utility.StringUtils;

import java.io.IOException;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.jupiter.api.Assertions.fail;

public class IntegrationContainerTest {
  enum TestConfigurationEngine {
    LIMITLESS,
    AURORA_PG,
    COMMUNITY
  }

  private static final int POSTGRES_PORT = 5432;
  private static final String TEST_CONTAINER_NAME = "test-container";
  private static final String TEST_DATABASE = "test";
  private static final String TEST_DSN_UNICODE = System.getenv("TEST_DSN_UNICODE");
  private static final String TEST_DSN_ANSI = System.getenv("TEST_DSN_ANSI");
  private static final String TEST_USERNAME = !StringUtils.isNullOrEmpty(System.getenv("TEST_USERNAME")) ?
          System.getenv("TEST_USERNAME") : "my_test_username";
  private static final String TEST_PASSWORD = !StringUtils.isNullOrEmpty(System.getenv("TEST_PASSWORD")) ?
          System.getenv("TEST_PASSWORD") : "my_test_password";
  private static final String TEST_IAM_USER = !StringUtils.isNullOrEmpty(System.getenv("TEST_IAM_USER")) ?
          System.getenv("TEST_IAM_USER") : "my_test_iam_user";

  private static final String ACCESS_KEY = System.getenv("AWS_ACCESS_KEY_ID");
  private static final String SECRET_ACCESS_KEY = System.getenv("AWS_SECRET_ACCESS_KEY");
  private static final String SESSION_TOKEN =
      !StringUtils.isNullOrEmpty(System.getenv("AWS_SESSION_TOKEN")) ?
          System.getenv("AWS_SESSION_TOKEN") : "";
  private static final String ENDPOINT = System.getenv("RDS_ENDPOINT");
  private static final String REGION = System.getenv("RDS_REGION");

  private static final boolean REUSE_CLUSTER = Boolean.parseBoolean(System.getenv("REUSE_RDS_CLUSTER"));

  private static final String DOCKER_UID = "1001";
  private static final String COMMUNITY_SERVER = "postgres-instance";

  private static final String DRIVER_LOCATION = System.getenv("DRIVER_PATH");
  private static final String PROXIED_DOMAIN_NAME_SUFFIX = ".proxied";
  private static List<ToxiproxyContainer> proxyContainers = new ArrayList<>();
  private static String dbClusterIdentifier = System.getenv("RDS_CLUSTER_NAME");
  private static String dbShardGroupIdentifier = System.getenv("TEST_DB_SHARD_GROUP_IDENTIFIER");

  private static final String ODBCINI_LOCATION = "/app/build/test/odbc.ini";
  private static final String ODBCINSTINI_LOCATION = "/app/build/test/odbcinst.ini";

  private static final String DEFAULT_LIMITLESS_PREFIX = "postgres-odbc-limitless-";
  private static final String DEFAULT_APG_PREFIX = "postgres-odbc-apg-";
  private static final String DEFAULT_LIMITLESS_DATABASE = "postgres_limitless";

  private static final ContainerHelper containerHelper = new ContainerHelper();
  private static final AuroraTestUtility auroraUtil = new AuroraTestUtility(REGION, ENDPOINT);

  private static final String IODBC_VERSION = "3.52.16";
  private static final String UNIXODBC_VERSION = "2.3.12";

  private static TestConfigurationEngine testConfiguration;
  private static int postgresProxyPort;
  private static List<String> postgresInstances = new ArrayList<>();
  private static GenericContainer<?> testContainer;
  private static GenericContainer<?> postgresContainer;
  private static String dbHostCluster = "";
  private static String dbHostClusterRo = "";
  private static String runnerIP = null;
  private static String dbConnStrSuffix = "";
  private static String secretsArn = "";

  private static final Network NETWORK = Network.newNetwork();

  @BeforeAll
  static void setUp() {
    testContainer = createTestContainer(NETWORK);
  }

  @AfterAll
  static void tearDown() {
    if (!REUSE_CLUSTER
        && !StringUtils.isNullOrEmpty(ACCESS_KEY)
        && !StringUtils.isNullOrEmpty(SECRET_ACCESS_KEY)
        && !StringUtils.isNullOrEmpty(dbHostCluster)) {
      switch (testConfiguration) {
        case LIMITLESS:
          auroraUtil.deleteLimitlessCluster(dbClusterIdentifier, dbShardGroupIdentifier);
          break;
        case AURORA_PG:
          auroraUtil.deleteCluster();
          break;
        case COMMUNITY:
        default:
          break;
      }

      if (!StringUtils.isNullOrEmpty(secretsArn)) {
        auroraUtil.deleteSecrets(secretsArn);
      }

      auroraUtil.ec2DeauthorizesIP(runnerIP);

      for (ToxiproxyContainer proxy : proxyContainers) {
        proxy.stop();
      }
    }
    testContainer.stop();
    if (postgresContainer != null) {
      postgresContainer.stop();
    }
  }

  @Test
  public void testRunCommunityTestInContainer()
      throws UnsupportedOperationException, IOException, InterruptedException {

    testConfiguration = TestConfigurationEngine.COMMUNITY;
    setupCommunityTests(NETWORK);

    try {
      // Allow the non-root user to access this folder which contains log files. Required to run tests
      testContainer.execInContainer("chown", DOCKER_UID, "/app/build/test/Testing/Temporary");
    } catch (Exception e) {
      fail("Test container was not initialized correctly");
    }

    containerHelper.runCommunityTest(testContainer, "/app");
  }

  @Test
  public void testRunLimitlessTestInContainer()
      throws UnsupportedOperationException, IOException, InterruptedException {

    testConfiguration = TestConfigurationEngine.LIMITLESS;
    setupLimitlessIntegrationTests(NETWORK);

    System.out.println("Run ANSI integration tests");
    testContainer.setEnv(List.of("TEST_DSN=" + TEST_DSN_ANSI));
    containerHelper.runExecutable(testContainer, "build_ansi/bin", "integration");

    System.out.println("Run Unicode integration tests");
    testContainer.setEnv(List.of("TEST_DSN=" + TEST_DSN_UNICODE));
    containerHelper.runExecutable(testContainer, "build_unicode/bin", "integration");
  }

  @Test
  public void testRunIntegrationTestInContainer()
      throws UnsupportedOperationException, IOException, InterruptedException {

    testConfiguration = TestConfigurationEngine.AURORA_PG;
    setupIntegrationTests(NETWORK);

    displayIniFiles();

    System.out.println("Run ANSI integration tests");
    testContainer.setEnv(List.of("TEST_DSN=" + TEST_DSN_ANSI));
    containerHelper.runExecutable(testContainer, "build_ansi/bin", "integration");

    System.out.println("Run Unicode integration tests");
    testContainer.setEnv(List.of("TEST_DSN=" + TEST_DSN_UNICODE));
    containerHelper.runExecutable(testContainer, "build_unicode/bin", "integration");
  }

  protected static GenericContainer<?> createTestContainer(final Network network) {
    return containerHelper.createTestContainer(
            "odbc/rds-test-container",
            DRIVER_LOCATION)
        .withNetworkAliases(TEST_CONTAINER_NAME)
        .withNetwork(network)
        .withEnv("TEST_DSN_UNICODE", TEST_DSN_UNICODE)
        .withEnv("TEST_DSN_ANSI", TEST_DSN_ANSI)
        .withEnv("TEST_DSN", TEST_DSN_UNICODE) // for community tests
        .withEnv("TEST_USERNAME", TEST_USERNAME)
        .withEnv("TEST_PASSWORD", TEST_PASSWORD)
        .withEnv("POSTGRES_PORT", Integer.toString(POSTGRES_PORT))
        .withEnv("ODBCINI", ODBCINI_LOCATION)
        .withEnv("ODBCINST", ODBCINSTINI_LOCATION)
        .withEnv("ODBCSYSINI", "/app/build/test")
        .withEnv("TEST_DRIVER_UNICODE", "/app/.libs/awspsqlodbcw.so")
        .withEnv("TEST_DRIVER_ANSI", "/app/.libs/awspsqlodbca.so");
  }

  private void installPrerequisites(final boolean withIODBC) throws Exception {
    System.out.println("apt-get update");
    Container.ExecResult result = testContainer.execInContainer("apt-get", "update");
    System.out.println(result.getStdout());

    // Install dependencies w/ apt-get and auto-confirm with -y
    // Denies any configuration changes e.g. Unixodbc changing odbc.ini, using `yes n`
    List<String> packages = Arrays.asList(
        "autoconf",
        "automake",
        "build-essential",
        "cmake",
        "curl",
        "g++-10",
        "git",
        "grep",
        "libcurl4-openssl-dev",
        "libssl-dev",
        "libgflags-dev",
        "libodbc2",
        "libodbcinst2",
        "libpq-dev",
        "libtool-bin",
        "lsb-base",
        "lsb-release",
        "uuid-dev",
        "zlib1g-dev"
    );

    // Install dependencies w/ apt-get and auto-confirm with -y
    // Denies any configuration changes e.g. Unixodbc changing odbc.ini, using `yes n`
    System.out.println("yes n | apt-get install " + String.join(" ", packages) + " -y");
    result = testContainer.execInContainer("sh", "-c", "yes n | apt-get install " + String.join(" ", packages) + " -y");
    System.out.println(result.getStdout());

    if (withIODBC) {
      configureWithIODBC();
    } else {
      configureWithUnixOdbc();
    }
  }

  private void configureWithIODBC() throws IOException, InterruptedException {
    // Download and configure using IODBC
    System.out.println(
        "curl -L https://github.com/openlink/iODBC/releases/download/v" + IODBC_VERSION + "/libiodbc-" + IODBC_VERSION
            + ".tar.gz -o libiodbc.tar");
    Container.ExecResult result = testContainer.execInContainer("curl", "-L",
        "https://github.com/openlink/iODBC/releases/download/v" + IODBC_VERSION + "/libiodbc-" + IODBC_VERSION
            + ".tar.gz", "-o", "libiodbc.tar");
    System.out.println(result.getStdout());

    System.out.println("tar xf libiodbc.tar");
    result = testContainer.execInContainer("tar", "xf", "libiodbc.tar");
    System.out.println(result.getStdout());

    System.out.println("sh -c cd libiodbc-" + IODBC_VERSION + " && ./configure && make && make install");
    result = testContainer.execInContainer("sh", "-c", "cd libiodbc-" + IODBC_VERSION + " && ./configure && make && make install");
    System.out.println(result.getStdout());
  }

  private void configureWithUnixOdbc() throws IOException, InterruptedException {
    // We need to build and install unixODBC because the apt-get package
    // does not include odbc_config
    System.out.println("curl -L https://www.unixodbc.org/unixODBC-" + UNIXODBC_VERSION + ".tar.gz -o unixODBC.tar");
    Container.ExecResult result = testContainer.execInContainer("curl", "-L", "https://www.unixodbc.org/unixODBC-" + UNIXODBC_VERSION + ".tar.gz", "-o", "unixODBC.tar");
    System.out.println(result.getStdout());

    System.out.println("tar xf unixODBC.tar");
    result = testContainer.execInContainer("tar", "xf", "unixODBC.tar");
    System.out.println(result.getStdout());

    System.out.println("sh -c cd unixODBC-" + UNIXODBC_VERSION + " && ./configure && make && make install");
    result = testContainer.execInContainer("sh", "-c",
        "cd unixODBC-" + UNIXODBC_VERSION + " && " +
            "./configure && make && make install"
    );
  }

  private void buildDriver() {
    buildDriver(true);
  }

  private void buildDriver(final boolean withIODBC) {
    try {
      installPrerequisites(withIODBC);

      System.out.println("bash linux/buildall release " + withIODBC + " false");
      Container.ExecResult result = testContainer.execInContainer("bash", "linux/buildall", "release", String.valueOf(withIODBC), "false");
      System.out.println(result.getStdout());
    } catch (Exception e) {
      fail("Test container failed during driver/test building process.");
    }
  }

  private void buildLimitlessTests() {
    try {
      // build unicode integration tests
      System.out.println("cmake -S test_integration -B build_unicode -DTEST_LIMITLESS=TRUE -DUNICODE_BUILD=TRUE");
      Container.ExecResult result = testContainer.execInContainer("cmake", "-S", "test_integration", "-B", "build_unicode", "-DTEST_LIMITLESS=TRUE", "-DUNICODE_BUILD=TRUE");
      System.out.println(result.getStdout());

      System.out.println("cmake --build build_unicode");
      result = testContainer.execInContainer("cmake", "--build", "build_unicode");
      System.out.println(result.getStdout());

      // build ansi integration tests
      System.out.println("cmake -S test_integration -B build_ansi -DTEST_LIMITLESS=TRUE");
      result = testContainer.execInContainer("cmake", "-S", "test_integration", "-B", "build_ansi", "-DTEST_LIMITLESS=TRUE");
      System.out.println(result.getStdout());

      System.out.println("cmake --build build_ansi");
      result = testContainer.execInContainer("cmake", "--build", "build_ansi");
      System.out.println(result.getStdout());
    } catch (Exception e) {
      fail("Test container failed during driver/test building process.");
    }
  }

  private void buildIntegrationTests() {
    try {
      // build unicode integration tests
      System.out.println("cmake -S test_integration -B build_unicode -DUNICODE_BUILD=TRUE");
      Container.ExecResult result = testContainer.execInContainer("cmake", "-S", "test_integration", "-B", "build_unicode", "-DUNICODE_BUILD=TRUE");
      System.out.println(result.getStdout());

      System.out.println("cmake --build build_unicode");
      result = testContainer.execInContainer("cmake", "--build", "build_unicode");
      System.out.println(result.getStdout());

      // build ansi integration tests
      System.out.println("cmake -S test_integration -B build_ansi");
      result = testContainer.execInContainer("cmake", "-S", "test_integration", "-B", "build_ansi");
      System.out.println(result.getStdout());

      System.out.println("cmake --build build_ansi");
      result = testContainer.execInContainer("cmake", "--build", "build_ansi");
      System.out.println(result.getStdout());
    } catch (Exception e) {
      fail("Test container failed during driver/test building process.");
    }
  }

  private void displayIniFiles() {
    try {
      System.out.println("Using the following odbc.ini:");
      Container.ExecResult result = testContainer.execInContainer("cat", ODBCINI_LOCATION);
      System.out.println(result.getStdout());

      System.out.println("Using the following odbcinst.ini:");
      result = testContainer.execInContainer("cat", ODBCINSTINI_LOCATION);
      System.out.println(result.getStdout());

      System.out.println("Verifying unicode driver dependencies:");
      result = testContainer.execInContainer("ldd", "/app/.libs/awspsqlodbcw.so");
      System.out.println(result.getStdout());

      System.out.println("Verifying ansi driver dependencies:");
      result = testContainer.execInContainer("ldd", "/app/.libs/awspsqlodbca.so");
      System.out.println(result.getStdout());
    } catch (Exception e) {
      fail("Test container failed.");
    }
  }

  private void setupLimitlessTestContainer(final Network network) throws InterruptedException, UnknownHostException {
    if (!StringUtils.isNullOrEmpty(ACCESS_KEY) && !StringUtils.isNullOrEmpty(SECRET_ACCESS_KEY)) {
      // Comment out below to not create a new cluster & instances

      if (StringUtils.isNullOrEmpty(dbClusterIdentifier)) {
        dbClusterIdentifier = DEFAULT_LIMITLESS_PREFIX + "cluster-" + System.nanoTime();
      }
      if (StringUtils.isNullOrEmpty(dbShardGroupIdentifier)) {
        dbShardGroupIdentifier = DEFAULT_LIMITLESS_PREFIX + "shard-" + System.nanoTime();
      }

      AuroraClusterInfo clusterInfo;

      if (REUSE_CLUSTER && !dbClusterIdentifier.isEmpty()) {
        clusterInfo = auroraUtil.getClusterInfo(dbClusterIdentifier);
        secretsArn = System.getenv("SECRETS_ARN");
      } else {
        clusterInfo =
                auroraUtil.createLimitlessCluster(TEST_USERNAME, TEST_PASSWORD, dbClusterIdentifier, dbShardGroupIdentifier);

        // Comment out getting public IP to not add & remove from EC2 whitelist
        runnerIP = auroraUtil.getPublicIPAddress();
        auroraUtil.ec2AuthorizeIP(runnerIP);

        String secretValue = auroraUtil.createSecretValue(clusterInfo.getClusterEndpoint(), TEST_USERNAME, TEST_PASSWORD);
        secretsArn = auroraUtil.createSecrets("AWS-PGSQL-ODBC-Tests-" + clusterInfo.getClusterEndpoint(), secretValue);
      }

      dbConnStrSuffix = clusterInfo.getClusterSuffix();
      dbHostCluster = clusterInfo.getClusterEndpoint();
      dbHostClusterRo = clusterInfo.getClusterROEndpoint();

      postgresInstances = clusterInfo.getInstances();

      proxyContainers = containerHelper.createProxyContainers(network, postgresInstances, PROXIED_DOMAIN_NAME_SUFFIX);
      for (ToxiproxyContainer container : proxyContainers) {
        container.start();
      }
      postgresProxyPort = containerHelper.createAuroraInstanceProxies(postgresInstances, proxyContainers, POSTGRES_PORT);

      proxyContainers.add(containerHelper.createAndStartProxyContainer(
          network,
          "toxiproxy-instance-cluster",
          dbHostCluster + PROXIED_DOMAIN_NAME_SUFFIX,
          dbHostCluster,
          POSTGRES_PORT,
          postgresProxyPort)
      );

      proxyContainers.add(containerHelper.createAndStartProxyContainer(
          network,
          "toxiproxy-ro-instance-cluster",
          dbHostClusterRo + PROXIED_DOMAIN_NAME_SUFFIX,
          dbHostClusterRo,
          POSTGRES_PORT,
          postgresProxyPort)
      );
    }

    testContainer
      .withEnv("AWS_ACCESS_KEY_ID", ACCESS_KEY)
      .withEnv("AWS_SECRET_ACCESS_KEY", SECRET_ACCESS_KEY)
      .withEnv("AWS_SESSION_TOKEN", SESSION_TOKEN)
      .withEnv("RDS_ENDPOINT", ENDPOINT == null ? "" : ENDPOINT)
      .withEnv("RDS_REGION", REGION == null ? "us-east-2" : REGION)
      .withEnv("TOXIPROXY_CLUSTER_NETWORK_ALIAS", "toxiproxy-instance-cluster")
      .withEnv("TOXIPROXY_RO_CLUSTER_NETWORK_ALIAS", "toxiproxy-ro-instance-cluster")
      .withEnv("PROXIED_DOMAIN_NAME_SUFFIX", PROXIED_DOMAIN_NAME_SUFFIX)
      .withEnv("TEST_SERVER", dbHostCluster)
      .withEnv("TEST_RO_SERVER", dbHostClusterRo)
      .withEnv("TEST_DATABASE", DEFAULT_LIMITLESS_DATABASE)
      .withEnv("DB_CONN_STR_SUFFIX", "." + dbConnStrSuffix)
      .withEnv("PROXIED_CLUSTER_TEMPLATE", "?." + dbConnStrSuffix + PROXIED_DOMAIN_NAME_SUFFIX)
      .withEnv("SECRETS_ARN", secretsArn);

    // Add postgres instances & proxies to container env
    for (int i = 0; i < postgresInstances.size(); i++) {
      // Add instance
      testContainer.addEnv(
          "POSTGRES_INSTANCE_" + (i + 1) + "_URL",
          postgresInstances.get(i));

      // Add proxies
      testContainer.addEnv(
          "TOXIPROXY_INSTANCE_" + (i + 1) + "_NETWORK_ALIAS",
          "toxiproxy-instance-" + (i + 1));
    }
    testContainer.addEnv("POSTGRES_PROXY_PORT", Integer.toString(postgresProxyPort));
    testContainer.start();

    System.out.println("Toxyproxy Instances port: " + postgresProxyPort);
  }

  private void setupApgTestContainer(final Network network) throws InterruptedException, UnknownHostException {
    if (!StringUtils.isNullOrEmpty(ACCESS_KEY) && !StringUtils.isNullOrEmpty(SECRET_ACCESS_KEY)) {
      AuroraClusterInfo clusterInfo;

      if (REUSE_CLUSTER && !dbClusterIdentifier.isEmpty()) {
        clusterInfo = auroraUtil.getClusterInfo(dbClusterIdentifier);
        secretsArn = System.getenv("SECRETS_ARN");
      } else {
        if (StringUtils.isNullOrEmpty(dbClusterIdentifier)) {
          dbClusterIdentifier = DEFAULT_APG_PREFIX + "cluster-" + System.nanoTime();
        }

        clusterInfo = auroraUtil.createApgCluster(TEST_USERNAME, TEST_PASSWORD, dbClusterIdentifier);

        runnerIP = auroraUtil.getPublicIPAddress();
        auroraUtil.ec2AuthorizeIP(runnerIP);

        String secretValue = auroraUtil.createSecretValue(clusterInfo.getClusterEndpoint(), TEST_USERNAME, TEST_PASSWORD);
        secretsArn = auroraUtil.createSecrets("AWS-PGSQL-ODBC-Tests-" + clusterInfo.getClusterEndpoint(), secretValue);
      }

      dbConnStrSuffix = clusterInfo.getClusterSuffix();
      dbHostCluster = clusterInfo.getClusterEndpoint();
      dbHostClusterRo = clusterInfo.getClusterROEndpoint();

      postgresInstances = clusterInfo.getInstances();

      proxyContainers = containerHelper.createProxyContainers(network, postgresInstances, PROXIED_DOMAIN_NAME_SUFFIX);
      for (ToxiproxyContainer container : proxyContainers) {
        container.start();
      }
      postgresProxyPort = containerHelper.createAuroraInstanceProxies(postgresInstances, proxyContainers, POSTGRES_PORT);

      proxyContainers.add(containerHelper.createAndStartProxyContainer(
          network,
          "toxiproxy-instance-cluster",
          dbHostCluster + PROXIED_DOMAIN_NAME_SUFFIX,
          dbHostCluster,
          POSTGRES_PORT,
          postgresProxyPort)
      );

      proxyContainers.add(containerHelper.createAndStartProxyContainer(
          network,
          "toxiproxy-ro-instance-cluster",
          dbHostClusterRo + PROXIED_DOMAIN_NAME_SUFFIX,
          dbHostClusterRo,
          POSTGRES_PORT,
          postgresProxyPort)
      );
    }

    testContainer
      .withEnv("AWS_ACCESS_KEY_ID", ACCESS_KEY)
      .withEnv("AWS_SECRET_ACCESS_KEY", SECRET_ACCESS_KEY)
      .withEnv("AWS_SESSION_TOKEN", SESSION_TOKEN)
      .withEnv("RDS_ENDPOINT", ENDPOINT == null ? "" : ENDPOINT)
      .withEnv("RDS_REGION", REGION == null ? "us-east-2" : REGION)
      .withEnv("TOXIPROXY_CLUSTER_NETWORK_ALIAS", "toxiproxy-instance-cluster")
      .withEnv("TOXIPROXY_RO_CLUSTER_NETWORK_ALIAS", "toxiproxy-ro-instance-cluster")
      .withEnv("PROXIED_DOMAIN_NAME_SUFFIX", PROXIED_DOMAIN_NAME_SUFFIX)
      .withEnv("TEST_SERVER", dbHostCluster)
      .withEnv("TEST_RO_SERVER", dbHostClusterRo)
      .withEnv("TEST_DATABASE", TEST_DATABASE)
      .withEnv("DB_CONN_STR_SUFFIX", "." + dbConnStrSuffix)
      .withEnv("PROXIED_CLUSTER_TEMPLATE", "?." + dbConnStrSuffix + PROXIED_DOMAIN_NAME_SUFFIX)
      .withEnv("IAM_USER", TEST_IAM_USER)
      .withEnv("SECRETS_ARN", secretsArn);

    // Add postgres instances & proxies to container env
    for (int i = 0; i < postgresInstances.size(); i++) {
      // Add instance
      testContainer.addEnv(
          "POSTGRES_INSTANCE_" + (i + 1) + "_URL",
          postgresInstances.get(i));

      // Add proxies
      testContainer.addEnv(
          "TOXIPROXY_INSTANCE_" + (i + 1) + "_NETWORK_ALIAS",
          "toxiproxy-instance-" + (i + 1));
    }
    testContainer.addEnv("POSTGRES_PROXY_PORT", Integer.toString(postgresProxyPort));
    testContainer.start();

    System.out.println("Toxyproxy Instances port: " + postgresProxyPort);
  }

  private void setupCommunityTests(final Network network) {
    postgresContainer = ContainerHelper.createPostgresContainer(network);
    postgresContainer.start();

    testContainer
      .withEnv("PGHOST", COMMUNITY_SERVER)
      .withEnv("PGUSER", TEST_USERNAME)
      .withEnv("PGPASSWORD", TEST_PASSWORD);
    testContainer.start();

    // Build driver with unixODBC for community integration tests.
    buildDriver(false);
  }

  private void setupLimitlessIntegrationTests(final Network network) throws InterruptedException, UnknownHostException {
    setupLimitlessTestContainer(network);

    buildDriver();
    buildLimitlessTests();
  }

  private void setupIntegrationTests(final Network network) throws InterruptedException, UnknownHostException {
    setupApgTestContainer(network);

    buildDriver();
    buildIntegrationTests();
  }
}
