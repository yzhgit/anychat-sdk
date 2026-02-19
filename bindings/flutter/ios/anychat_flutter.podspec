Pod::Spec.new do |s|
  s.name             = 'anychat_flutter'
  s.version          = '0.1.0'
  s.summary          = 'AnyChat IM SDK for Flutter (iOS)'
  s.description      = <<-DESC
AnyChat IM SDK native iOS bindings for Flutter.
                       DESC
  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => '../../../LICENSE' }
  s.author           = { 'AnyChat Team' => 'noreply@anychat.io' }
  s.source           = { :path => '.' }
  s.source_files     = '**/*.{h,m,mm,swift}'
  s.platform         = :ios, '12.0'

  # Link against the C API static library
  s.vendored_libraries = 'libanychat_c.a', 'libanychat_core.a'

  # Include paths for C headers
  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz'
  }

  # Frameworks
  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.dependency 'Flutter'
  s.ios.deployment_target = '12.0'

  # Pre-build script: build anychat_c and anychat_core
  s.prepare_command = <<-CMD
    cd ../../../core
    mkdir -p build-ios
    cd build-ios
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_TOOLCHAIN_FILE=../../cmake/ios.toolchain.cmake \
             -DPLATFORM=OS64 \
             -DBUILD_TESTS=OFF
    cmake --build . --target anychat_c anychat_core
    cp bin/libanychat_c.a ../../../bindings/flutter/ios/
    cp bin/libanychat_core.a ../../../bindings/flutter/ios/
  CMD
end
