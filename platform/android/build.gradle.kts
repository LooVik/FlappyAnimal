// Top-level build file.
//
// SFML's example template shipped Gradle 7.6.2 / AGP 7.4.1, which predate the
// JDK bundled with current Android Studio (25). Gradle only gained JDK 25
// support in 9.1.0, and AGP 9.0.1 requires exactly that — so both move together.
buildscript {
    repositories {
        google()
        mavenCentral()
    }
    dependencies {
        classpath("com.android.tools.build:gradle:9.0.1")
    }
}

allprojects {
    repositories {
        google()
        mavenCentral()
    }
}

tasks.withType<Wrapper> {
    gradleVersion = "9.1.0"
    distributionType = Wrapper.DistributionType.BIN
}

tasks.register("clean", Delete::class) {
    delete(rootProject.layout.buildDirectory)
}
