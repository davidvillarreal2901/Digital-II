#include <stdint.h>

#include <generated/csr.h>

#include "hub75_asset.h"


#define HUB75_WORD_COUNT 2048u


static void delay_cycles(uint32_t cycles)
{
    while (cycles--) {
        __asm__ volatile("nop");
    }
}


static void delay_ms(uint32_t milliseconds)
{
    uint32_t index;

    /*
     * Retardo visual aproximado para sys_clk de 60 MHz.
     * El tiempo de copia del frame también forma parte
     * del tiempo visible.
     */
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

    uart_rxtx_write(
        (uint32_t)character
    );
}


static void uart_puts_raw(const char *text)
{
    while (*text) {
        uart_putc_raw(*text++);
    }
}


static void uart_put_uint(uint32_t value)
{
    char buffer[11];
    uint32_t count;

    if (value == 0) {
        uart_putc_raw('0');
        return;
    }

    count = 0;

    while (value > 0 && count < 10) {
        buffer[count] = (char)(
            '0' + (value % 10)
        );

        value /= 10;
        count++;
    }

    while (count > 0) {
        count--;
        uart_putc_raw(buffer[count]);
    }
}


static void hub75_write_word(
    uint16_t address,
    uint32_t data
)
{
    hub75_pixel_addr_write(
        address
    );

    hub75_pixel_data_write(
        data & 0x00FFFFFFu
    );

    hub75_write_pixel_write(1);
}


static void hub75_clear_buffer(void)
{
    uint16_t address;

    hub75_blank_write(1);

    for (
        address = 0;
        address < HUB75_WORD_COUNT;
        address++
    ) {
        hub75_write_word(
            address,
            0x000000u
        );
    }

    hub75_blank_write(0);
}


static void hub75_load_frame(
    const uint32_t *frame
)
{
    uint16_t address;

    /*
     * Apagar temporalmente el panel para evitar tearing
     * mientras el procesador cambia las 2048 palabras.
     */
    hub75_blank_write(1);

    delay_cycles(1000);

    for (
        address = 0;
        address < HUB75_WORD_COUNT;
        address++
    ) {
        hub75_write_word(
            address,
            frame[address]
        );
    }

    delay_cycles(1000);

    hub75_blank_write(0);
}


static void hub75_show_image(void)
{
    hub75_load_frame(
        hub75_asset_frames[0]
    );
}


static void hub75_play_animation(
    uint32_t loops
)
{
    uint32_t loop;
    uint32_t frame;

    for (loop = 0; loop < loops; loop++) {

        for (
            frame = 0;
            frame < HUB75_ASSET_FRAME_COUNT;
            frame++
        ) {
            hub75_load_frame(
                hub75_asset_frames[frame]
            );

            delay_ms(
                hub75_asset_delay_ms[frame]
            );
        }
    }
}


int main(void)
{
    uart_ev_pending_write(
        0xffffffff
    );

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " HUB75 LITEX - IMAGENES Y GIF\r\n"
        " Framebuffer: 2048 x 24 bits\r\n"
        " Frames cargados: "
    );

    uart_put_uint(
        HUB75_ASSET_FRAME_COUNT
    );

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        "\r\n"
    );

    /*
     * Arranque seguro.
     */
    hub75_blank_write(1);
    hub75_clear_buffer();

    /*
     * Primero se deja visible el primer frame
     * como imagen estática.
     */
    uart_puts_raw(
        "Mostrando imagen estatica...\r\n"
    );

    hub75_show_image();

    delay_ms(2000);

    /*
     * Luego se reproduce continuamente el GIF.
     */
    uart_puts_raw(
        "Reproduciendo animacion...\r\n"
    );

    while (1) {
        hub75_play_animation(1);
    }

    return 0;
}