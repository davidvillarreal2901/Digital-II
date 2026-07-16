/*
 * Demostración automática de todas laj funcionalidades.
 */

#define main main_interactivo_no_usado
#include "../demo_integrada/main.c"
#undef main


#define PAUSA_OPERACION_MS  2200u
#define PAUSA_TEXTO_MS      3200u
#define PAUSA_CORTA_MS       700u


static void copiar_comando(
    char *destino,
    const char *origen
)
{
    uint8_t indice;

    indice = 0;

    while (
        origen[indice] != '\0'
        && indice < (LINE_SIZE - 1u)
    ) {
        destino[indice] = origen[indice];
        indice++;
    }

    destino[indice] = '\0';
}


static void ejecutar_comando_demo(
    const char *comando,
    uint32_t pausa_ms
)
{
    char linea[LINE_SIZE];

    copiar_comando(
        linea,
        comando
    );

    uart_puts_raw("\r\n[DEMO] ");
    uart_puts_raw(linea);
    uart_puts_raw("\r\n");

    /*
     * Se usa el mismo parser y las mismas funciones
     * del firmware interactivo.
     */
    process_line(linea);

    hub75_ui_delay_ms(pausa_ms);
}


static void inicializar_demo(void)
{
    uart_ev_pending_write(0xffffffff);
    uart_drain_rx();

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " DEMO AUTOMATICA - DIGITAL II\r\n"
        " CALCULADORA + OLED + WS2812 + HUB75\r\n"
        "========================================\r\n"
    );

    oled_init();
    oled_show_home();

    matrix_clear();
    hub75_clear();

    hub75_ui_delay_ms(1000u);
}


int main(void)
{
    inicializar_demo();

    while (1) {

        /*
         * Presentación del proyecto.
         */
        ejecutar_comando_demo(
            "T DEMO DIGITAL II",
            PAUSA_TEXTO_MS
        );

        /*
         * OLED: menú de ayuda.
         * WS2812: imagen.
         * HUB75: animación.
         */
        ejecutar_comando_demo(
            "H",
            PAUSA_OPERACION_MS
        );

        /*
         * Suma.
         */
        ejecutar_comando_demo(
            "12+5",
            PAUSA_OPERACION_MS
        );

        /*
         * Resta con resultado negativo.
         */
        ejecutar_comando_demo(
            "11-12",
            PAUSA_OPERACION_MS
        );

        /*
         * Multiplicación.
         */
        ejecutar_comando_demo(
            "7*6",
            PAUSA_OPERACION_MS
        );

        /*
         * División.
         */
        ejecutar_comando_demo(
            "100/4",
            PAUSA_OPERACION_MS
        );

        /*
         * Raíces consecutivas.
         */
        ejecutar_comando_demo(
            "R144",
            PAUSA_OPERACION_MS
        );

        ejecutar_comando_demo(
            "R81",
            PAUSA_OPERACION_MS
        );

        ejecutar_comando_demo(
            "R9",
            PAUSA_OPERACION_MS
        );

        /*
         * Texto libre en OLED y HUB75.
         */
        ejecutar_comando_demo(
            "T FPGA LITEX",
            PAUSA_TEXTO_MS
        );

        /*
         * Limpiar antes de repetir.
         */
        ejecutar_comando_demo(
            "C",
            PAUSA_CORTA_MS
        );
    }

    return 0;
}