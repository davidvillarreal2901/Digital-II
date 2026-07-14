from pathlib import Path

from migen import *

from litex.soc.interconnect.csr import AutoCSR, CSRStorage


class HUB75(Module, AutoCSR):
    def __init__(
        self,
        platform,
        pads,
        sys_clk_hz=60_000_000,
        panel_clk_hz=6_000_000,
    ):
        # ---------------------------------------------------------------------
        # CSR DEL FRAMEBUFFER
        #
        # Dirección:
        #   0...2047
        #   32 parejas de filas x 64 columnas
        #
        # Dato RGB444:
        #   [23:20] R1 / píxel superior
        #   [19:16] G1
        #   [15:12] B1
        #   [11:8]  R2 / píxel inferior
        #   [7:4]   G2
        #   [3:0]   B2
        # ---------------------------------------------------------------------

        self.pixel_addr  = CSRStorage(11)
        self.pixel_data  = CSRStorage(24)
        self.write_pixel = CSRStorage(1)

        # Panel apagado al arrancar.
        self.blank = CSRStorage(
            1,
            reset=1,
        )

        # ---------------------------------------------------------------------
        # RTL
        # ---------------------------------------------------------------------

        repo_root = Path(__file__).resolve().parents[2]
        rtl_dir = repo_root / "rtl" / "cores" / "hub75"

        rtl_sources = [
            rtl_dir / "hub75_core.v",
            rtl_dir / "count.v",
            rtl_dir / "comp.v",
            rtl_dir / "ctrl_lp4k.v",
            rtl_dir / "lsr_led.v",
            rtl_dir / "mux_led.v",
            rtl_dir / "ram_dual.v",
        ]

        for source in rtl_sources:
            if not source.is_file():
                raise FileNotFoundError(
                    f"No se encontro el archivo RTL: {source}"
                )

            platform.add_source(
                str(source),
                "systemverilog",
            )

        # ---------------------------------------------------------------------
        # INSTANCIA DEL NÚCLEO
        # ---------------------------------------------------------------------

        self.specials += Instance(
            "hub75_core",

            p_SYS_CLK_HZ   = int(sys_clk_hz),
            p_PANEL_CLK_HZ = int(panel_clk_hz),
            p_NUM_COLS     = 64,
            p_NUM_ADDR     = 2048,
            p_DELAY_VALUE  = 50,

            i_clk   = ClockSignal("sys"),
            i_reset = ResetSignal("sys"),

            i_pixel_addr = self.pixel_addr.storage,
            i_pixel_data = self.pixel_data.storage,
            i_pixel_we   = self.write_pixel.re,

            o_LP_CLK = pads.clk,
            o_LATCH  = pads.lat,
            o_NOE    = pads.oe,

            o_ROW  = pads.row,
            o_RGB0 = pads.rgb0,
            o_RGB1 = pads.rgb1,
            i_blank = self.blank.storage,
        )