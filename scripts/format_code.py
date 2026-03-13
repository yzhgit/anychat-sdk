#!/usr/bin/env python3
"""
Code Formatter Script for IM Project

This script automatically formats all C++ source files using clang-format.
It recursively processes all .h, .hpp, .cpp, and .cc files in the project.

Usage:
    python scripts/format_code.py              # Format all files
    python scripts/format_code.py --check      # Check formatting without modifying
    python scripts/format_code.py src/sdk      # Format specific directory
    python scripts/format_code.py file.cpp     # Format specific file
"""

import argparse
import os
import subprocess
import sys
from pathlib import Path
from typing import List, Tuple

# ANSI color codes for terminal output
class Colors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

def find_project_root() -> Path:
    """
    Find the project root directory by looking for .clang-format file.

    Returns:
        Path to project root
    """
    # Start from current working directory
    current = Path.cwd()

    # Search upwards for .clang-format file
    for parent in [current] + list(current.parents):
        if (parent / '.clang-format').exists():
            return parent

    # If .clang-format not found, try script location
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    if (project_root / '.clang-format').exists():
        return project_root

    # Fall back to current directory
    return current

def find_clang_format() -> str:
    """Find clang-format executable in system PATH."""
    # Try common names
    for name in ['clang-format', 'clang-format-17', 'clang-format-16', 'clang-format-15']:
        try:
            result = subprocess.run([name, '--version'],
                                    capture_output=True,
                                    text=True,
                                    check=True)
            print(f"{Colors.OKGREEN}Found: {result.stdout.strip()}{Colors.ENDC}")
            return name
        except (subprocess.CalledProcessError, FileNotFoundError):
            continue

    print(f"{Colors.FAIL}Error: clang-format not found in PATH{Colors.ENDC}")
    print(f"{Colors.WARNING}Please install clang-format or add it to your PATH{Colors.ENDC}")
    sys.exit(1)

def get_source_files(paths: List[str]) -> List[Path]:
    """
    Get all C++ source files from the given paths.

    Args:
        paths: List of file or directory paths

    Returns:
        List of Path objects for all C++ source files (absolute paths)
    """
    extensions = {'.h', '.hpp', '.cpp', '.cc'}
    source_files = []

    for path_str in paths:
        # Convert to absolute path
        path = Path(path_str).resolve()

        if not path.exists():
            print(f"{Colors.WARNING}Warning: Path does not exist: {path}{Colors.ENDC}")
            continue

        if path.is_file():
            if path.suffix in extensions:
                source_files.append(path)
            else:
                print(f"{Colors.WARNING}Warning: Skipping non-C++ file: {path}{Colors.ENDC}")
        elif path.is_dir():
            # Recursively find all source files
            for ext in extensions:
                source_files.extend(path.rglob(f'*{ext}'))

    # Exclude build directories and third-party code
    excluded_dirs = {'build', 'cmake-build-debug', 'cmake-build-release',
                     'thirdparty', 'third_party', 'external', '.git'}

    filtered_files = []
    for file in source_files:
        # Convert to absolute path and check if any parent directory is in excluded list
        abs_file = file.resolve()
        if not any(excluded in abs_file.parts for excluded in excluded_dirs):
            filtered_files.append(abs_file)

    return sorted(set(filtered_files))

def format_file(clang_format: str, file_path: Path, check_only: bool = False) -> Tuple[bool, str]:
    """
    Format a single file using clang-format.

    Args:
        clang_format: Path to clang-format executable
        file_path: Path to the file to format
        check_only: If True, only check formatting without modifying

    Returns:
        Tuple of (success, message)
    """
    try:
        if check_only:
            # Check if file needs formatting
            result = subprocess.run([clang_format, '--dry-run', '-Werror', str(file_path)],
                                    capture_output=True,
                                    text=True)
            if result.returncode == 0:
                return True, "OK"
            else:
                return False, "Needs formatting"
        else:
            # Format the file in-place
            result = subprocess.run([clang_format, '-i', str(file_path)],
                                    capture_output=True,
                                    text=True,
                                    check=True)
            return True, "Formatted"
    except subprocess.CalledProcessError as e:
        return False, f"Error: {e.stderr}"

