#!/usr/bin/env python3
"""
format-code.py - Source code formatter for C/C++ projects.

Applies consistent formatting to source files:
- Updates license headers (GPL2/LGPL3)
- Converts 4 spaces to tabs
- Converts local includes from <> to ""
- Ensures trailing newline (POSIX compliance)

Usage:
    ./format-code.py [--config config.json]
    ./format-code.py --project-name "Name" --project-url "URL" --license lgpl3 --paths src
"""

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Optional


@dataclass
class ProjectConfig:
    """Project configuration for code formatting."""
    project_name: str
    project_url: str
    author: str
    license_type: str  # 'gpl2' or 'lgpl3'
    paths: list[str]
    extensions: list[str]
    exclusions: list[str]


def get_gpl2_license(filepath: str, config: ProjectConfig) -> str:
    """Generate GPL2 license header."""
    year = datetime.now().year
    return f"""/*
 * {filepath}
 * This file is part of {config.project_name}
 *
 * Copyright (C) 2010-{year} - {config.author}
 *
 * {config.project_name} is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * {config.project_name} is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with {config.project_name}; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA  02110-1301  USA
 *
 * Complete project and additional information can be found at :
 * {config.project_url}
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

"""


def get_lgpl3_license(filepath: str, config: ProjectConfig) -> str:
    """Generate LGPL3 license header."""
    year = datetime.now().year
    return f"""/*
 * {filepath}
 * This file is part of {config.project_name}
 *
 * Copyright (C) 2010-{year} - {config.author}
 *
 * {config.project_name} is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 *
 * {config.project_name} is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with {config.project_name}; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * Complete project and additional information can be found at :
 * {config.project_url}
 *
 * --- THIS IS AUTOMATICALLY GENERATED, DO NOT CHANGE ---
 */

"""


def convert_spaces_to_tabs(content: str) -> str:
    """Convert 4 consecutive spaces to tabs."""
    return content.replace("    ", "\t")


def convert_local_includes(content: str) -> str:
    """Convert #include <local.hpp> to #include "local.hpp" for local files."""
    # Match #include <something.hpp> but not system headers (no dots or specific patterns)
    pattern = r'#include <([^.>]+\.hpp)>'
    return re.sub(pattern, r'#include "\1"', content)


def strip_existing_license(content: str, min_start: int = 64, max_length: int = 512) -> str:
    """
    Remove existing license header from content.

    Looks for a /* ... */ block at the start of the file that's likely a license.
    """
    start = content.find("/*")

    if start >= 0 and start < min_start:
        end = content.find("*/", start)

        if end > 0 and (end - start) > max_length:
            # Found a large comment block at the start - likely a license
            content = content[end + 2:]
            return content.strip() + "\n"

    return content


def ensure_trailing_newline(content: str) -> str:
    """Ensure file ends with exactly one newline (POSIX compliance, macOS friendly)."""
    content = content.rstrip()
    return content + "\n"


def get_license_header(filepath: str, config: ProjectConfig) -> str:
    """Get the appropriate license header based on config."""
    if config.license_type == "gpl2":
        return get_gpl2_license(filepath, config)
    elif config.license_type == "lgpl3":
        return get_lgpl3_license(filepath, config)
    else:
        return ""


def process_file(filepath: Path, base_path: Path, config: ProjectConfig) -> bool:
    """
    Process a single source file.

    Returns True if file was modified, False otherwise.
    """
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception as e:
        print(f"Unable to read '{filepath}': {e}", file=sys.stderr)
        return False

    # Calculate relative path for license header
    relative_path = str(filepath.relative_to(base_path))

    # Apply transformations
    updated = strip_existing_license(content)
    updated = get_license_header(relative_path, config) + updated
    updated = convert_spaces_to_tabs(updated)
    updated = convert_local_includes(updated)
    updated = ensure_trailing_newline(updated)

    # Only write if content changed
    if content == updated:
        print(f"No change in '{filepath}'. Skipping...")
        return False

    try:
        filepath.write_text(updated, encoding="utf-8")
        print(f"Updated '{filepath}'")
        return True
    except Exception as e:
        print(f"Unable to write '{filepath}': {e}", file=sys.stderr)
        sys.exit(1)


