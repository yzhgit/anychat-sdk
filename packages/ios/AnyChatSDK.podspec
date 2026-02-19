Pod::Spec.new do |s|
  s.name         = 'AnyChatSDK'
  s.version      = '0.1.0'
  s.summary      = 'iOS SDK for the AnyChat instant messaging system.'
  s.homepage     = 'https://github.com/yzhgit/anychat-sdk'
  s.license      = { :type => 'MIT', :file => 'LICENSE' }
  s.author       = { 'AnyChat' => 'sdk@anychat.io' }
  s.platform     = :ios, '14.0'
  s.source       = { :git => 'https://github.com/yzhgit/anychat-sdk.git', :tag => s.version.to_s }

  s.source_files        = 'bindings/ios/**/*.{h,m,mm,swift}',
                          'core/include/**/*.h'
  s.public_header_files = 'core/include/**/*.h'

  s.pod_target_xcconfig = {
    'SWIFT_VERSION'          => '5.0',
    'CLANG_CXX_LANGUAGE_STANDARD' => 'c++17',
  }
end
