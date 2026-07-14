#!/usr/bin/env python3

from migen import *

from litex.build.generic_platform import *
from litex.build.sim import SimPlatform
from litex.build.sim.config import SimConfig

from litex.soc.integration.soc_core import SoCCore
from litex.soc.integration.builder import Builder

from calculator import Calculator


# Recursos de simulación.
# OJO: este "serial" NO es tx/rx físico.
# Es el stream que usa uart_name="sim" junto con serial2tcp.
_io = [
    ("sys_clk", 0, Pins(1)),
    ("sys_rst", 0, Pins(1)),

    ("serial", 0,
        Subsignal("source_valid", Pins(1)),
        Subsignal("source_ready", Pins(1)),
        Subsignal("source_data",  Pins(8)),

        Subsignal("sink_valid", Pins(1)),
        Subsignal("sink_ready", Pins(1)),
        Subsignal("sink_data",  Pins(8)),
    ),
]


class _CRG(Module):
    def __init__(self, platform):
        self.rst = Signal()

        self.clock_domains.cd_sys = ClockDomain()

        self.comb += [
            self.cd_sys.clk.eq(platform.request("sys_clk")),
            self.cd_sys.rst.eq(platform.request("sys_rst") | self.rst),
        ]


class SimCalcSoC(SoCCore):
    def __init__(self, **kwargs):
        platform = SimPlatform("SIM", _io, toolchain="verilator")

        sys_clk_freq = int(1e6)  # 1 MHz para simulación

        self.submodules.crg = _CRG(platform)

        # UART de simulación.
        # NO cambiar a "serial", porque "serial" aquí no tiene tx/rx.
        # "sim" crea la UART compatible con serial2tcp.
        kwargs["uart_name"] = "sim"

        # Memorias de simulación.
        kwargs["integrated_rom_size"]      = 0x20000     # 128 KiB
        kwargs["integrated_sram_size"]     = 0x2000      # 8 KiB
        kwargs["integrated_main_ram_size"] = 0x200000    # 2 MiB

        SoCCore.__init__(
            self,
            platform,
            clk_freq=sys_clk_freq,
            ident="LiteX Calculator SoC Simulation",
            **kwargs
        )

        # Periférico de calculadora.
        self.submodules.calculator = Calculator(platform)
        self.csr.add("calculator")


def main():
    soc = SimCalcSoC()

    sim_config = SimConfig()

    # Reloj del sistema simulado.
    sim_config.add_clocker("sys_clk", freq_hz=int(1e6))

    # UART por TCP.
    # Se conecta con:
    # litex_term socket://127.0.0.1:1236
    sim_config.add_module("serial2tcp", "serial", args={"port": 1236})

    builder = Builder(
        soc,
        output_dir="build/sim_calc",
        csr_csv="build/sim_calc/csr.csv"
    )

    # run=False: este .py solo construye el simulador.
    # Luego corres manualmente build/sim_calc/gateware/obj_dir/Vsim.
    builder.build(
        sim_config=sim_config,
        run=False,
        trace=False
    )


if __name__ == "__main__":
    main()