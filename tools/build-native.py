#!/usr/bin/env python3
"""Build AnyChat SDK for the native host platform (Linux / macOS / Windows).

This script uses CMake Presets for standardized cross-platform builds.

Usage:
  python3 tools/build-native.py [options]

Options:
  --preset PRESET            CMake preset name (auto-detect if not specified)
  --compiler {gcc,clang,msvc} Compiler choice (default: auto-detect)
  --config {debug,release}   Build configuration (default: release)
  -j, --jobs N               Parallel jobs passed to cmake --build (default: cpu count)
  --test                     Run CTest after a successful build
  --clean                    Clean build directory before building
  --list-presets             List all available CMake presets
"""

import argparse
import json
import multiprocessing
import os
import platform
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Optional

ROOT = Path(__file__).resolve().parent.parent
PRESETS_FILE = ROOT / "CMakePresets.json"


def load_presets() -> dict:
    """Load CMakePresets.json and return parsed data."""
    if not PRESETS_FILE.exists():
        print(f"Error: {PRESETS_FILE} not found", file=sys.stderr)
        sys.exit(1)

    with open(PRESETS_FILE, "r", encoding="utf-8") as f:
        return json.load(f)


def list_presets() -> None:
    """List all available CMake configure presets."""
    presets_data = load_presets()
    configure_presets = presets_data.get("configurePresets", [])

    print("Available CMake Configure Presets:")
    print("=" * 70)
    for preset in configure_presets:
        if preset.get("hidden", False):
            continue
        name = preset.get("name", "")
        display_name = preset.get("displayName", "")
        description = preset.get("description", "")
        print(f"  {name:30s} - {display_name}")
        if description:
            print(f"    {description}")
    print()


def detect_platform() -> str:
    """Detect the current platform."""
    system = platform.system()
    if system == "Linux":
        return "linux"
    elif system == "Darwin":
        return "macos"
    elif system == "Windows":
        return "windows"
    else:
        print(f"Error: Unsupported platform: {system}", file=sys.stderr)
        sys.exit(1)


def auto_detect_preset(compiler: Optional[str], config: str) -> str:
    """Auto-detect the appropriate CMake preset based on platform and options."""
    plat = detect_platform()
    config_lower = config.lower()

    # Platform-specific compiler defaults
    if plat == "linux":
        if compiler is None:
            # Default to GCC on Linux
            compiler = "gcc"
        if compiler not in ["gcc", "clang"]:
            print(f"Error: Invalid compiler '{compiler}' for Linux (must be gcc or clang)", file=sys.stderr)
            sys.exit(1)
        return f"linux-{compiler}-{config_lower}"

    elif plat == "macos":
        # macOS always uses clang
        return f"macos-clang-{config_lower}"

    elif plat == "windows":
        # Windows always uses MSVC
        return f"windows-msvc-{config_lower}"

    return ""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Build AnyChat SDK for the native host platform using CMake Presets.",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument(
        "--preset",
        metavar="PRESET",
        help="CMake preset name (auto-detect if not specified)",
    )
    parser.add_argument(
        "--compiler",
        choices=["gcc", "clang", "msvc"],
        help="Compiler choice (default: auto-detect based on platform)",
    )
    parser.add_argument(
        "--config",
        choices=["debug", "release"],
        default="release",
        help="Build configuration (default: release)",
    )
    parser.add_argument(
        "-j", "--jobs",
        type=int,
        default=multiprocessing.cpu_count(),
        metavar="N",
        help="Number of parallel build jobs (default: cpu count)",
    )
    parser.add_argument(
        "--test",
        action="store_true",
        help="Run CTest after a successful build",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="Clean build directory before building",
    )
    parser.add_argument(
        "--list-presets",
        action="store_true",
        help="List all available CMake presets and exit",
    )
    return parser.parse_args()


def configure(preset: str, clean: bool) -> None:
    """Configure the project using the specified CMake preset."""
    print(f"[build-native] Configuring with preset: {preset}")

    # Determine build directory from preset
    presets_data = load_presets()
    configure_presets = presets_data.get("configurePresets", [])
    preset_data = None
    for p in configure_presets:
        if p.get("name") == preset:
            preset_data = p
            break

    if not preset_data:
        print(f"Error: Preset '{preset}' not found in CMakePresets.json", file=sys.stderr)
        sys.exit(1)

    # Get binary dir (expand variables)
    binary_dir_template = preset_data.get("binaryDir", "")
    binary_dir = binary_dir_template.replace("${sourceDir}", str(ROOT))
    binary_dir = binary_dir.replace("${presetName}", preset)
    build_path = Path(binary_dir)

    # Clean if requested
    if clean and build_path.exists():
        print(f"[build-native] Cleaning build directory: {build_path}")
        shutil.rmtree(build_path)

    # Configure
    cmake_args = ["cmake", "--preset", preset]
    subprocess.run(cmake_args, cwd=ROOT, check=True)


def build(preset: str, jobs: int) -> None:
    """Build the project using the specified preset."""
    print(f"[build-native] Building with preset: {preset} (jobs={jobs})")

    cmake_args = [
        "cmake", "--build", "--preset", preset,
        "--parallel", str(jobs)
    ]
    subprocess.run(cmake_args, cwd=ROOT, check=True)


def run_tests(preset: str) -> None:
    """Run tests using the specified preset."""
    print(f"[build-native] Running tests with preset: {preset}")

    cmake_args = ["ctest", "--preset", preset]
    subprocess.run(cmake_args, cwd=ROOT, check=True)


def main() -> None:
    args = parse_args()

    if args.list_presets:
        list_presets()
        return

    # Determine the preset to use
    if args.preset:
        preset = args.preset
    else:
        preset = auto_detect_preset(args.compiler, args.config)

    print(f"[build-native] Using CMake preset: {preset}")
    print(f"[build-native] Platform: {detect_platform()}")
    print()

    # Configure
    configure(preset, args.clean)

    # Build
    build(preset, args.jobs)

    # Test
    if args.test:
        run_tests(preset)

    print()
    print(f"[build-native] Build complete!")
    print(f"[build-native] Preset: {preset}")


if __name__ == "__main__":
    try:
        main()
    except subprocess.CalledProcessError as e:
        print(f"\n[build-native] Error: Command failed with exit code {e.returncode}", file=sys.stderr)
        sys.exit(e.returncode)
    except KeyboardInterrupt:
        print("\n[build-native] Build cancelled by user", file=sys.stderr)
        sys.exit(130)
