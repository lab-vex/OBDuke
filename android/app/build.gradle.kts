plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
}

android {
    namespace = "au.obduke.app"
    compileSdk = 34

    defaultConfig {
        applicationId = "au.obduke.app"
        minSdk = 26
        targetSdk = 34
        // Only versionName needs bumping: versionCode is worked out from it (1.2.1 -> 10201),
        // so every release can install over the last one.
        versionName = "1.2.1"
        versionCode = versionName!!.split(".").map { it.toInt() }.let { (a, b, c) -> a * 10000 + b * 100 + c }
    }

    // One permanent key for every build, so in-app updates install over the top.
    // Taken from the OBDUKE_KEYSTORE env var (CI secret) if set, else android/signing/.
    // See android/signing/README.md. Without either, falls back to the debug key.
    val repoKs = rootProject.file("signing/obduke-release.jks")
    val ksProps = java.util.Properties().apply {
        rootProject.file("signing/keystore.properties").takeIf { it.exists() }?.inputStream()?.use { load(it) }
    }
    // CI passes "" for a secret that isn't set, so treat blank env vars as missing
    fun env(name: String) = System.getenv(name)?.takeIf { it.isNotBlank() }
    val ks = env("OBDUKE_KEYSTORE")?.let { file(it) }?.takeIf { it.exists() }
        ?: repoKs.takeIf { it.exists() }
    signingConfigs {
        if (ks != null) create("obduke") {
            storeFile = ks
            storePassword = env("OBDUKE_KEYSTORE_PASSWORD") ?: ksProps.getProperty("storePassword")
            keyAlias = ksProps.getProperty("keyAlias") ?: "obduke"
            keyPassword = env("OBDUKE_KEYSTORE_PASSWORD") ?: ksProps.getProperty("keyPassword")
        }
    }
    val signer = if (ks != null) signingConfigs.getByName("obduke") else signingConfigs.getByName("debug")

    buildTypes {
        debug { signingConfig = signer }
        release {
            isMinifyEnabled = false
            signingConfig = signer
        }
    }
    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
    buildFeatures { compose = true; buildConfig = true }
}

dependencies {
    val bom = platform("androidx.compose:compose-bom:2024.09.02")
    implementation(bom)
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.activity:activity-compose:1.9.2")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.6")
    implementation("androidx.lifecycle:lifecycle-viewmodel-compose:2.8.6")
    implementation("androidx.lifecycle:lifecycle-viewmodel-ktx:2.8.6")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.material:material-icons-extended")
}
