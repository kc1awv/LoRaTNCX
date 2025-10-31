#!/usr/bin/env python3
"""Prepare SPIFFS assets by trimming whitespace and bundling them into a zip archive."""

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path
from typing import Iterable
import zipfile

TEXT_EXTENSIONS = {'.html', '.htm', '.js', '.css', '.json', '.txt'}


def minify_text(content: str) -> str:
    lines = [line.rstrip() for line in content.splitlines()]
    minified: list[str] = []
    blank = False
    for line in lines:
        if line.strip():
            minified.append(line)
            blank = False
        else:
            if not blank:
                minified.append('')
            blank = True
    result = '\n'.join(minified).strip()
    return result


def minify_html(content: str) -> str:
    compact = minify_text(content)
    # Collapse whitespace between tags while keeping a single space where needed.
    compact = re.sub(r'>\s+<', '><', compact)
    return compact


def iter_files(base: Path) -> Iterable[Path]:
    for path in base.rglob('*'):
        if path.is_file():
            yield path


def process_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)

    if source.suffix.lower() in TEXT_EXTENSIONS:
        text = source.read_text(encoding='utf-8')
        if source.suffix.lower() in {'.html', '.htm'}:
            text = minify_html(text)
        else:
            text = minify_text(text)
        if text:
            destination.write_text(text + '\n', encoding='utf-8')
        else:
            destination.write_text('', encoding='utf-8')
    else:
        shutil.copy2(source, destination)


def build_archive(source: Path, build_dir: Path, output: Path) -> None:
    if build_dir.exists():
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)

    for path in iter_files(source):
        relative = path.relative_to(source)
        destination = build_dir / relative
        process_file(path, destination)

    output.parent.mkdir(parents=True, exist_ok=True)
    with zipfile.ZipFile(output, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
        for path in iter_files(build_dir):
            archive.write(path, arcname=path.relative_to(build_dir))


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('-s', '--source', type=Path, default=Path('data'), help='UI source directory (default: data)')
    parser.add_argument('-b', '--build-dir', type=Path, default=Path('build/ui'), help='Temporary build directory')
    parser.add_argument('-o', '--output', type=Path, default=Path('build/ui.zip'), help='Output zip path')
    return parser.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    source = args.source

    if not source.exists():
        print(f"Source directory '{source}' does not exist", file=sys.stderr)
        return 1

    build_dir = args.build_dir
    output = args.output

    build_archive(source.resolve(), build_dir.resolve(), output.resolve())
    print(f"Created archive: {output.resolve()}")
    return 0


if __name__ == '__main__':
    raise SystemExit(main(sys.argv[1:]))
