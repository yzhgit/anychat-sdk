Pod::Spec.new do |s|
  s.name             = 'anychat_sdk'
  s.version          = '0.1.0'
  s.summary          = 'Official AnyChat IM SDK for Flutter (iOS)'
  s.description      = <<-DESC
Official Flutter SDK for the AnyChat instant messaging system.
This package wraps the native FFI bindings for iOS.
                       DESC
  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => '../../../LICENSE' }
  s.author           = { 'AnyChat Team' => 'noreply@anychat.io' }
  s.source           = { :path => '.' }
  s.source_files     = 'Classes/**/*'
  s.platform         = :ios, '12.0'

  s.dependency 'Flutter'

  s.vendored_libraries = 'libanychat.a'

  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz -force_load "${PODS_TARGET_SRCROOT}/libanychat.a"'
  }

  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.ios.deployment_target = '12.0'

  s.prepare_command = <<-CMD
    cd ../../..
    mkdir -p build-ios
    cd build-ios
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_TOOLCHAIN_FILE=cmake/ios.toolchain.cmake \
             -DPLATFORM=OS64 \
             -DBUILD_TESTS=OFF
    cmake --build . --target anychat
    cp core/libanychat.a ../packages/flutter/ios/ || cp libanychat.a ../packages/flutter/ios/ || true
  CMD
end
