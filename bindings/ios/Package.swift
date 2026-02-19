// swift-tools-version:5.9
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "AnyChatSDK",
    platforms: [
        .iOS(.v13),
        .macOS(.v10_15)
    ],
    products: [
        .library(
            name: "AnyChatSDK",
            targets: ["AnyChatSDK"]
        ),
    ],
    dependencies: [],
    targets: [
        .target(
            name: "AnyChatCAPI",
            dependencies: [],
            path: "../../core",
            exclude: [
                "src",
                "tests",
                "examples"
            ],
            sources: [],
            publicHeadersPath: "include/anychat_c",
            cSettings: [
                .headerSearchPath("include")
            ],
            linkerSettings: [
                .linkedLibrary("anychat_c", .when(platforms: [.iOS, .macOS]))
            ]
        ),
        .target(
            name: "AnyChatSDK",
            dependencies: ["AnyChatCAPI"],
            path: "Sources/AnyChatSDK",
            exclude: [],
            swiftSettings: [
                .enableExperimentalFeature("StrictConcurrency")
            ]
        )
    ],
    cLanguageStandard: .c11,
    cxxLanguageStandard: .cxx17
)
