SHELL := /bin/bash
.DEFAULT_GOAL := ayuda


# =============================================================================
# CONFIGURACIÓN
# =============================================================================

PYTHON       ?= python
LITEX_TERM   ?= litex_term

SOC_TARGET   := integracion_litex/soc/colorlight_i5.py
BOARD        := i9
REVISION     := 7.2

BUILD_DIR    := build/colorlight_i5
BITSTREAM    := $(BUILD_DIR)/gateware/colorlight_i5.bit
CSR_FILE     := $(BUILD_DIR)/csr.csv

APP          ?= demo_integrada
FIRMWARE_DIR := firmware/$(APP)
FIRMWARE_BIN := $(FIRMWARE_DIR)/firmware.bin

PORT         ?= $(firstword $(wildcard /dev/cu.usbmodem*))


# =============================================================================
# RECURSOS GRÁFICOS
# =============================================================================

IMAGEN_WS2812   ?= recursos/ws2812/kirby.png
RECURSO_HUB75   ?= recursos/hub75/tux.gif

BRILLO_WS2812   ?= 32

FRAMES_HUB75    ?= 8
CADA_HUB75      ?= 1
RETARDO_HUB75   ?= 100
FILTRO_HUB75    ?= --nearest

HEADER_WS2812   := firmware/demo_integrada/imagen_ws2812.h
HEADER_HUB75    := firmware/demo_integrada/hub75_asset.h

SCRIPT_WS2812   := herramientas/ws2812/imagen_a_c.py
SCRIPT_HUB75    := herramientas/hub75/recurso_a_c.py


# =============================================================================
# OBJETIVOS
# =============================================================================

.PHONY: ayuda
.PHONY: sintetizar
.PHONY: cargar
.PHONY: compilar
.PHONY: ejecutar
.PHONY: interactiva
.PHONY: automatica
.PHONY: todo-interactiva
.PHONY: todo-automatica
.PHONY: puerto
.PHONY: estado
.PHONY: csr
.PHONY: limpiar
.PHONY: limpiar-todo
.PHONY: listar-recursos
.PHONY: recurso-ws2812
.PHONY: recurso-hub75
.PHONY: recursos
.PHONY: interactiva-recursos
.PHONY: automatica-recursos


# =============================================================================
# AYUDA
# =============================================================================

ayuda:
	@echo ""
	@echo "===================================================="
	@echo " DIGITAL II - COLORLIGHT I9"
	@echo "===================================================="
	@echo ""
	@echo " make sintetizar"
	@echo "     Sintetiza el SoC completo."
	@echo ""
	@echo " make cargar"
	@echo "     Carga temporalmente el bitstream en la FPGA."
	@echo ""
	@echo " make interactiva"
	@echo "     Compila y ejecuta demo_integrada por UART."
	@echo ""
	@echo " make automatica"
	@echo "     Compila y ejecuta demo_automatica por UART."
	@echo ""
	@echo " make todo-interactiva"
	@echo "     Sintetiza, carga la FPGA y ejecuta la demo interactiva."
	@echo ""
	@echo " make todo-automatica"
	@echo "     Sintetiza, carga la FPGA y ejecuta la demo automática."
	@echo ""
	@echo " make compilar APP=hub75_diag"
	@echo "     Compila cualquier firmware existente."
	@echo ""
	@echo " make ejecutar APP=hub75_diag"
	@echo "     Compila y ejecuta cualquier firmware por UART."
	@echo ""
	@echo " make puerto"
	@echo "     Muestra el puerto serie detectado."
	@echo ""
	@echo " make estado"
	@echo "     Muestra el estado general del proyecto."
	@echo ""
	@echo " make csr"
	@echo "     Muestra los CSR de los periféricos integrados."
	@echo ""
	@echo " make listar-recursos"
	@echo "     Muestra las imágenes y GIF disponibles."
	@echo ""
	@echo " make recurso-ws2812 IMAGEN_WS2812=ruta/imagen.png"
	@echo "     Convierte una imagen para la matriz WS2812."
	@echo ""
	@echo " make recurso-hub75 RECURSO_HUB75=ruta/imagen_o_gif"
	@echo "     Convierte una imagen o GIF para el panel HUB75."
	@echo ""
	@echo " make recursos IMAGEN_WS2812=... RECURSO_HUB75=..."
	@echo "     Genera los recursos de ambas pantallas."
	@echo ""
	@echo " make interactiva-recursos IMAGEN_WS2812=... RECURSO_HUB75=..."
	@echo "     Genera recursos y ejecuta la demo interactiva."
	@echo ""
	@echo " make automatica-recursos IMAGEN_WS2812=... RECURSO_HUB75=..."
	@echo "     Genera recursos y ejecuta la demo automática."
	@echo ""
	@echo ""
	@echo " make limpiar"
	@echo "     Limpia los firmwares principales."
	@echo ""
	@echo " make limpiar-todo"
	@echo "     Limpia firmwares y síntesis del SoC."
	@echo ""
	@echo "Para indicar manualmente el puerto:"
	@echo ""
	@echo " make interactiva PORT=/dev/cu.usbmodem102"
	@echo ""


