# Pruebas y resultados

## 1. Calculadora RTL

Banco de prueba:

```text
testbench/calculadora/calc_core_tb.sv
```

Casos cubiertos:

| Operación | Entrada | Resultado esperado |
|---|---|---:|
| Suma | 12 + 5 | 17 |
| Resta | 12 - 5 | 7 |
| Multiplicación | 7 × 6 | 42 |
| División exacta | 100 / 4 | 25 |
| División entera | 7 / 2 | 3 |
| División entera | 10 / 3 | 3 |
| Máximo 16 bits | 65535 / 1 | 65535 |
| Raíz | √81 | 9 |
| Raíz | √144 | 12 |
| División por cero | 10 / 0 | `error=1` |

Comando:

```bash
iverilog -g2012 \
    -o testbench/calculadora/calc_core_tb.out \
    testbench/calculadora/calc_core_tb.sv \
    rtl/cores/calculadora/calc_core.sv \
    rtl/cores/calculadora/multiplicador.sv \
    rtl/cores/calculadora/divisor.sv \
    rtl/cores/calculadora/raiz.sv

vvp testbench/calculadora/calc_core_tb.out
```

## 2. I2C RTL

Banco de prueba:

```text
testbench/i2c/i2c_master_tb.sv
```

El testbench simula un esclavo que:

- detecta START;
- captura dirección+W;
- captura `data0`;
- captura `data1`;
- genera tres ACK;
- detecta STOP.

Caso principal:

```text
dirección = 0x3C
data0     = 0x00
data1     = 0xAE
```

Esto corresponde al comando SSD1306 `display OFF`.

Comando:

```bash
iverilog -g2012 \
    -o testbench/i2c/i2c_master_tb.out \
    testbench/i2c/i2c_master_tb.sv \
    rtl/cores/i2c/i2c_master.v

vvp testbench/i2c/i2c_master_tb.out
```

## 3. WS2812 RTL

Banco de prueba:

```text
testbench/ws2812/ws2812_matrix_tb.sv
```

Aspectos comprobados por el banco:

- escritura del framebuffer;
- activación de `busy`;
- finalización con `done`;
- envío de 64 × 24 bits;
- orden GRB;
- diferencia entre bit cero y bit uno;
- intervalo bajo de reset.

Comando:

```bash
iverilog -g2012 \
    -o testbench/ws2812/ws2812_matrix_tb.out \
    testbench/ws2812/ws2812_matrix_tb.sv \
    rtl/cores/ws2812/ws2812_matrix_core.v \
    rtl/cores/ws2812/ws2812_framebuffer.v

vvp testbench/ws2812/ws2812_matrix_tb.out
```

## 4. Verificaciones físicas realizadas

Durante la integración se verificaron:

- detección de la OLED en la dirección 0x3C;
- inicialización y escritura de texto SSD1306;
- colores, patrones e imagen de la WS2812;
- mapeo de filas y canales RGB de la HUB75;
- reproducción de imagen/GIF;
- operaciones consecutivas;
- resultados negativos;
- raíces consecutivas sin conservar un `done` anterior;
- división por cero;
- texto recibido por UART en OLED y HUB75;
- demo automática en ejecución continua.

Casos de calculadora observados en la placa:

```text
12+5   → 17
7*6    → 42
100/4  → 25
10/0   → error
11-12  → -1
R144   → 12
R81    → 9
R9     → 3
```


