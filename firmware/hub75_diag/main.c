#include <stdint.h>

#include <generated/csr.h>


#define HUB75_WIDTH       64u
#define HUB75_SCAN_ROWS   32u
#define HUB75_WORD_COUNT  2048u

#define COLOR_BLACK       0x000u
#define COLOR_RED         0xF00u
#define COLOR_GREEN       0x0F0u
#define COLOR_BLUE        0x00Fu
#define COLOR_YELLOW      0xFF0u
#define COLOR_CYAN        0x0FFu
#define COLOR_MAGENTA     0xF0Fu
#define COLOR_WHITE       0xFFFu


static void delay_cycles(uint32_t cycles)
{
    while (cycles--) {
        __asm__ volatile("nop");
    }
}


static void delay_ms(uint32_t milliseconds)
{
    uint32_t index;

    while (milliseconds--) {
        for (index = 0; index < 12000u; index++) {
            __asm__ volatile("nop");
        }
    }
}


static void uart_putc_raw(char character)
{
    while (uart_txfull_read()) {
        delay_cycles(10);
    }

    uart_rxtx_write((uint32_t)character);
}


static void uart_puts_raw(const char *text)
{
    while (*text) {
        uart_putc_raw(*text++);
    }
}


static uint32_t pack_pixels(
    uint16_t top_color,
    uint16_t bottom_color
)
{
    return (
        ((uint32_t)(top_color & 0x0FFFu) << 12)
        | ((uint32_t)bottom_color & 0x0FFFu)
    );
}


static void hub75_write_word(
    uint16_t address,
    uint32_t data
)
{
    hub75_pixel_addr_write(address);
    hub75_pixel_data_write(data & 0x00FFFFFFu);
    hub75_write_pixel_write(1);
}


static void hub75_fill_pair(
    uint16_t top_color,
    uint16_t bottom_color
)
{
    uint16_t address;
    uint32_t packed;

    packed = pack_pixels(
        top_color,
        bottom_color
    );

    hub75_blank_write(1);

    for (
        address = 0;
        address < HUB75_WORD_COUNT;
        address++
    ) {
        hub75_write_word(
            address,
            packed
        );
    }

    hub75_blank_write(0);
}


static void hub75_clear(void)
{
    hub75_fill_pair(
        COLOR_BLACK,
        COLOR_BLACK
    );
}


/*
 * Prueba individual de una línea de dirección.
 *
 * bit 0 -> A
 * bit 1 -> B
 * bit 2 -> C
 * bit 3 -> D
 * bit 4 -> E
 *
 * Las filas cuyo bit esté en 1 se iluminan.
 */
static void hub75_test_row_bit(
    uint8_t bit,
    uint16_t color
)
{
    uint8_t row;
    uint8_t column;

    uint16_t address;
    uint16_t selected_color;
    uint32_t packed;

    hub75_blank_write(1);

    for (
        row = 0;
        row < HUB75_SCAN_ROWS;
        row++
    ) {
        if (row & (1u << bit)) {
            selected_color = color;
        } else {
            selected_color = COLOR_BLACK;
        }

        packed = pack_pixels(
            selected_color,
            selected_color
        );

        for (
            column = 0;
            column < HUB75_WIDTH;
            column++
        ) {
            address = (uint16_t)(
                (row * HUB75_WIDTH)
                + column
            );

            hub75_write_word(
                address,
                packed
            );
        }
    }

    hub75_blank_write(0);
}


/*
 * Cada pareja de filas recibe un color según row % 8.
 * Sirve para observar si el direccionamiento se repite.
 */
static void hub75_row_rainbow(void)
{
    static const uint16_t colors[8] = {
        COLOR_RED,
        COLOR_GREEN,
        COLOR_BLUE,
        COLOR_YELLOW,
        COLOR_CYAN,
        COLOR_MAGENTA,
        COLOR_WHITE,
        COLOR_BLACK,
    };

    uint8_t row;
    uint8_t column;

    uint16_t address;
    uint16_t color;
    uint32_t packed;

    hub75_blank_write(1);

    for (
        row = 0;
        row < HUB75_SCAN_ROWS;
        row++
    ) {
        color = colors[row & 0x07u];

        packed = pack_pixels(
            color,
            color
        );

        for (
            column = 0;
            column < HUB75_WIDTH;
            column++
        ) {
            address = (uint16_t)(
                (row * HUB75_WIDTH)
                + column
            );

            hub75_write_word(
                address,
                packed
            );
        }
    }

    hub75_blank_write(0);
}


int main(void)
{
    uart_ev_pending_write(0xffffffff);

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " HUB75 DIAGNOSTICO\r\n"
        " Colores solidos y lineas A/B/C/D/E\r\n"
        "========================================\r\n"
        "\r\n"
    );

    hub75_blank_write(1);
    hub75_clear();

    while (1) {

        uart_puts_raw("SOLIDO ROJO\r\n");
        hub75_fill_pair(
            COLOR_RED,
            COLOR_RED
        );
        delay_ms(2500);


        uart_puts_raw("SOLIDO VERDE\r\n");
        hub75_fill_pair(
            COLOR_GREEN,
            COLOR_GREEN
        );
        delay_ms(2500);


        uart_puts_raw("SOLIDO AZUL\r\n");
        hub75_fill_pair(
            COLOR_BLUE,
            COLOR_BLUE
        );
        delay_ms(2500);


        uart_puts_raw(
            "SUPERIOR ROJO / INFERIOR AZUL\r\n"
        );
        hub75_fill_pair(
            COLOR_RED,
            COLOR_BLUE
        );
        delay_ms(3000);


        uart_puts_raw("DIRECCION A - BIT 0\r\n");
        hub75_test_row_bit(
            0,
            COLOR_RED
        );
        delay_ms(3000);


        uart_puts_raw("DIRECCION B - BIT 1\r\n");
        hub75_test_row_bit(
            1,
            COLOR_GREEN
        );
        delay_ms(3000);


        uart_puts_raw("DIRECCION C - BIT 2\r\n");
        hub75_test_row_bit(
            2,
            COLOR_BLUE
        );
        delay_ms(3000);


        uart_puts_raw("DIRECCION D - BIT 3\r\n");
        hub75_test_row_bit(
            3,
            COLOR_YELLOW
        );
        delay_ms(3000);


        uart_puts_raw("DIRECCION E - BIT 4\r\n");
        hub75_test_row_bit(
            4,
            COLOR_MAGENTA
        );
        delay_ms(3000);


        uart_puts_raw("ARCOIRIS POR FILAS\r\n");
        hub75_row_rainbow();
        delay_ms(4000);


        uart_puts_raw("LIMPIAR\r\n");
        hub75_clear();
        delay_ms(1500);
    }

    return 0;
}