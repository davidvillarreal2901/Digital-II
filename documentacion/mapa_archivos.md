# Mapa de archivos

Este documento explica el propósito de los archivos relevantes del repositorio. Los recursos gráficos individuales no se enumeran uno por uno porque todos cumplen la misma función: servir de entrada a los conversores.

## Raíz

| Archivo o carpeta | Propósito |
|-|-
| `Makefile` | Automatiza síntesis, carga, firmware y conversión de recursos. |
| `.gitignore` | Evita versionar productos de compilación comunes. |
| `rtl/` | Núcleos de hardware. |
| `integracion_litex/` | Wrappers CSR y SoC superior. |
| `firmware/` | Programas ejecutados por el procesador. |
| `testbench/` | Pruebas directas de los núcleos RTL. |
| `sim/` | Construcción de un SoC de simulación. |
| `herramientas/` | Conversión de recursos gráficos a encabezados C. |
| `recursos/` | PNG, JPG y GIF de entrada. |
| `documentacion/` | Explicaciones y diagramas. |

## Integración LiteX

### `integracion_litex/soc/colorlight_i5.py`

Archivo superior. Define pines, reloj, reset, procesador, memorias y periféricos. También recibe `--build` y `--load`.

### `integracion_litex/perifericos/calculadora.py`

Crea los CSR de la calculadora, agrega los cuatro archivos SystemVerilog e instancia `calc_core`.

### `integracion_litex/perifericos/ws2812.py`

Crea los CSR del framebuffer WS2812 e instancia `ws2812_matrix_core`.

### `integracion_litex/perifericos/i2c.py`

Crea los CSR del maestro I2C. Convierte las señales `oen` del RTL en líneas físicas open-drain.

### `integracion_litex/perifericos/hub75.py`

Crea los CSR de escritura del framebuffer y el control `blank`. Instancia `hub75_core` y conecta todas las señales HUB75.

## RTL de calculadora

### `rtl/cores/calculadora/calc_core.sv`

Controlador superior. Decodifica el opcode, resuelve suma/resta e inicia multiplicador, divisor o raíz. Entrega `result`, `busy`, `done` y `error`.

### `rtl/cores/calculadora/multiplicador.sv`

Multiplicador iterativo shift-and-add de 16 × 16 bits. Examina un bit del segundo operando por ciclo y acumula el primer operando desplazado.

### `rtl/cores/calculadora/divisor.sv`

Divisor entero iterativo de 16 bits mediante división restauradora. El cociente queda en los 16 bits inferiores de `reg_a`.

### `rtl/cores/calculadora/raiz.sv`

Raíz cuadrada entera sin multiplicadores. Evalúa pares de bits desde el más significativo hacia el menos significativo.

## RTL de WS2812

### `rtl/cores/ws2812/ws2812_matrix_core.v`

Contiene el framebuffer de 64 colores RGB. Acepta escrituras del CPU y conecta la memoria con el transmisor.

### `rtl/cores/ws2812/ws2812_framebuffer.v`

Recorre los 64 píxeles, convierte RGB a GRB y genera los tiempos alto/bajo de cada bit WS2812. Al final genera el tiempo de reset/latch.

## RTL de I2C

### `rtl/cores/i2c/i2c_master.v`

Máquina de estados para una escritura I2C de tres bytes en el bus: dirección+W, `data0` y `data1`. Controla START, bits, ACK y STOP.

## RTL de HUB75

### `rtl/cores/hub75/hub75_core.v`

Integra reloj de panel, RAM, contadores, selector de plano de bits y FSM de barrido.

### `rtl/cores/hub75/ctrl_lp4k.v`

FSM que organiza desplazamiento de columnas, latch, tiempo visible, cambio de plano y cambio de fila.

### `rtl/cores/hub75/ram_dual.v`

RAM de doble puerto de 4096 × 24 bits. El diseño usa 2048 direcciones.

### `rtl/cores/hub75/count.v`

Contador genérico usado para fila, columna, tiempo y plano de bits. Su entrada `reset` se comporta como habilitación activa en alto: cuando vale cero borra el contador.

### `rtl/cores/hub75/mux_led.v`

Selecciona uno de los cuatro bits de cada canal RGB444 y genera las seis líneas R1/G1/B1/R2/G2/B2.

### `rtl/cores/hub75/lsr_led.v`

Registro de desplazamiento que carga un tiempo base y lo duplica en cada plano: 1, 2, 4 y 8 veces.