def main():
    parser = argparse.ArgumentParser(
        description='Format C++ source files using clang-format',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Format all source files in the project
  python scripts/format_code.py

  # Check formatting without modifying files
  python scripts/format_code.py --check

  # Format specific directory
  python scripts/format_code.py src/sdk

  # Format specific files
  python scripts/format_code.py src/sdk/sdk_types.h src/sdk/sdk_types.cpp

  # Verbose output
  python scripts/format_code.py -v
        """
    )

    parser.add_argument('paths', nargs='*', default=['.'],
                        help='Files or directories to format (default: current directory)')
    parser.add_argument('--check', action='store_true',
                        help='Check formatting without modifying files')
    parser.add_argument('-v', '--verbose', action='store_true',
                        help='Verbose output')
    parser.add_argument('--project-root', type=str, default=None,
                        help='Project root directory (default: auto-detect from .clang-format)')

    args = parser.parse_args()

    # Find project root
    if args.project_root:
        project_root = Path(args.project_root).resolve()
        if not project_root.is_dir():
            print(f"{Colors.FAIL}Error: Project root does not exist: {project_root}{Colors.ENDC}")
            return 1
    else:
        project_root = find_project_root()

    # Change to project root
    os.chdir(project_root)

    # Verify .clang-format exists
    if not (project_root / '.clang-format').exists():
        print(f"{Colors.WARNING}Warning: .clang-format not found in project root{Colors.ENDC}")
        print(f"{Colors.WARNING}         Formatting will use clang-format defaults{Colors.ENDC}")
        print()

    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}C++ Code Formatter{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"Project root: {project_root}")
    print()

    # Find clang-format
    clang_format = find_clang_format()
    print()

    # Get source files
    print(f"{Colors.OKBLUE}Searching for C++ source files...{Colors.ENDC}")
    source_files = get_source_files(args.paths)

    if not source_files:
        print(f"{Colors.WARNING}No C++ source files found{Colors.ENDC}")
        return 0

    print(f"{Colors.OKGREEN}Found {len(source_files)} file(s){Colors.ENDC}")
    print()

    # Format files
    mode = "Checking" if args.check else "Formatting"
    print(f"{Colors.OKBLUE}{mode} files...{Colors.ENDC}")
    print()

    success_count = 0
    failure_count = 0
    needs_formatting = []

    for i, file_path in enumerate(source_files, 1):
        # Try to get relative path, fallback to absolute if not possible
        try:
            relative_path = file_path.relative_to(project_root)
        except ValueError:
            # File is outside project root, use absolute path
            relative_path = file_path

        success, message = format_file(clang_format, file_path, args.check)

        if success:
            success_count += 1
            if args.verbose or (args.check and message == "Needs formatting"):
                status_color = Colors.OKGREEN if message == "OK" else Colors.WARNING
                print(f"[{i:3d}/{len(source_files)}] {status_color}{message:15s}{Colors.ENDC} {relative_path}")

            if args.check and message == "Needs formatting":
                needs_formatting.append(relative_path)
        else:
            failure_count += 1
            print(f"[{i:3d}/{len(source_files)}] {Colors.FAIL}{message:15s}{Colors.ENDC} {relative_path}")

    # Print summary
    print()
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}Summary{Colors.ENDC}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*70}{Colors.ENDC}")
    print(f"Total files:    {len(source_files)}")
    print(f"{Colors.OKGREEN}Success:        {success_count}{Colors.ENDC}")

    if failure_count > 0:
        print(f"{Colors.FAIL}Failures:       {failure_count}{Colors.ENDC}")

    if args.check and needs_formatting:
        print()
        print(f"{Colors.WARNING}Files needing formatting: {len(needs_formatting)}{Colors.ENDC}")
        for file in needs_formatting:
            print(f"  - {file}")
        print()
        print(f"{Colors.WARNING}Run without --check flag to format these files{Colors.ENDC}")
        return 1

    if failure_count > 0:
        return 1

    print()
    if args.check:
        print(f"{Colors.OKGREEN}✓ All files are properly formatted{Colors.ENDC}")
    else:
        print(f"{Colors.OKGREEN}✓ All files formatted successfully{Colors.ENDC}")

    return 0

if __name__ == '__main__':
    sys.exit(main())
