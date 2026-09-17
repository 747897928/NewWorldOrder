#!/usr/bin/env python3
"""
Split uniformly spaced horizontal icon strips into square PNG files.

Example:
    python split_icons.py \
        "strip1.png" "strip2.png" "strip3.png" "strip4.png" \
        --counts 5 7 4 3 \
        --output wardrobe_icon_pack \
        --size 512 \
        --prefix T_XB

Dependencies:
    pip install pillow numpy opencv-python
"""

from pathlib import Path
from PIL import Image
import argparse
import csv
import numpy as np
import cv2


def crop_equal_square(image_rgba: np.ndarray, count: int, index: int) -> np.ndarray:
    height, width = image_rgba.shape[:2]
    cell_width = width / count
    side = int(round(cell_width))
    center_x = (index + 0.5) * cell_width
    center_y = height / 2
    x0 = int(round(center_x - side / 2))
    y0 = int(round(center_y - side / 2))
    x1, y1 = x0 + side, y0 + side

    result = np.zeros((side, side, 4), dtype=np.uint8)
    result[..., :3] = 255
    result[..., 3] = 255

    src_x0, src_y0 = max(0, x0), max(0, y0)
    src_x1, src_y1 = min(width, x1), min(height, y1)
    dst_x0, dst_y0 = src_x0 - x0, src_y0 - y0

    result[
        dst_y0:dst_y0 + (src_y1 - src_y0),
        dst_x0:dst_x0 + (src_x1 - src_x0),
    ] = image_rgba[src_y0:src_y1, src_x0:src_x1]
    return result


def estimate_foreground_alpha(rgb: np.ndarray) -> np.ndarray:
    f = rgb.astype(np.float32)
    red, green, blue_channel = f[..., 0], f[..., 1], f[..., 2]
    maximum = np.max(f, axis=2)
    minimum = np.min(f, axis=2)
    saturation_range = maximum - minimum
    blue_excess = blue_channel - (red + green) / 2.0
    luminance = 0.2126 * red + 0.7152 * green + 0.0722 * blue_channel

    color_score = np.maximum(
        np.clip((blue_excess - 0.5) / 18.0, 0.0, 1.0),
        np.clip((saturation_range - 2.0) / 35.0, 0.0, 1.0),
    )
    hard_seed = ((blue_excess > 5.0) | (saturation_range > 14.0)).astype(np.uint8) * 255

    hard_near = cv2.dilate(
        hard_seed, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (13, 13)), 1
    )
    vicinity = cv2.dilate(
        hard_seed, cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (31, 31)), 1
    ).astype(np.float32) / 255.0

    nearby_contrast = np.maximum(
        np.clip((luminance - 247.0) / 8.0, 0.0, 1.0),
        np.clip((232.0 - luminance) / 35.0, 0.0, 1.0),
    )
    alpha = np.maximum(color_score, nearby_contrast * vicinity)

    binary = (alpha > 0.08).astype(np.uint8) * 255
    binary = cv2.morphologyEx(
        binary,
        cv2.MORPH_CLOSE,
        cv2.getStructuringElement(cv2.MORPH_ELLIPSE, (5, 5)),
        iterations=2,
    )

    component_count, labels, stats, centers = cv2.connectedComponentsWithStats(binary, 8)
    kept = np.zeros_like(binary)
    height, width = binary.shape

    for component_id in range(1, component_count):
        component = labels == component_id
        area = int(stats[component_id, cv2.CC_STAT_AREA])
        overlap = int(np.count_nonzero(component & (hard_near > 0)))
        center_x, center_y = centers[component_id]

        if (
            area >= 20
            and overlap >= max(2, int(area * 0.002))
            and 0.02 * width < center_x < 0.98 * width
            and 0.02 * height < center_y < 0.98 * height
        ):
            kept[component] = 255

    alpha *= kept.astype(np.float32) / 255.0
    expanded_kept = cv2.dilate(kept, np.ones((7, 7), np.uint8), 1)
    white_linework = ((luminance > 247.0) & (expanded_kept > 0)).astype(np.float32)
    alpha = np.maximum(alpha, white_linework * 0.95)
    alpha = np.maximum(alpha, (hard_seed.astype(np.float32) / 255.0) * 0.98)
    alpha = cv2.GaussianBlur(alpha, (0, 0), 0.8)

    alpha = np.clip(alpha * 255.0, 0, 255).astype(np.uint8)
    alpha[alpha < 10] = 0
    return alpha


def make_transparent_icon(square_rgba: np.ndarray, size: int, padding: int) -> Image.Image:
    rgb = square_rgba[..., :3]
    alpha = estimate_foreground_alpha(rgb)
    foreground = np.argwhere(alpha > 10)
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    if foreground.size == 0:
        return canvas

    y0, x0 = foreground.min(axis=0)
    y1, x1 = foreground.max(axis=0) + 1
    source_pad = max(4, int(round(max(square_rgba.shape[:2]) * 0.02)))
    x0, y0 = max(0, x0 - source_pad), max(0, y0 - source_pad)
    x1 = min(square_rgba.shape[1], x1 + source_pad)
    y1 = min(square_rgba.shape[0], y1 + source_pad)

    icon = Image.fromarray(np.dstack([rgb, alpha])[y0:y1, x0:x1], "RGBA")
    max_content = size - padding * 2
    scale = min(max_content / icon.width, max_content / icon.height)
    new_size = (
        max(1, int(round(icon.width * scale))),
        max(1, int(round(icon.height * scale))),
    )
    icon = icon.resize(new_size, Image.Resampling.LANCZOS)
    canvas.alpha_composite(icon, ((size - new_size[0]) // 2, (size - new_size[1]) // 2))
    return canvas


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("images", nargs="+", type=Path)
    parser.add_argument("--counts", nargs="+", type=int, required=True)
    parser.add_argument("--output", type=Path, default=Path("icon_pack"))
    parser.add_argument("--size", type=int, default=512)
    parser.add_argument("--padding", type=int, default=42)
    parser.add_argument("--prefix", default="T_XB")
    args = parser.parse_args()

    if len(args.images) != len(args.counts):
        raise SystemExit("The number of images must match the number of --counts values.")

    raw_dir = args.output / "raw_square"
    transparent_dir = args.output / "transparent"
    raw_dir.mkdir(parents=True, exist_ok=True)
    transparent_dir.mkdir(parents=True, exist_ok=True)

    manifest = []

    for set_index, (path, count) in enumerate(zip(args.images, args.counts), start=1):
        source = np.array(Image.open(path).convert("RGBA"))

        for icon_index in range(count):
            letter = chr(ord("A") + icon_index)
            name = f"{args.prefix}{set_index}_{letter}"
            filename = f"{name}.png"
            square = crop_equal_square(source, count, icon_index)

            Image.fromarray(square, "RGBA").resize(
                (args.size, args.size), Image.Resampling.LANCZOS
            ).save(raw_dir / filename, optimize=True)

            make_transparent_icon(square, args.size, args.padding).save(
                transparent_dir / filename, optimize=True
            )

            manifest.append(
                {
                    "texture_name": name,
                    "source_file": path.name,
                    "source_position": icon_index + 1,
                    "transparent_file": f"transparent/{filename}",
                    "raw_square_file": f"raw_square/{filename}",
                }
            )

    with (args.output / "manifest.csv").open(
        "w", encoding="utf-8-sig", newline=""
    ) as file:
        writer = csv.DictWriter(file, fieldnames=manifest[0].keys())
        writer.writeheader()
        writer.writerows(manifest)


if __name__ == "__main__":
    main()
