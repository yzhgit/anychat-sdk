Pod::Spec.new do |s|
  s.name             = 'anychat_flutter'
  s.version          = '0.1.0'
  s.summary          = 'AnyChat IM SDK for Flutter (macOS)'
  s.description      = <<-DESC
AnyChat IM SDK native macOS bindings for Flutter.
                       DESC
  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => '../../../LICENSE' }
  s.author           = { 'AnyChat Team' => 'noreply@anychat.io' }
  s.source           = { :path => '.' }
  s.source_files     = '**/*.{h,m,mm,swift}'
  s.platform         = :osx, '10.14'

  s.vendored_libraries = 'libanychat_c.a', 'libanychat_core.a'

  s.xcconfig = {
    'HEADER_SEARCH_PATHS' => '"${PODS_TARGET_SRCROOT}/../../../core/include"',
    'OTHER_LDFLAGS' => '-lc++ -lz'
  }

  s.frameworks = 'Foundation', 'Security'
  s.libraries = 'c++', 'z', 'resolv'

  s.dependency 'FlutterMacOS'
  s.osx.deployment_target = '10.14'

  s.prepare_command = <<-CMD
    cd ../../../core
    mkdir -p build-macos
    cd build-macos
    cmake .. -DCMAKE_BUILD_TYPE=Release \
             -DCMAKE_OSX_DEPLOYMENT_TARGET=10.14 \
             -DBUILD_TESTS=OFF
    cmake --build . --target anychat_c anychat_core
    cp bin/libanychat_c.a ../../../bindings/flutter/macos/
    cp bin/libanychat_core.a ../../../bindings/flutter/macos/
  CMD
end
