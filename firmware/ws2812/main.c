#include <stdint.h>

#include <generated/csr.h>


// -----------------------------------------------------------------------------
// Utilidades
// -----------------------------------------------------------------------------

static void delay(unsigned int n)
{
    while (n--) {
        __asm__ volatile("nop");
    }
}


static void uart_putc_raw(char c)
{
    while (uart_txfull_read()) {
        delay(10);
    }

    uart_rxtx_write((uint32_t)c);
}


static void uart_puts_raw(const char *s)
{
    while (*s) {
        uart_putc_raw(*s++);
    }
}


// -----------------------------------------------------------------------------
// WS2812
// -----------------------------------------------------------------------------

static void ws2812_set_pixel(
    uint32_t pixel,
    uint32_t color
)
{
    ws2812_pixel_index_write(pixel);
    ws2812_pixel_color_write(color);

    /*
     * write_pixel está conectado a CSRStorage.re.
     *
     * Una escritura al CSR genera un pulso de un ciclo hacia
     * pixel_we en el núcleo RTL.
     */
    ws2812_write_pixel_write(1);
}


static void ws2812_clear(void)
{
    uint32_t pixel;

    for (pixel = 0; pixel < 64; pixel++) {
        ws2812_set_pixel(
            pixel,
            0x000000
        );
    }
}


static int ws2812_show(void)
{
    uint32_t timeout;

    /*
     * start está conectado a CSRStorage.re.
     *
     * Una única escritura genera el pulso START.
     */
    ws2812_start_write(1);


    /*
     * done es sticky en ws2812_matrix_core.
     *
     * Esperamos primero que una finalización anterior se limpie.
     */
    timeout = 1000000;

    while (
        ws2812_done_read()
        && timeout
    ) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }


    /*
     * Esperar la finalización de la transmisión completa.
     */
    timeout = 10000000;

    while (
        !ws2812_done_read()
        && timeout
    ) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    return 1;
}


// -----------------------------------------------------------------------------
// Programa principal
// -----------------------------------------------------------------------------

int main(void)
{
    uart_ev_pending_write(0xffffffff);

    uart_puts_raw(
        "\r\n"
        "WS2812 LiteX - Matriz 8x8\r\n"
        "Salida de datos: Colorlight J4\r\n"
        "\r\n"
    );


    while (1) {

        // ---------------------------------------------------------------------
        // Pixel 0 rojo tenue
        // ---------------------------------------------------------------------

        uart_puts_raw(
            "Pixel 0 -> ROJO\r\n"
        );

        ws2812_clear();

        ws2812_set_pixel(
            0,
            0x200000
        );

        if (!ws2812_show()) {
            uart_puts_raw(
                "ERROR: timeout WS2812\r\n"
            );
        }

        delay(20000000);


        // ---------------------------------------------------------------------
        // Pixel 1 verde tenue
        // ---------------------------------------------------------------------

        uart_puts_raw(
            "Pixel 1 -> VERDE\r\n"
        );

        ws2812_clear();

        ws2812_set_pixel(
            1,
            0x002000
        );

        if (!ws2812_show()) {
            uart_puts_raw(
                "ERROR: timeout WS2812\r\n"
            );
        }

        delay(20000000);
    }


    return 0;
}