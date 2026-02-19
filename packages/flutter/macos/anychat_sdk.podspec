Pod::Spec.new do |s|
  s.name             = 'anychat_sdk'
  s.version          = '0.1.0'
  s.summary          = 'Official AnyChat IM SDK for Flutter (macOS)'
  s.description      = <<-DESC
Official Flutter SDK for the AnyChat instant messaging system.
This package wraps the native FFI bindings for macOS.
                       DESC
  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => '../../../LICENSE' }
  s.author           = { 'AnyChat Team' => 'noreply@anychat.io' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.platform         = :osx, '10.14'

  s.dependency 'FlutterMacOS'

  s.vendored_libraries = 'libanychat_c.a', 'libanychat_core.a'

  # Include paths for C headers
  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz'
  }

  # Frameworks
  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.osx.deployment_target = '10.14'

  # Pre-build: Build the native library
  s.prepare_command = <<-CMD
    cd ../../../core
    mkdir -p build-macos
    cd build-macos
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
             -DBUILD_TESTS=OFF
    cmake --build . --target anychat_c anychat_core
    cp bin/libanychat_c.a ../../packages/flutter/macos/
    cp bin/libanychat_core.a ../../packages/flutter/macos/
  CMD
end
