#!/usr/bin/env python3

import argparse
import re
from pathlib import Path

from PIL import Image


WIDTH = 64
HEIGHT = 64
SCAN_ROWS = 32
WORDS_PER_FRAME = WIDTH * SCAN_ROWS


def sanitize_identifier(name: str) -> str:
    identifier = re.sub(r"[^a-zA-Z0-9_]", "_", name)

    if not identifier:
        identifier = "hub75_asset"

    if identifier[0].isdigit():
        identifier = f"asset_{identifier}"

    return identifier.lower()


def resize_frame(
    frame: Image.Image,
    nearest: bool,
) -> Image.Image:
    resampling = (
        Image.Resampling.NEAREST
        if nearest
        else Image.Resampling.LANCZOS
    )

    rgba = frame.convert("RGBA")

    background = Image.new(
        "RGBA",
        rgba.size,
        (0, 0, 0, 255),
    )

    background.alpha_composite(rgba)

    return background.convert("RGB").resize(
        (WIDTH, HEIGHT),
        resampling,
    )


def swap_red_blue(
    color: tuple[int, int, int],
    enabled: bool,
) -> tuple[int, int, int]:
    red, green, blue = color

    if enabled:
        return blue, green, red

    return red, green, blue


def pack_frame(
    frame: Image.Image,
    swap_rb: bool,
) -> list[int]:
    words: list[int] = []

    pixels = frame.load()

    for row in range(SCAN_ROWS):
        for column in range(WIDTH):
            top = swap_red_blue(
                pixels[column, row],
                swap_rb,
            )

            bottom = swap_red_blue(
                pixels[column, row + SCAN_ROWS],
                swap_rb,
            )

            r0, g0, b0 = top
            r1, g1, b1 = bottom

            r0 >>= 4
            g0 >>= 4
            b0 >>= 4

            r1 >>= 4
            g1 >>= 4
            b1 >>= 4

            word = (
                (r0 << 20)
                | (g0 << 16)
                | (b0 << 12)
                | (r1 << 8)
                | (g1 << 4)
                | b1
            )

            words.append(word)

    if len(words) != WORDS_PER_FRAME:
        raise RuntimeError(
            f"Frame invalido: {len(words)} palabras, "
            f"se esperaban {WORDS_PER_FRAME}"
        )

    return words


def read_frames(
    source: Path,
    max_frames: int,
    every: int,
    nearest: bool,
    swap_rb: bool,
    default_delay_ms: int,
) -> tuple[list[list[int]], list[int]]:
    image = Image.open(source)

    frames: list[list[int]] = []
    delays: list[int] = []

    frame_index = 0
    selected_index = 0

    while True:
        try:
            image.seek(frame_index)
        except EOFError:
            break

        if frame_index % every == 0:
            converted = resize_frame(
                image.copy(),
                nearest,
            )

            packed = pack_frame(
                converted,
                swap_rb,
            )

            duration = int(
                image.info.get(
                    "duration",
                    default_delay_ms,
                )
            )

            if duration <= 0:
                duration = default_delay_ms

            duration = max(20, min(duration, 5000))

            frames.append(packed)
            delays.append(duration)

            print(
                f"Frame {selected_index}: "
                f"origen={frame_index}, "
                f"delay={duration} ms"
            )

            selected_index += 1

            if max_frames > 0 and len(frames) >= max_frames:
                break

        frame_index += 1

    if not frames:
        raise RuntimeError(
            "No se pudo extraer ningun frame"
        )

    return frames, delays


def write_header(
    output: Path,
    frames: list[list[int]],
    delays: list[int],
    identifier: str,
) -> None:
    guard = f"{identifier.upper()}_H"

    with output.open(
        "w",
        encoding="utf-8",
    ) as file:
        file.write(
            f"#ifndef {guard}\n"
            f"#define {guard}\n\n"
            "#include <stdint.h>\n\n"
            f"#define HUB75_ASSET_FRAME_COUNT {len(frames)}u\n"
            f"#define HUB75_ASSET_WORDS_PER_FRAME {WORDS_PER_FRAME}u\n\n"
        )

        file.write(
            "static const uint16_t "
            "hub75_asset_delay_ms"
            "[HUB75_ASSET_FRAME_COUNT] = {\n    "
        )

        file.write(
            ", ".join(
                f"{delay}u"
                for delay in delays
            )
        )

        file.write("\n};\n\n")

        file.write(
            "static const uint32_t "
            "hub75_asset_frames"
            "[HUB75_ASSET_FRAME_COUNT]"
            "[HUB75_ASSET_WORDS_PER_FRAME] = {\n"
        )

        for frame_index, frame in enumerate(frames):
            file.write(
                f"    /* Frame {frame_index} */\n"
                "    {\n"
            )

            for offset in range(
                0,
                len(frame),
                8,
            ):
                row = frame[offset:offset + 8]

                file.write(
                    "        "
                    + ", ".join(
                        f"0x{word:06X}u"
                        for word in row
                    )
                    + ",\n"
                )

            file.write("    },\n")

        file.write(
            "};\n\n"
            f"#endif /* {guard} */\n"
        )


def main() -> None:
    parser = argparse.ArgumentParser(
        description=(
            "Convierte una imagen o GIF al formato "
            "RGB444 usado por el controlador HUB75."
        )
    )

    parser.add_argument(
        "entrada",
        type=Path,
        help="Imagen o GIF de entrada",
    )

    parser.add_argument(
        "salida",
        type=Path,
        help="Archivo .h de salida",
    )

    parser.add_argument(
        "--max-frames",
        type=int,
        default=8,
        help="Numero maximo de frames. 0 significa todos.",
    )

    parser.add_argument(
        "--cada",
        type=int,
        default=1,
        help="Tomar uno de cada N frames.",
    )

    parser.add_argument(
        "--delay",
        type=int,
        default=100,
        help="Delay por defecto en milisegundos.",
    )

    parser.add_argument(
        "--nearest",
        action="store_true",
        help="Usar vecino mas cercano para pixel art.",
    )

    parser.add_argument(
        "--sin-swap-rb",
        action="store_true",
        help=(
            "No intercambiar rojo y azul. "
            "Por defecto se conserva el comportamiento "
            "del repositorio antiguo."
        ),
    )

    args = parser.parse_args()

    if args.cada < 1:
        parser.error("--cada debe ser mayor o igual a 1")

    if not args.entrada.is_file():
        parser.error(
            f"No existe: {args.entrada}"
        )

    args.salida.parent.mkdir(
        parents=True,
        exist_ok=True,
    )

    identifier = sanitize_identifier(
        args.salida.stem
    )

    frames, delays = read_frames(
        source=args.entrada,
        max_frames=args.max_frames,
        every=args.cada,
        nearest=args.nearest,
        swap_rb=not args.sin_swap_rb,
        default_delay_ms=args.delay,
    )

    write_header(
        output=args.salida,
        frames=frames,
        delays=delays,
        identifier=identifier,
    )

    print()
    print(f"Generado: {args.salida}")
    print(f"Frames: {len(frames)}")
    print(
        "Palabras RGB444: "
        f"{len(frames) * WORDS_PER_FRAME}"
    )


if __name__ == "__main__":
    main()