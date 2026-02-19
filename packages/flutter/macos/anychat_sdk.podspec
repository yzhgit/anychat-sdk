Pod::Spec.new do |s|
  s.name             = 'anychat_sdk'
  s.version          = '0.1.0'
  s.summary          = 'Official AnyChat IM SDK for Flutter (macOS)'
  s.description      = <<-DESC
Official Flutter SDK for the AnyChat instant messaging system.
This package wraps the native FFI bindings for macOS.
                       DESC
  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => '../LICENSE' }
  s.author           = { 'AnyChat Team' => 'noreply@anychat.io' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.platform         = :osx, '10.14'

  # Delegate to the actual bindings podspec
  s.dependency 'FlutterMacOS'

  # Reference the bindings podspec
  # The actual native library building is handled by ../../bindings/flutter/macos/anychat_flutter.podspec
  s.vendored_frameworks = '../../bindings/flutter/macos/Frameworks/*.framework'

  # Include paths for C headers
  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../bindings/flutter/macos" "${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz'
  }

  # Frameworks
  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.osx.deployment_target = '10.14'

  # Pre-build: Build the native library via the bindings
  s.prepare_command = <<-CMD
    cd ../../bindings/flutter/macos
    # The anychat_flutter.podspec will handle building the native library
    # We just need to ensure it's built before our pod is used
  CMD
end
