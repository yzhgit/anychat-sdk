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

  s.vendored_libraries = 'libanychat.a'

  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz -force_load "${PODS_TARGET_SRCROOT}/libanychat.a"'
  }

  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.osx.deployment_target = '10.14'

  s.prepare_command = <<-CMD
    cd ../../..
    mkdir -p build-macos
    cd build-macos
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
             -DBUILD_TESTS=OFF
    cmake --build . --target anychat
    cp core/libanychat.a ../packages/flutter/macos/ || cp libanychat.a ../packages/flutter/macos/ || true
  CMD
end
