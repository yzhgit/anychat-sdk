#!/usr/bin/env python3
"""
Backup modified and untracked files in a Git repository to a specified directory.
Preserves the relative directory structure from the project root.
"""

import argparse
import subprocess
import sys
from pathlib import Path
import shutil


def get_git_root():
    """Get the root directory of the Git repository."""
    try:
        result = subprocess.run(
            ['git', 'rev-parse', '--show-toplevel'],
            capture_output=True,
            text=True,
            check=True
        )
        return Path(result.stdout.strip())
    except subprocess.CalledProcessError:
        print("Error: Not a git repository", file=sys.stderr)
        sys.exit(1)


def get_modified_files():
    """Get list of modified files (tracked changes)."""
    try:
        result = subprocess.run(
            ['git', 'diff', '--name-only', 'HEAD'],
            capture_output=True,
            text=True,
            check=True
        )
        files = result.stdout.strip().split('\n')
        return [f for f in files if f]  # Filter out empty strings
    except subprocess.CalledProcessError as e:
        print(f"Error getting modified files: {e}", file=sys.stderr)
        return []


def get_untracked_files():
    """Get list of untracked files (excluding ignored files)."""
    try:
        result = subprocess.run(
            ['git', 'ls-files', '--others', '--exclude-standard'],
            capture_output=True,
            text=True,
            check=True
        )
        files = result.stdout.strip().split('\n')
        return [f for f in files if f]  # Filter out empty strings
    except subprocess.CalledProcessError as e:
        print(f"Error getting untracked files: {e}", file=sys.stderr)
        return []


def backup_files(target_dir, files, git_root):
    """
    Backup files to target directory preserving relative paths.

    Args:
        target_dir: Destination directory for backups
        files: List of file paths relative to git root
        git_root: Path to git repository root
    """
    backup_count = 0
    error_count = 0

    for file_rel_path in files:
        source_file = git_root / file_rel_path
        dest_file = target_dir / file_rel_path

        # Skip if source file doesn't exist
        if not source_file.exists():
            print(f"Warning: File not found, skipping: {file_rel_path}", file=sys.stderr)
            error_count += 1
            continue

        # Create destination directory if needed
        dest_file.parent.mkdir(parents=True, exist_ok=True)

        try:
            # Copy file
            shutil.copy2(source_file, dest_file)
            print(f"Copied: {file_rel_path}")
            backup_count += 1
        except Exception as e:
            print(f"Error copying {file_rel_path}: {e}", file=sys.stderr)
            error_count += 1

    return backup_count, error_count


def main():
    parser = argparse.ArgumentParser(
        description='Backup modified and untracked Git files to a specified directory.'
    )
    parser.add_argument(
        'target',
        nargs='?',
        default='e:\\backup',
        help='Target directory for backup (default: e:\\backup)'
    )
    parser.add_argument(
        '--modified-only',
        action='store_true',
        help='Only backup modified files (exclude untracked files)'
    )
    parser.add_argument(
        '--untracked-only',
        action='store_true',
        help='Only backup untracked files (exclude modified files)'
    )

    args = parser.parse_args()

    # Get git root directory
    git_root = get_git_root()
    target_dir = Path(args.target).resolve()

    print(f"Git repository: {git_root}")
    print(f"Backup target: {target_dir}")
    print("-" * 60)

    # Get files to backup
    modified_files = [] if args.untracked_only else get_modified_files()
    untracked_files = [] if args.modified_only else get_untracked_files()

    all_files = modified_files + untracked_files

    if not all_files:
        print("No files to backup.")
        return 0

    print(f"Found {len(modified_files)} modified file(s)")
    print(f"Found {len(untracked_files)} untracked file(s)")
    print(f"Total: {len(all_files)} file(s) to backup")
    print("-" * 60)

    # Backup files
    backup_count, error_count = backup_files(target_dir, all_files, git_root)

    print("-" * 60)
    print(f"Backup completed: {backup_count} file(s) copied")
    if error_count > 0:
        print(f"Errors: {error_count} file(s) failed", file=sys.stderr)
        return 1

    return 0


if __name__ == '__main__':
    sys.exit(main())
