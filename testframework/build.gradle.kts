plugins {
    java
}

group = "software.aws.rds"
version = "1.0-SNAPSHOT"

repositories {
    mavenCentral()
}

dependencies {
    testImplementation("software.amazon.awssdk:rds:2.29.17")
    testImplementation("software.amazon.awssdk:ec2:2.29.17")
    testImplementation("software.amazon.awssdk:secretsmanager:2.29.17")
    testImplementation("org.junit.jupiter:junit-jupiter-api:5.11.3")
    testImplementation("org.testcontainers:toxiproxy:1.20.3")
    testImplementation("org.testcontainers:postgresql:1.20.3")
    testImplementation("org.json:json:20240303")

    testRuntimeOnly("org.junit.jupiter:junit-jupiter-engine")
}

tasks.getByName<Test>("test") {
    useJUnitPlatform()
}

tasks.register<Test>("test-limitless") {
    filter.includeTestsMatching("host.IntegrationContainerTest.testRunLimitlessTestInContainer")
}

tasks.register<Test>("test-community") {
    filter.includeTestsMatching("host.IntegrationContainerTest.testRunCommunityTestInContainer")
}

tasks.register<Test>("test-performance") {
    filter.includeTestsMatching("host.IntegrationContainerTest.testRunPerformanceTestInContainer")
}

tasks.register<Test>("test-integration") {
    filter.includeTestsMatching("host.IntegrationContainerTest.testRunIntegrationTestInContainer")
}

tasks.withType<Test> {
    useJUnitPlatform()
    group = "verification"
    this.testLogging {
        this.showStandardStreams = true
    }
}
