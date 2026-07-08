from pathlib import Path

from migen import *
from litex.soc.interconnect.csr import *


class Calculator(Module, AutoCSR):
    def __init__(self, platform):
        self.op1    = CSRStorage(32)
        self.op2    = CSRStorage(32)
        self.opcode = CSRStorage(3)
        self.start  = CSRStorage(1)

        self.result = CSRStatus(32)
        self.done   = CSRStatus(1)
        self.busy   = CSRStatus(1)
        self.error  = CSRStatus(1)

        repo_root = Path(__file__).resolve().parents[2]
        rtl_dir = repo_root / "rtl" / "cores" / "calculadora"

        rtl_sources = [
            rtl_dir / "calc_core.sv",
            rtl_dir / "multiplicador.sv",
            rtl_dir / "divisor.sv",
            rtl_dir / "raiz.sv",
        ]

        for source in rtl_sources:
            if not source.is_file():
                raise FileNotFoundError(
                    f"No se encontro el archivo RTL: {source}"
                )

            platform.add_source(str(source), "systemverilog")

        self.specials += Instance(
            "calc_core",

            i_clk    = ClockSignal("sys"),
            i_rst    = ResetSignal("sys"),

            i_op1    = self.op1.storage,
            i_op2    = self.op2.storage,
            i_opcode = self.opcode.storage,
            i_start  = self.start.re,

            o_result = self.result.status,
            o_done   = self.done.status,
            o_busy   = self.busy.status,
            o_error  = self.error.status,
        )