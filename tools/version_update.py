#!/usr/bin/env python3
"""
LoRaTNCX Version Update Tool

Command-line utility to update firmware version numbers in version.h

Usage:
    version_update.py --major <num> --minor <num> --patch <num>
    version_update.py --bump-major
    version_update.py --bump-minor
    version_update.py --bump-patch

Examples:
    # Set specific version
    version_update.py --major 1 --minor 2 --patch 3

    # Bump patch version
    version_update.py --bump-patch

    # Bump minor version (resets patch to 0)
    version_update.py --bump-minor
"""

import argparse
import re
import os
import sys

VERSION_FILE = os.path.join(os.path.dirname(__file__), '..', 'include', 'version.h')

def read_current_version():
    """Read current version from version.h"""
    if not os.path.exists(VERSION_FILE):
        print(f"Error: {VERSION_FILE} not found")
        return None

    with open(VERSION_FILE, 'r') as f:
        content = f.read()

    major_match = re.search(r'#define FIRMWARE_VERSION_MAJOR (\d+)', content)
    minor_match = re.search(r'#define FIRMWARE_VERSION_MINOR (\d+)', content)
    patch_match = re.search(r'#define FIRMWARE_VERSION_PATCH (\d+)', content)

    if major_match is None or minor_match is None or patch_match is None:
        print("Error: Could not parse version defines")
        return None

    return {
        'major': int(major_match.group(1)),
        'minor': int(minor_match.group(1)),
        'patch': int(patch_match.group(1))
    }

def update_version(major=None, minor=None, patch=None):
    """Update version in version.h"""
    current = read_current_version()
    if not current:
        return False

    new_major = major if major is not None else current['major']
    new_minor = minor if minor is not None else current['minor']
    new_patch = patch if patch is not None else current['patch']

    version_string = f"{new_major}.{new_minor}.{new_patch}"

    with open(VERSION_FILE, 'r') as f:
        content = f.read()

    # Update defines
    content = re.sub(r'#define FIRMWARE_VERSION_MAJOR \d+', f'#define FIRMWARE_VERSION_MAJOR {new_major}', content)
    content = re.sub(r'#define FIRMWARE_VERSION_MINOR \d+', f'#define FIRMWARE_VERSION_MINOR {new_minor}', content)
    content = re.sub(r'#define FIRMWARE_VERSION_PATCH \d+', f'#define FIRMWARE_VERSION_PATCH {new_patch}', content)
    content = re.sub(r'#define FIRMWARE_VERSION_STRING "[^"]*"', f'#define FIRMWARE_VERSION_STRING "{version_string}"', content)

    with open(VERSION_FILE, 'w') as f:
        f.write(content)

    print(f"Updated version to {version_string}")
    return True

def main():
    parser = argparse.ArgumentParser(description='Update LoRaTNCX firmware version')
    parser.add_argument('--major', type=int, help='Set major version')
    parser.add_argument('--minor', type=int, help='Set minor version')
    parser.add_argument('--patch', type=int, help='Set patch version')
    parser.add_argument('--bump-major', action='store_true', help='Bump major version')
    parser.add_argument('--bump-minor', action='store_true', help='Bump minor version')
    parser.add_argument('--bump-patch', action='store_true', help='Bump patch version')

    args = parser.parse_args()

    if args.bump_major or args.bump_minor or args.bump_patch:
        current = read_current_version()
        if not current:
            return 1

        if args.bump_major:
            update_version(major=current['major'] + 1, minor=0, patch=0)
        elif args.bump_minor:
            update_version(major=current['major'], minor=current['minor'] + 1, patch=0)
        elif args.bump_patch:
            update_version(major=current['major'], minor=current['minor'], patch=current['patch'] + 1)
    else:
        if not any([args.major is not None, args.minor is not None, args.patch is not None]):
            print("Error: Must specify version numbers or bump option")
            return 1
        update_version(args.major, args.minor, args.patch)

    return 0

if __name__ == '__main__':
    sys.exit(main())