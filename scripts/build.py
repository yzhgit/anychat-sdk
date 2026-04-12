#!/usr/bin/env python3
"""
IM Client Build Script

This script automates the build and packaging process for the IM Client application.
It supports cross-platform builds and allows selection of Qt version.

Usage:
    python build.py [options]

For help:
    python build.py --help
"""

import os
import sys
import argparse
import subprocess
import platform
import shutil
from pathlib import Path
import tempfile


def get_system_info():
    """Get system information"""
    system = platform.system().lower()
    arch = platform.machine().lower()

    # Normalize architecture names
    if arch in ['amd64', 'x86_64']:
        arch = 'x64'
    elif arch in ['i386', 'i686']:
        arch = 'x86'
    elif arch in ['arm64', 'aarch64']:
        arch = 'arm64'

    return system, arch





def get_cmake_preset(system, config):
    """Get CMake preset suitable for current system"""
    if system == 'windows':
        return f'windows-msvc-{config.lower()}'
    elif system == 'linux':
        return f'linux-gcc-{config.lower()}'
    elif system == 'darwin':
        return f'macos-clang-{config.lower()}'
    else:
        return f'linux-gcc-{config.lower()}'  # default to linux


def run_command(cmd, cwd=None, env=None):
    """Run command and print output"""
    print(f"Running command: {' '.join(cmd)}")
    try:
        result = subprocess.run(cmd, cwd=cwd, env=env, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {e}")
        return False


def find_visual_studio_vcvars():
    """Find Visual Studio vcvarsall.bat path"""
    import winreg

    # Common Visual Studio installation paths
    vs_paths = [
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files (x86)\Microsoft Visual Studio\2019\Enterprise\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat",
        r"C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat"
    ]

    for path in vs_paths:
        if Path(path).exists():
            return path

    # Try to find via registry
    try:
        # Try to find VS 2019/2022
        keys_to_try = [
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7\16.0"),
            (winreg.HKEY_LOCAL_MACHINE, r"SOFTWARE\WOW6432Node\Microsoft\VisualStudio\SxS\VS7\17.0")
        ]

        for hkey, subkey in keys_to_try:
            try:
                with winreg.OpenKey(hkey, subkey) as key:
                    i = 0
                    while True:
                        try:
                            name, value, _ = winreg.EnumValue(key, i)
                            if "2019" in name or "2022" in name:
                                vcvars_path = Path(value) / "VC" / "Auxiliary" / "Build" / "vcvars64.bat"
                                if vcvars_path.exists():
                                    return str(vcvars_path)
                            i += 1
                        except WindowsError:
                            break
            except FileNotFoundError:
                continue

    except ImportError:
        # If unable to import winreg (e.g., on non-Windows systems)
        pass

    return None


def run_windows_build(args):
    """Run build process on Windows"""
    # Find vcvarsall.bat
    vcvars_path = find_visual_studio_vcvars()
    if not vcvars_path:
        print("Error: Could not find Visual Studio installation with vcvars64.bat")
        print("Please install Visual Studio 2019 or later with C++ development tools")
        return False

    print(f"Found vcvars64.bat at: {vcvars_path}")

    # Create a temporary batch file to set environment and run build
    temp_batch = tempfile.NamedTemporaryFile(mode='w', delete=False, suffix='.bat')

    # Write batch content
    temp_batch.write(f'call "{vcvars_path}"\n')
    temp_batch.write('@echo on\n')

    # Get preset
    preset = get_cmake_preset('windows', args.config)

    # Check build directory
    build_dir = Path(args.build_dir).resolve()
    if build_dir.exists():
        temp_batch.write(f'if exist "{build_dir}\\CMakeCache.txt" del "{build_dir}\\CMakeCache.txt"\n')

    temp_batch.write(f'mkdir "{build_dir}" 2>nul\n')

    # Build CMake configuration options
    cmake_options = []
    if hasattr(args, 'build_tests') and args.build_tests is not None:
        cmake_options.append(f'-DBUILD_TESTS={args.build_tests}')
    if hasattr(args, 'build_android') and args.build_android:
        cmake_options.append('-DBUILD_ANDROID_BINDING=ON')
    if hasattr(args, 'build_ios') and args.build_ios:
        cmake_options.append('-DBUILD_IOS_BINDING=ON')
    if hasattr(args, 'build_web') and args.build_web:
        cmake_options.append('-DBUILD_WEB_BINDING=ON')

    cmake_options_str = ' '.join(cmake_options)

    # Add CMake configure command
    temp_batch.write(f'echo Configuring with CMake preset: {preset}...\n')
    temp_batch.write(f'cmake --preset={preset} {cmake_options_str} "{Path(".").resolve()}"\n')

    # Build command
    temp_batch.write(f'cmake --build "{build_dir}" --config {args.config}\n')

    temp_batch.close()

    # Run batch file
    try:
        result = subprocess.run([temp_batch.name], shell=True, check=True)
        print("Build completed successfully!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"Build failed: {e}")
        return False
    finally:
        # Clean up temp files
        try:
            os.remove(temp_batch.name)
        except:
            pass


def run_unix_build(args):
    """Run build process on Unix systems"""
    build_env = os.environ.copy()

    # Check build directory
    build_dir = Path(args.build_dir).resolve()
    if build_dir.exists():
        cache_file = build_dir / "CMakeCache.txt"
        if cache_file.exists():
            print("Removing existing CMakeCache.txt to avoid compiler detection issues...")
            cache_file.unlink()

    build_dir.mkdir(parents=True, exist_ok=True)

    # Get preset
    system, _ = get_system_info()
    preset = get_cmake_preset(system, args.config)

    # Configure using CMake preset
    print(f"Configuring with CMake preset: {preset}...")
    source_path = str(Path('.').resolve())

    # Build CMake configuration options
    cmake_configure_cmd = [
        'cmake',
        f'--preset={preset}',
        source_path
    ]

    # Add optional build options
    if hasattr(args, 'build_tests') and args.build_tests is not None:
        cmake_configure_cmd.insert(2, f'-DBUILD_TESTS={args.build_tests}')
    if hasattr(args, 'build_android') and args.build_android:
        cmake_configure_cmd.insert(2, '-DBUILD_ANDROID_BINDING=ON')
    if hasattr(args, 'build_ios') and args.build_ios:
        cmake_configure_cmd.insert(2, '-DBUILD_IOS_BINDING=ON')
    if hasattr(args, 'build_web') and args.build_web:
        cmake_configure_cmd.insert(2, '-DBUILD_WEB_BINDING=ON')

    if not run_command(cmake_configure_cmd, cwd=str(build_dir), env=build_env):
        print("CMake configuration with preset failed")
        return False

    # Build project
    build_cmd = ['cmake', '--build', str(build_dir), '--config', args.config]
    if not run_command(build_cmd, env=build_env):
        print("Build failed")
        return False

    print("Build completed successfully!")
    return True


def run_build_with_environment(args):
    """Run build process in configured environment"""
    system, _ = get_system_info()

    if system == 'windows':
        return run_windows_build(args)
    else:
        return run_unix_build(args)


def build_project(args):
    """Build project"""
    print("Starting build process...")

    # Check CMake
    if not shutil.which('cmake'):
        print("Error: cmake not found in PATH")
        return False

    # Run build process
    return run_build_with_environment(args)


def package_project(args):
    """Package project"""
    print("Starting packaging process...")

    build_dir = Path(args.build_dir).resolve()

    # Run CPack for packaging
    package_cmd = ['cmake', '--build', str(build_dir), '--target', 'package']

    if not run_command(package_cmd):
        print("Packaging failed")
        return False

    print("Packaging completed successfully!")
    return True


def main():
    parser = argparse.ArgumentParser(
        description='Build and package the AnyChat SDK',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s --build                    # Build the project
  %(prog)s --package                  # Package the project (requires build first)
  %(prog)s --all                      # Build and package the project
  %(prog)s --config Debug             # Build debug version
  %(prog)s --build-dir ./mybuild      # Use custom build directory
  %(prog)s --build-tests ON           # Build with unit tests
        """
    )

    # Add build/package options
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--build', action='store_true', help='Build the project')
    group.add_argument('--package', action='store_true', help='Package the project')
    group.add_argument('--all', action='store_true', help='Build and package the project')

    # Add build configuration options
    parser.add_argument('--config', choices=['Debug', 'Release'], default='Release',
                       help='Build configuration (default: Release)')

    # Add build directory option
    parser.add_argument('--build-dir', default='./build',
                       help='Build directory (default: ./build)')

    # Add build options (correspond to actual options in CMakeLists.txt)
    parser.add_argument('--build-tests', choices=['ON', 'OFF'], default=None,
                       help='Build unit tests (default: ON)')
    parser.add_argument('--build-android', action='store_true',
                       help='Build Android JNI binding')
    parser.add_argument('--build-ios', action='store_true',
                       help='Build iOS binding')
    parser.add_argument('--build-web', action='store_true',
                       help='Build WebAssembly binding')

    # Parse arguments
    args = parser.parse_args()

    # If --all, first build then package
    if args.all:
        if not build_project(args):
            sys.exit(1)
        if not package_project(args):
            sys.exit(1)
    elif args.build:
        if not build_project(args):
            sys.exit(1)
    elif args.package:
        if not package_project(args):
            sys.exit(1)


if __name__ == '__main__':
    main()