def should_exclude(filepath: Path, exclusions: list[str]) -> bool:
    """Check if filepath should be excluded based on exclusion patterns."""
    path_str = str(filepath)
    return any(excl in path_str for excl in exclusions)


def crawl_directory(base_path: Path, config: ProjectConfig) -> tuple[int, int]:
    """
    Recursively process all matching files in directory.

    Returns tuple of (files_processed, files_modified).
    """
    processed = 0
    modified = 0

    for path in sorted(base_path.rglob("*")):
        if not path.is_file():
            continue

        # Check exclusions
        if should_exclude(path, config.exclusions):
            continue

        # Check extension
        ext = path.suffix.lstrip(".")
        if ext not in config.extensions:
            continue

        processed += 1
        if process_file(path, base_path.parent, config):
            modified += 1

    return processed, modified


def load_config(config_path: Optional[Path]) -> Optional[ProjectConfig]:
    """Load configuration from JSON file."""
    if config_path is None or not config_path.exists():
        return None

    try:
        with open(config_path, "r", encoding="utf-8") as f:
            data = json.load(f)

        return ProjectConfig(
            project_name=data["project_name"],
            project_url=data["project_url"],
            author=data["author"],
            license_type=data.get("license", "lgpl3"),
            paths=data.get("paths", ["./src"]),
            extensions=data.get("extensions", ["h", "hpp", "c", "cpp", "in"]),
            exclusions=data.get("exclusions", ["txt"])
        )
    except Exception as e:
        print(f"Error loading config: {e}", file=sys.stderr)
        return None


def main():
    parser = argparse.ArgumentParser(
        description="Format C/C++ source files with license headers and consistent style."
    )
    parser.add_argument(
        "--config", "-c",
        type=Path,
        help="Path to JSON configuration file"
    )
    parser.add_argument(
        "--project-name",
        help="Project name for license header"
    )
    parser.add_argument(
        "--project-url",
        help="Project URL for license header"
    )
    parser.add_argument(
        "--author",
        default='Sébastien Léon Claude Christian Bémelmans "LondNoir" <londnoir@gmail.com>',
        help="Author name and email for license header"
    )
    parser.add_argument(
        "--license", "-l",
        choices=["gpl2", "lgpl3"],
        default="lgpl3",
        help="License type (default: lgpl3)"
    )
    parser.add_argument(
        "--paths", "-p",
        nargs="+",
        default=["./src"],
        help="Paths to process (default: ./src)"
    )
    parser.add_argument(
        "--extensions", "-e",
        nargs="+",
        default=["h", "hpp", "c", "cpp", "in"],
        help="File extensions to process"
    )
    parser.add_argument(
        "--exclusions", "-x",
        nargs="+",
        default=["txt"],
        help="Patterns to exclude from processing"
    )
    parser.add_argument(
        "--dry-run", "-n",
        action="store_true",
        help="Show what would be done without making changes"
    )

    args = parser.parse_args()

    # Try to load config from file first
    config = load_config(args.config)

    # Override with command-line arguments if provided
    if config is None:
        if not args.project_name or not args.project_url:
            # Try to find format-code.json in current directory
            default_config = Path("format-code.json")
            config = load_config(default_config)

            if config is None:
                print("Error: Either --config, or --project-name and --project-url are required.", file=sys.stderr)
                print("Alternatively, create a format-code.json in the current directory.", file=sys.stderr)
                sys.exit(1)
        else:
            config = ProjectConfig(
                project_name=args.project_name,
                project_url=args.project_url,
                author=args.author,
                license_type=args.license,
                paths=args.paths,
                extensions=args.extensions,
                exclusions=args.exclusions
            )

    # Process all paths
    total_processed = 0
    total_modified = 0

    for path_str in config.paths:
        path = Path(path_str)
        if not path.exists():
            print(f"Warning: Path '{path}' does not exist, skipping...", file=sys.stderr)
            continue

        if path.is_file():
            total_processed += 1
            if process_file(path, path.parent, config):
                total_modified += 1
        else:
            processed, modified = crawl_directory(path, config)
            total_processed += processed
            total_modified += modified

    print(f"\n{total_processed} files processed, {total_modified} files modified.")

if __name__ == "__main__":
    main()