### `rtl/cores/hub75/comp.v`

Comparador de igualdad usado para detectar que terminó el tiempo visible de un plano.

## Firmware principal

### `firmware/demo_integrada/main.c`

Aplicación interactiva. Incluye:

- UART y filtro de caracteres repetidos;
- parser de comandos;
- acceso CSR a la calculadora;
- inicialización y dibujo SSD1306;
- coordinación de WS2812 y HUB75;
- presentación de errores y resultados.

### `firmware/demo_integrada/ws2812_ui.h`

Rutinas de interfaz para la matriz 8 × 8: escritura, limpieza, símbolos de operadores, píxel móvil e imagen de ayuda.

### `firmware/demo_integrada/imagen_ws2812.h`

Header generado por `imagen_a_c.py`. Contiene 64 valores RGB de 24 bits.

### `firmware/demo_integrada/hub75_ui.h`

Rutinas de dibujo del panel: escritura RGB444, limpieza, reproducción de frames, fuente 5 × 7, operaciones, mensajes y errores.

### `firmware/demo_integrada/hub75_asset.h`

Header generado por `recurso_a_c.py`. Contiene frames RGB444 y retardos.

### `firmware/demo_automatica/main.c`

Renombra el `main` interactivo, incluye su implementación y reutiliza `process_line`. Ejecuta una lista fija de comandos en un bucle.

## Archivos comunes de firmware

Todos los directorios de firmware contienen:

| Archivo | Función |
|---|---|
| `Makefile` | Compila con las variables generadas por LiteX, enlaza y produce `firmware.bin`. |
| `isr.c` | Define una rutina de interrupción vacía para satisfacer el entorno de LiteX. |
| `linker.ld` | Ubica `.text`, `.rodata`, `.data`, `.bss` y la pila en `main_ram`. |

## Firmwares de prueba

| Directorio | Propósito |
|---|---|
| `firmware/calculadora/` | Consola UART para probar únicamente la calculadora. |
| `firmware/ws2812/` | Colores sólidos, píxel móvil, figuras e imagen 8 × 8. |
| `firmware/i2c_oled/` | Etapa previa de integración de calculadora, OLED y WS2812. |
| `firmware/hub75_diag/` | Diagnóstico de filas, colores y mapeo del panel. |
| `firmware/hub75_demo/` | Presentación de una imagen o animación HUB75. |
| `firmware/uart_test/` | Diagnóstico de la recepción UART y caracteres repetidos. |

Estos firmwares son útiles para aislar fallos, tonces los dejé por si las demos no funcionan bien.Pero es la misma lógica implementada en `demo_integrada` o `demo_automatica` pero individuales casi.

## Testbench

### `testbench/calculadora/calc_core_tb.sv`

Genera reloj/reset, ejecuta operaciones y compara resultado/error. Incluye suma, resta, multiplicación, divisiones, raíces y división por cero.

### `testbench/i2c/i2c_master_tb.sv`

Modela un esclavo, captura los tres bytes, genera ACK y comprueba START, STOP, `busy`, `done` y ausencia de error.

### `testbench/ws2812/ws2812_matrix_tb.sv`

Escribe colores al framebuffer, inicia la transmisión y mide número de bits, tiempos de pulsos, orden GRB y reset final.

Los archivos `.out` son ejecutables generados por Icarus Verilog; no contienen el código fuente de las pruebas.

## Simulación LiteX

### `sim/calculadora/sim_calc_litex.py`

Construye un SoC de simulación con UART TCP. Es un flujo anterior y requiere revisar su importación: actualmente intenta importar `calculator`, mientras el wrapper del repositorio se llama `calculadora.py`.No le ponga tanta atención profe, que era una prueba que leí por ahí pero no está bien hecha, la quiero trabjar a futuro.

## Herramientas

### `herramientas/ws2812/imagen_a_c.py`

- abre una imagen con Pillow;
- la convierte a RGB;
- la redimensiona a 8 × 8 por vecino más cercano;
- limita el brillo;
- genera `imagen_ws2812.h`.

### `herramientas/hub75/recurso_a_c.py`

- extrae frames de una imagen o GIF;
- redimensiona a 64 × 64;
- permite seleccionar uno de cada N frames;
- reduce RGB888 a RGB444;
- empaca píxeles superiores e inferiores;
- genera `hub75_asset.h`.


