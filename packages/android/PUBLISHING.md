# Publishing to Maven Central

This guide explains how to publish the AnyChat Android SDK to Maven Central.

## Prerequisites

1. **Sonatype Account**
   - Sign up at https://s01.oss.sonatype.org/
   - Create a JIRA ticket to claim the `io.github.yzhgit` groupId
   - Wait for approval (usually 1-2 business days)

2. **GPG Key for Signing**
   ```bash
   # Generate a new key
   gpg --gen-key

   # List keys
   gpg --list-keys

   # Export public key to keyserver
   gpg --keyserver keyserver.ubuntu.com --send-keys YOUR_KEY_ID

   # Export secret key
   gpg --export-secret-keys YOUR_KEY_ID > ~/.gnupg/secring.gpg
   ```

3. **Gradle Configuration**

   Add to `~/.gradle/gradle.properties` (NOT the project's gradle.properties):
   ```properties
   OSSRH_USERNAME=your_sonatype_username
   OSSRH_PASSWORD=your_sonatype_password

   signing.keyId=YOUR_GPG_KEY_ID (last 8 digits)
   signing.password=YOUR_GPG_PASSPHRASE
   signing.secretKeyRingFile=/path/to/.gnupg/secring.gpg
   ```

## Publishing Steps

### 1. Update Version

Edit `gradle.properties`:
```properties
VERSION_NAME=0.1.0
```

### 2. Enable Signing

Uncomment the signing block in `build.gradle.kts`:
```kotlin
signing {
    sign(publishing.publications["release"])
}
```

### 3. Build and Publish

```bash
cd packages/android

# Clean build
./gradlew clean

# Build the library
./gradlew assembleRelease

# Generate artifacts (AAR, sources, javadoc)
./gradlew publishToMavenLocal  # Test locally first

# Publish to Sonatype staging
./gradlew publish

# Or publish and release in one step
./gradlew publishReleasePublicationToSonatypeRepository
```

### 4. Release on Sonatype

1. Login to https://s01.oss.sonatype.org/
2. Click "Staging Repositories" in the left sidebar
3. Find your repository (e.g., `iogithub-1234`)
4. Click "Close" to validate the artifacts
5. Wait for validation to complete
6. Click "Release" to publish to Maven Central

### 5. Verify Publication

After 10-30 minutes, verify on Maven Central:
- https://central.sonatype.com/artifact/io.github.yzhgit/anychat-sdk-android

After 2 hours, it will appear in search:
- https://search.maven.org/artifact/io.github.yzhgit/anychat-sdk-android

## Automated Publishing with GitHub Actions

Create `.github/workflows/publish.yml`:

```yaml
name: Publish to Maven Central

on:
  release:
    types: [created]

jobs:
  publish:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
        with:
          submodules: recursive

      - name: Set up JDK 17
        uses: actions/setup-java@v4
        with:
          java-version: '17'
          distribution: 'temurin'

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3

      - name: Install NDK
        run: |
          echo "y" | sdkmanager "ndk;25.2.9519653"

      - name: Decode GPG key
        run: |
          echo "${{ secrets.GPG_SECRET_KEY }}" | base64 -d > ~/.gnupg/secring.gpg

      - name: Publish to Maven Central
        env:
          OSSRH_USERNAME: ${{ secrets.OSSRH_USERNAME }}
          OSSRH_PASSWORD: ${{ secrets.OSSRH_PASSWORD }}
          signing.keyId: ${{ secrets.GPG_KEY_ID }}
          signing.password: ${{ secrets.GPG_PASSWORD }}
          signing.secretKeyRingFile: ~/.gnupg/secring.gpg
        run: |
          cd packages/android
          ./gradlew publish --no-daemon
```

### Required Secrets

Add these to GitHub repository secrets (Settings -> Secrets and variables -> Actions):

- `OSSRH_USERNAME`: Sonatype username
- `OSSRH_PASSWORD`: Sonatype password
- `GPG_KEY_ID`: Last 8 digits of GPG key ID
- `GPG_PASSWORD`: GPG key passphrase
- `GPG_SECRET_KEY`: Base64-encoded secret key
  ```bash
  gpg --export-secret-keys YOUR_KEY_ID | base64
  ```

## Snapshot Releases

For snapshot releases, append `-SNAPSHOT` to the version:

```properties
VERSION_NAME=0.2.0-SNAPSHOT
```

Snapshots are automatically published to the snapshots repository and are available immediately (no close/release required).

Users can access snapshots by adding the repository:

```kotlin
repositories {
    maven {
        url = uri("https://s01.oss.sonatype.org/content/repositories/snapshots/")
    }
}
```

## Troubleshooting

### "401 Unauthorized" Error

- Verify OSSRH credentials in `~/.gradle/gradle.properties`
- Ensure you're using the correct Sonatype instance (s01 vs oss)

### Signing Failed

- Check GPG key ID (last 8 digits only)
- Verify passphrase is correct
- Ensure `secring.gpg` path is correct

### POM Validation Failed

- Ensure all required POM fields are filled (name, description, url, licenses, developers, scm)
- Check that URLs are valid and accessible

### Close/Release Buttons Disabled

- Wait for validation to complete (check "Activity" tab)
- Fix any validation errors shown
- Ensure all required files are present (AAR, POM, sources JAR, javadoc JAR, .asc signatures)

## Best Practices

1. **Always test locally first**
   ```bash
   ./gradlew publishToMavenLocal
   ```
   Then test in another project:
   ```kotlin
   repositories {
       mavenLocal()
   }
   ```

2. **Use semantic versioning**
   - Major.Minor.Patch (e.g., 1.2.3)
   - Breaking changes = bump major
   - New features = bump minor
   - Bug fixes = bump patch

3. **Update CHANGELOG.md** before releasing

4. **Create Git tags** for releases
   ```bash
   git tag -a v0.1.0 -m "Release version 0.1.0"
   git push origin v0.1.0
   ```

5. **Announce releases**
   - GitHub Releases page
   - Update documentation
   - Notify users

## Resources

- [Maven Central Publishing Guide](https://central.sonatype.org/publish/publish-guide/)
- [GPG Signing Guide](https://central.sonatype.org/publish/requirements/gpg/)
- [Gradle Maven Publish Plugin](https://docs.gradle.org/current/userguide/publishing_maven.html)
