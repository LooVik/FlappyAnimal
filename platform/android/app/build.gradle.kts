// Overridable from gradle.properties or the command line, e.g.
//   ./gradlew assembleDebug -PARCH_ABI=armeabi-v7a
val NDK_VERSION by extra(project.properties["NDK_VERSION"] as? String ?: "26.1.10909125")
val ARCH_ABI    by extra(project.properties["ARCH_ABI"]    as? String ?: "arm64-v8a")
val MIN_SDK     by extra((project.properties["MIN_SDK"]    as? String ?: "21").toInt())

// AGP 9.0.1 supports up to API 36.1, so compileSdk cannot be 37 even though
// that platform is installed. Bump this only when AGP itself supports it.
val TARGET_SDK  by extra((project.properties["TARGET_SDK"] as? String ?: "36").toInt())
val STL_TYPE    by extra(project.properties["STL_TYPE"]    as? String ?: "c++_shared")

plugins {
    id("com.android.application")
}

android {
    namespace  = "com.loovik.flappyanimals"
    ndkVersion = NDK_VERSION
    compileSdk = TARGET_SDK

    defaultConfig {
        applicationId = "com.loovik.flappyanimals"
        minSdk        = MIN_SDK
        targetSdk     = TARGET_SDK
        versionCode   = 1
        versionName   = "0.1"

        ndk {
            // One ABI while developing: arm64-v8a covers essentially every
            // phone made since 2017 and halves the build time. Add
            // armeabi-v7a later if older devices matter.
            abiFilters.add(ARCH_ABI)
        }

        externalNativeBuild {
            cmake {
                arguments.add("-DANDROID_STL=${STL_TYPE}")
                arguments.add("-DSFML_STATIC_LIBRARIES=ON")
            }
        }
    }

    buildTypes {
        release {
            // The manifest sets hasCode="false" — this app is pure native code
            // with no Java or Kotlin at all, so R8 has nothing to shrink or
            // obfuscate. Enabling it would only add a build step.
            isMinifyEnabled = false
        }
    }

    externalNativeBuild {
        cmake {
            path("src/main/jni/CMakeLists.txt")
        }
    }

    sourceSets {
        getByName("main") {
            // Not src/main/assets. The generated folder below is the only
            // source of game assets for the APK — see syncGameAssets.
            //
            // A plain path, not layout.buildDirectory.dir(...): AGP 9 refuses
            // lazy Providers here with "You cannot add Provider instances to
            // the Android SourceSet API". Relative paths resolve against this
            // module's directory, so this is app/build/generated/gameAssets.
            assets.srcDirs("build/generated/gameAssets")
        }
    }
}

// Keep the APK's assets generated, never hand-copied.
//
// The game's art, audio and font live once, at the repository root, and the
// desktop build copies them next to the .exe from CMake. Android needs them
// packaged inside the APK instead. The obvious fix — a second copy checked in
// under src/main/assets — is a trap: it rots the first time a file is added on
// one side only, and it fails silently, as a missing texture at runtime on one
// platform.
//
// Sync (not Copy) also deletes files from the destination that no longer exist
// in the source, so a renamed asset does not leave its old self behind.
//
// The nested "assets" folder is deliberate: everything in this directory
// becomes the APK's asset root, so the game's assets must sit one level down
// for art.load("assets") to find them — the same relative path the desktop
// build uses. That one line is why src/main.cpp needs no Android special case.
val syncGameAssets = tasks.register<Sync>("syncGameAssets") {
    description = "Copies the repository's assets/ folder into the APK."
    from(rootProject.file("../../assets"))
    into(layout.buildDirectory.dir("generated/gameAssets/assets"))
}

// AGP names this task per variant (mergeDebugAssets, mergeReleaseAssets, ...),
// so match by name rather than hard-coding one variant.
tasks.matching { it.name.startsWith("merge") && it.name.endsWith("Assets") }.configureEach {
    dependsOn(syncGameAssets)
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar"))))
}
