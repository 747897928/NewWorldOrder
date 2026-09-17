#!/usr/bin/env python3
"""
Batch-resize PNG UI icons to an exact square size while preserving transparency.

Supports:
1. A directory containing PNG files.
2. A ZIP archive containing PNG files.

Examples:
    python resize_ui_icons.py T_UI_IconPack.zip
    python resize_ui_icons.py ./T_UI_IconPack --size 100
    python resize_ui_icons.py T_UI_IconPack.zip --output T_UI_IconPack_100x100.zip
"""

from __future__ import annotations

import argparse
import shutil
import tempfile
import zipfile
from pathlib import Path

from PIL import Image


def resize_png(source: Path, destination: Path, size: int) -> None:
    """Resize one PNG to size x size, preserving its alpha channel."""
    destination.parent.mkdir(parents=True, exist_ok=True)

    with Image.open(source) as image:
        # Convert palette/grayscale PNGs to RGBA so transparency is preserved reliably.
        image = image.convert("RGBA")
        resized = image.resize((size, size), Image.Resampling.LANCZOS)
        resized.save(destination, format="PNG", optimize=True)


def resize_directory(input_dir: Path, output_dir: Path, size: int) -> int:
    """Resize every PNG below input_dir, preserving subdirectory structure."""
    png_files = sorted(input_dir.rglob("*.png"))
    if not png_files:
        raise FileNotFoundError(f"No PNG files found in: {input_dir}")

    for source in png_files:
        relative = source.relative_to(input_dir)
        resize_png(source, output_dir / relative, size)

    return len(png_files)


def create_zip(source_dir: Path, zip_path: Path) -> None:
    """Create a ZIP archive containing all files below source_dir."""
    zip_path.parent.mkdir(parents=True, exist_ok=True)

    if zip_path.exists():
        zip_path.unlink()

    with zipfile.ZipFile(zip_path, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for file_path in sorted(source_dir.rglob("*")):
            if file_path.is_file():
                archive.write(file_path, file_path.relative_to(source_dir))


def process_zip(input_zip: Path, output_zip: Path, size: int) -> int:
    """Extract, resize all PNGs, and create a new ZIP archive."""
    with tempfile.TemporaryDirectory(prefix="resize_ui_icons_") as temp:
        temp_root = Path(temp)
        extracted_dir = temp_root / "input"
        resized_dir = temp_root / "output"
        extracted_dir.mkdir()
        resized_dir.mkdir()

        with zipfile.ZipFile(input_zip, "r") as archive:
            archive.extractall(extracted_dir)

        count = resize_directory(extracted_dir, resized_dir, size)
        create_zip(resized_dir, output_zip)
        return count


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Resize PNG UI icons to an exact square size."
    )
    parser.add_argument(
        "input",
        type=Path,
        help="Input directory or ZIP archive.",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=None,
        help="Output directory or ZIP path. A sensible default is used when omitted.",
    )
    parser.add_argument(
        "--size",
        type=int,
        default=100,
        help="Output width and height in pixels. Default: 100.",
    )
    return parser.parse_args()


def main() -> None:
    args = parse_args()

    if args.size <= 0:
        raise SystemExit("--size must be greater than zero.")

    input_path = args.input.resolve()

    if not input_path.exists():
        raise SystemExit(f"Input does not exist: {input_path}")

    if input_path.is_dir():
        output_path = (
            args.output.resolve()
            if args.output
            else input_path.with_name(f"{input_path.name}_{args.size}x{args.size}")
        )

        if output_path.exists():
            shutil.rmtree(output_path)

        count = resize_directory(input_path, output_path, args.size)
        print(f"Resized {count} PNG files.")
        print(f"Output directory: {output_path}")
        return

    if input_path.suffix.lower() == ".zip":
        output_path = (
            args.output.resolve()
            if args.output
            else input_path.with_name(
                f"{input_path.stem}_{args.size}x{args.size}.zip"
            )
        )

        count = process_zip(input_path, output_path, args.size)
        print(f"Resized {count} PNG files.")
        print(f"Output ZIP: {output_path}")
        return

    raise SystemExit("Input must be a directory or a .zip archive.")


if __name__ == "__main__":
    main()
