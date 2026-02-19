Pod::Spec.new do |s|
  s.name             = 'AnyChatSDK'
  s.version          = '1.0.0'
  s.summary          = 'AnyChat IM SDK for iOS and macOS'
  s.description      = <<-DESC
    Complete Swift wrapper for the AnyChat IM system, providing messaging,
    voice/video calls, group management, and real-time communication features.
  DESC

  s.homepage         = 'https://github.com/yzhgit/anychat-sdk'
  s.license          = { :type => 'MIT', :file => 'LICENSE' }
  s.author           = { 'AnyChat Team' => 'dev@anychat.io' }
  s.source           = { :git => 'https://github.com/yzhgit/anychat-sdk.git', :tag => s.version.to_s }

  s.ios.deployment_target = '13.0'
  s.osx.deployment_target = '10.15'

  s.swift_version = '5.9'

  s.source_files = 'Sources/AnyChatSDK/**/*.swift'
  s.public_header_files = 'Sources/AnyChatSDK/AnyChatSDK.h'

  s.module_map = 'Sources/AnyChatSDK/module.modulemap'

  s.dependency 'anychat_c', '~> 1.0'

  s.frameworks = 'Foundation'
  s.ios.frameworks = 'UIKit'
  s.osx.frameworks = 'AppKit'

  s.pod_target_xcconfig = {
    'DEFINES_MODULE' => 'YES',
    'SWIFT_VERSION' => '5.9'
  }
end
