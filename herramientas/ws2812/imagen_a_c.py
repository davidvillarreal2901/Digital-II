#!/usr/bin/env python3

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image


WIDTH = 8
HEIGHT = 8
NUM_PIXELS = WIDTH * HEIGHT


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Convierte una imagen RGB a un header C "
            "para la matriz WS2812 8x8."
        )
    )

    parser.add_argument(
        "entrada",
        type=Path,
        help="Imagen PNG, JPG, WEBP, etc.",
    )

    parser.add_argument(
        "salida",
        type=Path,
        help="Archivo .h de salida.",
    )

    parser.add_argument(
        "--brillo",
        type=int,
        default=32,
        help=(
            "Brillo maximo por canal entre 0 y 255. "
            "Por defecto: 32."
        ),
    )

    return parser.parse_args()


def escalar_canal(
    value: int,
    brillo: int,
) -> int:
    return round(
        value * brillo / 255
    )


def main() -> None:
    args = parse_args()

    if not args.entrada.is_file():
        raise FileNotFoundError(
            f"No se encontro la imagen: {args.entrada}"
        )

    if not 0 <= args.brillo <= 255:
        raise ValueError(
            "--brillo debe estar entre 0 y 255"
        )

    with Image.open(args.entrada) as image:
        image = image.convert("RGB")

        print(
            f"Imagen original: "
            f"{image.width}x{image.height}"
        )

        if image.size != (WIDTH, HEIGHT):
             image = image.resize(
                (WIDTH, HEIGHT),
                Image.Resampling.NEAREST,
            )

        pixels_rgb = list(
            image.getdata()
        )

    if len(pixels_rgb) != NUM_PIXELS:
        raise RuntimeError(
            "La imagen procesada no contiene 64 pixeles"
        )

    colors: list[int] = []

    for red, green, blue in pixels_rgb:
        red = escalar_canal(
            red,
            args.brillo,
        )

        green = escalar_canal(
            green,
            args.brillo,
        )

        blue = escalar_canal(
            blue,
            args.brillo,
        )

        color = (
            (red << 16)
            | (green << 8)
            | blue
        )

        colors.append(color)

    args.salida.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    lines: list[str] = []

    lines.append(
        "#ifndef IMAGEN_WS2812_H"
    )

    lines.append(
        "#define IMAGEN_WS2812_H"
    )

    lines.append("")
    lines.append("#include <stdint.h>")
    lines.append("")

    lines.append(
        "#define IMAGEN_WS2812_WIDTH  8"
    )

    lines.append(
        "#define IMAGEN_WS2812_HEIGHT 8"
    )

    lines.append(
        "#define IMAGEN_WS2812_PIXELS 64"
    )

    lines.append("")

    lines.append(
        "static const uint32_t imagen_ws2812[64] = {"
    )

    for y in range(HEIGHT):
        row = colors[
            y * WIDTH:
            (y + 1) * WIDTH
        ]

        values = ", ".join(
            f"0x{color:06X}"
            for color in row
        )

        lines.append(
            f"    {values},"
        )

    lines.append("};")
    lines.append("")
    lines.append("#endif")
    lines.append("")

    args.salida.write_text(
        "\n".join(lines),
        encoding="utf-8",
    )

    print(
        f"Imagen WS2812: {WIDTH}x{HEIGHT}"
    )

    print(
        f"Pixeles: {len(colors)}"
    )

    print(
        f"Brillo maximo: {args.brillo}/255"
    )

    print(
        f"Header generado: {args.salida}"
    )

    print("")
    print("Vista RGB procesada:")

    for y in range(HEIGHT):
        row = colors[
            y * WIDTH:
            (y + 1) * WIDTH
        ]

        print(
            " ".join(
                f"{color:06X}"
                for color in row
            )
        )


if __name__ == "__main__":
    main()