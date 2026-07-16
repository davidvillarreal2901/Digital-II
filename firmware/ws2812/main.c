#include <stdint.h>

#include <generated/csr.h>
#include "imagen_ws2812.h" //Para la imagen de prueba del WS2812


// -----------------------------------------------------------------------------
// Configuracion
// -----------------------------------------------------------------------------

#define WS2812_NUM_PIXELS 64
#define WS2812_WIDTH       8
#define WS2812_HEIGHT      8


// -----------------------------------------------------------------------------
// Colores RGB
// -----------------------------------------------------------------------------

#define COLOR_OFF      0x000000

#define COLOR_RED      0x200000
#define COLOR_GREEN    0x002000
#define COLOR_BLUE     0x000020

#define COLOR_YELLOW   0x202000
#define COLOR_CYAN     0x002020
#define COLOR_MAGENTA  0x200020

#define COLOR_WHITE    0x101010


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
     * write_pixel esta conectado a CSRStorage.re.
     *
     * Cada escritura genera un pulso de un ciclo.
     */
    ws2812_write_pixel_write(1);
}


static void ws2812_clear(void)
{
    uint32_t pixel;

    for (pixel = 0; pixel < WS2812_NUM_PIXELS; pixel++) {
        ws2812_set_pixel(
            pixel,
            COLOR_OFF
        );
    }
}


static void ws2812_fill(uint32_t color)
{
    uint32_t pixel;

    for (pixel = 0; pixel < WS2812_NUM_PIXELS; pixel++) {
        ws2812_set_pixel(
            pixel,
            color
        );
    }
}


static int ws2812_show(void)
{
    uint32_t timeout;

    /*
     * Una escritura al CSR START genera el pulso de inicio.
     */
    ws2812_start_write(1);


    /*
     * Esperar que DONE anterior se limpie.
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
     * Esperar finalizacion de la transmision.
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
// Coordenadas 8x8
//
// De momento se asume:
//
// pixel = y * 8 + x
//
// La prueba del pixel movil permitira determinar si la matriz fisica usa
// orden lineal o serpentino.
// Al final fue lineal 
// -----------------------------------------------------------------------------

static uint32_t ws2812_xy_to_index(
    uint32_t x,
    uint32_t y
)
{
    return (
        y * WS2812_WIDTH
        + x
    );
}


// -----------------------------------------------------------------------------
// Patrones
// -----------------------------------------------------------------------------

static void test_fill_colors(void)
{
    uart_puts_raw(
        "[TEST] Matriz completa ROJA\r\n"
    );

    ws2812_fill(COLOR_RED);
    ws2812_show();

    delay(15000000);


    uart_puts_raw(
        "[TEST] Matriz completa VERDE\r\n"
    );

    ws2812_fill(COLOR_GREEN);
    ws2812_show();

    delay(15000000);


    uart_puts_raw(
        "[TEST] Matriz completa AZUL\r\n"
    );

    ws2812_fill(COLOR_BLUE);
    ws2812_show();

    delay(15000000);


    uart_puts_raw(
        "[TEST] Matriz completa BLANCA\r\n"
    );

    ws2812_fill(COLOR_WHITE);
    ws2812_show();

    delay(15000000);


    ws2812_clear();
    ws2812_show();
}


static void test_moving_pixel(void)
{
    uint32_t pixel;

    uart_puts_raw(
        "[TEST] Pixel movil 0 -> 63\r\n"
    );

    for (pixel = 0; pixel < WS2812_NUM_PIXELS; pixel++) {

        ws2812_clear();

        ws2812_set_pixel(
            pixel,
            COLOR_CYAN
        );

        ws2812_show();

        delay(2500000);
    }


    ws2812_clear();
    ws2812_show();
}


static void test_cross(void)
{
    uint32_t x;
    uint32_t y;
    uint32_t pixel;

    uart_puts_raw(
        "[TEST] Cruz amarilla\r\n"
    );

    ws2812_clear();


    /*
     * Linea horizontal.
     */
    y = 3;

    for (x = 0; x < WS2812_WIDTH; x++) {

        pixel = ws2812_xy_to_index(
            x,
            y
        );

        ws2812_set_pixel(
            pixel,
            COLOR_YELLOW
        );
    }


    /*
     * Linea vertical.
     */
    x = 3;

    for (y = 0; y < WS2812_HEIGHT; y++) {

        pixel = ws2812_xy_to_index(
            x,
            y
        );

        ws2812_set_pixel(
            pixel,
            COLOR_YELLOW
        );
    }


    ws2812_show();

    delay(20000000);


    ws2812_clear();
    ws2812_show();
}


static void test_x(void)
{
    uint32_t y;
    uint32_t pixel_a;
    uint32_t pixel_b;

    uart_puts_raw(
        "[TEST] X magenta\r\n"
    );

    ws2812_clear();


    for (y = 0; y < WS2812_HEIGHT; y++) {

        pixel_a = ws2812_xy_to_index(
            y,
            y
        );

        pixel_b = ws2812_xy_to_index(
            WS2812_WIDTH - 1 - y,
            y
        );


        ws2812_set_pixel(
            pixel_a,
            COLOR_MAGENTA
        );

        ws2812_set_pixel(
            pixel_b,
            COLOR_MAGENTA
        );
    }


    ws2812_show();

    delay(20000000);


    ws2812_clear();
    ws2812_show();
}

static void test_image(void)
{
    uint32_t pixel;

    uart_puts_raw(
        "[TEST] Imagen RGB 8x8\r\n"
    );


    for (
        pixel = 0;
        pixel < IMAGEN_WS2812_PIXELS;
        pixel++
    ) {
        ws2812_set_pixel(
            pixel,
            imagen_ws2812[pixel]
        );
    }


    if (!ws2812_show()) {
        uart_puts_raw(
            "ERROR: timeout mostrando imagen\r\n"
        );

        return;
    }


    delay(40000000);


    ws2812_clear();
    ws2812_show();
}


// -----------------------------------------------------------------------------
// Programa principal
// -----------------------------------------------------------------------------

int main(void)
{
    uart_ev_pending_write(0xffffffff);


    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " WS2812 LiteX - Matriz 8x8\r\n"
        " Colorlight i9\r\n"
        " Salida: J4\r\n"
        "========================================\r\n"
        "\r\n"
    );


    ws2812_clear();

    if (!ws2812_show()) {
        uart_puts_raw(
            "ERROR: timeout WS2812 inicial\r\n"
        );
    }


    while (1) {

        uart_puts_raw(
            "\r\n"
            "========================================\r\n"
            " INICIO SECUENCIA WS2812\r\n"
            "========================================\r\n"
        );


        test_fill_colors();

        delay(5000000);


        test_moving_pixel();

        delay(5000000);


        test_cross();

        delay(5000000);


        test_x();

        delay(5000000);

        test_image();

        delay(5000000);
    }


    return 0;
}