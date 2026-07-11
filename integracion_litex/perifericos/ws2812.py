from pathlib import Path

from migen import *

from litex.soc.interconnect.csr import AutoCSR, CSRStatus, CSRStorage


class WS2812(Module, AutoCSR):
    def __init__(self, platform, pad, clk_hz=60_000_000):
        # ------------------------------------------------------------
        # CSR de escritura
        # ------------------------------------------------------------

        self.pixel_index = CSRStorage(6)
        self.pixel_color = CSRStorage(24)
        self.write_pixel = CSRStorage(1)
        self.start       = CSRStorage(1)

        # ------------------------------------------------------------
        # CSR de estado
        # ------------------------------------------------------------

        self.busy = CSRStatus(1)
        self.done = CSRStatus(1)

        # ------------------------------------------------------------
        # Archivos RTL
        # ------------------------------------------------------------

        repo_root = Path(__file__).resolve().parents[2]

        rtl_dir = (
            repo_root
            / "rtl"
            / "cores"
            / "ws2812"
        )

        rtl_sources = [
            rtl_dir / "ws2812_framebuffer.v",
            rtl_dir / "ws2812_matrix_core.v",
        ]

        for source in rtl_sources:
            if not source.is_file():
                raise FileNotFoundError(
                    f"No se encontro el archivo RTL: {source}"
                )

            platform.add_source(str(source), "verilog")

        # ------------------------------------------------------------
        # Core WS2812
        # ------------------------------------------------------------

        self.specials += Instance(
            "ws2812_matrix_core",

            p_CLK_HZ   = int(clk_hz),
            p_NUM_LEDS = 64,

            i_clk   = ClockSignal("sys"),
            i_reset = ResetSignal("sys"),

            i_pixel_index = self.pixel_index.storage,
            i_pixel_color = self.pixel_color.storage,

            # Cada escritura genera un pulso de un ciclo.
            i_pixel_we = self.write_pixel.re,

            # Cada escritura genera un pulso de inicio.
            i_start = self.start.re,

            o_D_OUT = pad,
            o_busy  = self.busy.status,
            o_done  = self.done.status,
        )