# =============================================================================
# SÍNTESIS DEL SOC
# =============================================================================

sintetizar:
	@echo ""
	@echo "===================================================="
	@echo " SINTETIZANDO SOC COMPLETO"
	@echo "===================================================="
	@echo ""

	rm -rf "$(BUILD_DIR)"

	$(PYTHON) "$(SOC_TARGET)" \
		--board "$(BOARD)" \
		--revision "$(REVISION)" \
		--build

	@test -f "$(BITSTREAM)" || { \
		echo ""; \
		echo "ERROR: no se generó el bitstream:"; \
		echo "$(BITSTREAM)"; \
		exit 1; \
	}

	@echo ""
	@echo "Síntesis terminada:"
	@ls -lh "$(BITSTREAM)"
	@echo ""


# =============================================================================
# CARGA TEMPORAL DE LA FPGA
# =============================================================================

cargar:
	@test -f "$(BITSTREAM)" || { \
		echo ""; \
		echo "ERROR: no existe el bitstream:"; \
		echo "$(BITSTREAM)"; \
		echo ""; \
		echo "Ejecuta primero:"; \
		echo "make sintetizar"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " CARGANDO BITSTREAM TEMPORALMENTE"
	@echo "===================================================="
	@echo ""

	$(PYTHON) "$(SOC_TARGET)" \
		--board "$(BOARD)" \
		--revision "$(REVISION)" \
		--load

	@echo ""
	@echo "Bitstream cargado en la FPGA."
	@echo ""


# =============================================================================
# COMPILACIÓN DE FIRMWARE
# =============================================================================

compilar:
	@test -d "$(FIRMWARE_DIR)" || { \
		echo ""; \
		echo "ERROR: no existe el firmware:"; \
		echo "$(FIRMWARE_DIR)"; \
		exit 1; \
	}

	@test -f "$(FIRMWARE_DIR)/Makefile" || { \
		echo ""; \
		echo "ERROR: no existe:"; \
		echo "$(FIRMWARE_DIR)/Makefile"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " COMPILANDO FIRMWARE: $(APP)"
	@echo "===================================================="
	@echo ""

	$(MAKE) -C "$(FIRMWARE_DIR)" clean
	$(MAKE) -C "$(FIRMWARE_DIR)"

	@test -f "$(FIRMWARE_BIN)" || { \
		echo ""; \
		echo "ERROR: no se generó:"; \
		echo "$(FIRMWARE_BIN)"; \
		exit 1; \
	}

	@echo ""
	@echo "Firmware generado:"
	@ls -lh "$(FIRMWARE_BIN)"
	@echo ""


# =============================================================================
# EJECUCIÓN POR UART
# =============================================================================

ejecutar: compilar
	@test -n "$(PORT)" || { \
		echo ""; \
		echo "ERROR: no se encontró ningún puerto /dev/cu.usbmodem*"; \
		echo ""; \
		echo "Revisa la conexión USB o especifica el puerto:"; \
		echo "make ejecutar APP=$(APP) PORT=/dev/cu.usbmodem102"; \
		exit 1; \
	}

	@test -e "$(PORT)" || { \
		echo ""; \
		echo "ERROR: el puerto no existe:"; \
		echo "$(PORT)"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " EJECUTANDO FIRMWARE: $(APP)"
	@echo "===================================================="
	@echo ""
	@echo "Puerto:   $(PORT)"
	@echo "Firmware: $(FIRMWARE_BIN)"
	@echo ""

	$(LITEX_TERM) "$(PORT)" \
		--kernel "$(FIRMWARE_BIN)" \
		--safe


# =============================================================================
# DEMOSTRACIONES
# =============================================================================

interactiva:
	$(MAKE) ejecutar \
		APP=demo_integrada \
		PORT="$(PORT)"


automatica:
	$(MAKE) ejecutar \
		APP=demo_automatica \
		PORT="$(PORT)"


todo-interactiva: sintetizar cargar
	$(MAKE) interactiva \
		PORT="$(PORT)"


todo-automatica: sintetizar cargar
	$(MAKE) automatica \
		PORT="$(PORT)"



# =============================================================================
# RECURSOS GRÁFICOS
# =============================================================================

listar-recursos:
	@echo ""
	@echo "===================================================="
	@echo " RECURSOS DISPONIBLES"
	@echo "===================================================="
	@echo ""
	@echo "WS2812:"
	@if [ -d "recursos/ws2812" ]; then \
		find recursos/ws2812 \
			-maxdepth 1 \
			-type f \
			| sort; \
	else \
		echo "No existe recursos/ws2812"; \
	fi
	@echo ""
	@echo "HUB75:"
	@if [ -d "recursos/hub75" ]; then \
		find recursos/hub75 \
			-maxdepth 1 \
			-type f \
			| sort; \
	else \
		echo "No existe recursos/hub75"; \
	fi
	@echo ""


recurso-ws2812:
	@test -f "$(SCRIPT_WS2812)" || { \
		echo ""; \
		echo "ERROR: no existe el conversor:"; \
		echo "$(SCRIPT_WS2812)"; \
		exit 1; \
	}

	@test -f "$(IMAGEN_WS2812)" || { \
		echo ""; \
		echo "ERROR: no existe la imagen:"; \
		echo "$(IMAGEN_WS2812)"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " GENERANDO IMAGEN PARA WS2812"
	@echo "===================================================="
	@echo ""
	@echo "Entrada: $(IMAGEN_WS2812)"
	@echo "Salida:  $(HEADER_WS2812)"
	@echo "Brillo:  $(BRILLO_WS2812)"
	@echo ""

	$(PYTHON) "$(SCRIPT_WS2812)" \
		"$(IMAGEN_WS2812)" \
		"$(HEADER_WS2812)" \
		--brillo "$(BRILLO_WS2812)"

	@test -f "$(HEADER_WS2812)" || { \
		echo "ERROR: no se generó $(HEADER_WS2812)"; \
		exit 1; \
	}

	@echo ""
	@echo "Recurso WS2812 generado:"
	@ls -lh "$(HEADER_WS2812)"
	@echo ""


recurso-hub75:
	@test -f "$(SCRIPT_HUB75)" || { \
		echo ""; \
		echo "ERROR: no existe el conversor:"; \
		echo "$(SCRIPT_HUB75)"; \
		exit 1; \
	}

	@test -f "$(RECURSO_HUB75)" || { \
		echo ""; \
		echo "ERROR: no existe el recurso:"; \
		echo "$(RECURSO_HUB75)"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " GENERANDO RECURSO PARA HUB75"
	@echo "===================================================="
	@echo ""
	@echo "Entrada:        $(RECURSO_HUB75)"
	@echo "Salida:         $(HEADER_HUB75)"
	@echo "Máximo frames:  $(FRAMES_HUB75)"
	@echo "Tomar cada:     $(CADA_HUB75)"
	@echo "Retardo base:   $(RETARDO_HUB75) ms"
	@echo ""

	$(PYTHON) "$(SCRIPT_HUB75)" \
		"$(RECURSO_HUB75)" \
		"$(HEADER_HUB75)" \
		--max-frames "$(FRAMES_HUB75)" \
		--cada "$(CADA_HUB75)" \
		--delay "$(RETARDO_HUB75)" \
		$(FILTRO_HUB75)

	@test -f "$(HEADER_HUB75)" || { \
		echo "ERROR: no se generó $(HEADER_HUB75)"; \
		exit 1; \
	}

	@echo ""
	@echo "Recurso HUB75 generado:"
	@ls -lh "$(HEADER_HUB75)"
	@echo ""


recursos: recurso-ws2812 recurso-hub75
	@echo ""
	@echo "Recursos de WS2812 y HUB75 generados correctamente."
	@echo ""


interactiva-recursos: recursos
	$(MAKE) interactiva PORT="$(PORT)"


automatica-recursos: recursos
	$(MAKE) automatica PORT="$(PORT)"
# =============================================================================
# INFORMACIÓN DEL PROYECTO
# =============================================================================

puerto:
	@echo ""
	@if [ -n "$(PORT)" ]; then \
		echo "Puerto detectado: $(PORT)"; \
	else \
		echo "No se encontró ningún puerto /dev/cu.usbmodem*"; \
		exit 1; \
	fi
	@echo ""


estado:
	@echo ""
	@echo "===================================================="
	@echo " ESTADO DEL PROYECTO"
	@echo "===================================================="
	@echo ""
	@echo "Rama Git:"
	@git branch --show-current 2>/dev/null || true
	@echo ""
	@echo "Último commit:"
	@git log -1 --oneline 2>/dev/null || true
	@echo ""
	@echo "Puerto detectado:"
	@if [ -n "$(PORT)" ]; then \
		echo "$(PORT)"; \
	else \
		echo "No detectado"; \
	fi
	@echo ""
	@echo "Bitstream:"
	@if [ -f "$(BITSTREAM)" ]; then \
		ls -lh "$(BITSTREAM)"; \
	else \
		echo "No generado"; \
	fi
	@echo ""
	@echo "Firmware interactivo:"
	@if [ -f "firmware/demo_integrada/firmware.bin" ]; then \
		ls -lh "firmware/demo_integrada/firmware.bin"; \
	else \
		echo "No generado"; \
	fi
	@echo ""
	@echo "Firmware automático:"
	@if [ -f "firmware/demo_automatica/firmware.bin" ]; then \
		ls -lh "firmware/demo_automatica/firmware.bin"; \
	else \
		echo "No generado"; \
	fi
	@echo ""


csr:
	@test -f "$(CSR_FILE)" || { \
		echo ""; \
		echo "ERROR: no existe:"; \
		echo "$(CSR_FILE)"; \
		echo ""; \
		echo "Ejecuta primero:"; \
		echo "make sintetizar"; \
		exit 1; \
	}

	@echo ""
	@echo "===================================================="
	@echo " CSR DE LOS PERIFÉRICOS"
	@echo "===================================================="
	@echo ""

	@grep -E \
		"calculator|i2c_oled|ws2812|hub75" \
		"$(CSR_FILE)" || true

	@echo ""


# =============================================================================
# LIMPIEZA
# =============================================================================

limpiar:
	@echo ""
	@echo "Limpiando firmware interactivo..."

	@if [ -d "firmware/demo_integrada" ]; then \
		$(MAKE) -C firmware/demo_integrada clean; \
	fi

	@echo ""
	@echo "Limpiando firmware automático..."

	@if [ -d "firmware/demo_automatica" ]; then \
		$(MAKE) -C firmware/demo_automatica clean; \
	fi

	@echo ""
	@echo "Limpieza de firmwares terminada."
	@echo ""


limpiar-todo: limpiar
	@echo "Eliminando síntesis del SoC..."

	rm -rf "$(BUILD_DIR)"

	@echo ""
	@echo "Limpieza completa terminada."
	@echo ""