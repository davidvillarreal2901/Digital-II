from pathlib import Path

from migen import *
from migen.fhdl.specials import TSTriple

from litex.soc.interconnect.csr import AutoCSR, CSRStatus, CSRStorage

class I2CMaster(Module, AutoCSR):
    def __init__(self, platform, pads, clk_hz=60_000_000, i2c_hz=100_000):
        # ------------------------------------------------------------
        # CSR de control
        # ------------------------------------------------------------

        self.address = CSRStorage(7)
        self.data0   = CSRStorage(8)
        self.data1   = CSRStorage(8)
        self.start   = CSRStorage(1)

        # ------------------------------------------------------------
        # CSR de estado
        # ------------------------------------------------------------

        self.busy      = CSRStatus(1)
        self.done      = CSRStatus(1)
        self.ack_error = CSRStatus(1)

        # ------------------------------------------------------------
        # Señales internas open-drain
        # ------------------------------------------------------------

        scl_oen = Signal()
        sda_oen = Signal()
        sda_in  = Signal()

        # ------------------------------------------------------------
        # Archivos RTL
        # ------------------------------------------------------------

        repo_root = Path(__file__).resolve().parents[2]

        rtl_dir = (
            repo_root
            / "rtl"
            / "cores"
            / "i2c"
        )

        rtl_sources = [
            rtl_dir / "i2c_master.v",
        ]

        for source in rtl_sources:
            if not source.is_file():
                raise FileNotFoundError(
                    f"No se encontro el archivo RTL: {source}"
                )

            platform.add_source(str(source), "verilog")

        # ------------------------------------------------------------
        # Open-drain real
        #
        # i2c_master usa:
        #
        #   *_oen = 1 -> liberar linea
        #   *_oen = 0 -> forzar LOW
        #
        # TSTriple usa:
        #
        #   oe = 1 -> manejar salida
        #   oe = 0 -> alta impedancia
        #
        # Por eso:
        #
        #   oe = ~*_oen
        #   salida siempre = 0
        # ------------------------------------------------------------

        scl_tristate = TSTriple()
        sda_tristate = TSTriple()

        self.specials += scl_tristate.get_tristate(pads.scl)
        self.specials += sda_tristate.get_tristate(pads.sda)

        self.comb += [
            scl_tristate.o.eq(0),
            scl_tristate.oe.eq(~scl_oen),

            sda_tristate.o.eq(0),
            sda_tristate.oe.eq(~sda_oen),

            sda_in.eq(sda_tristate.i),
]

        # ------------------------------------------------------------
        # Core I2C
        # ------------------------------------------------------------

        self.specials += Instance(
            "i2c_master",

            p_CLK_HZ = int(clk_hz),
            p_I2C_HZ = int(i2c_hz),

            i_clk   = ClockSignal("sys"),
            i_reset = ResetSignal("sys"),

            i_start   = self.start.re,
            i_address = self.address.storage,
            i_data0   = self.data0.storage,
            i_data1   = self.data1.storage,

            i_sda_in = sda_in,

            o_scl_oen = scl_oen,
            o_sda_oen = sda_oen,

            o_busy      = self.busy.status,
            o_done      = self.done.status,
            o_ack_error = self.ack_error.status,
        )