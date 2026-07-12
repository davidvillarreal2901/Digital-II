#ifndef WS2812_UI_H
#define WS2812_UI_H


#include <stdint.h>

#include <generated/csr.h>

#include "imagen_ws2812.h"


#define MATRIX_NUM_PIXELS 64
#define MATRIX_WIDTH      8
#define MATRIX_HEIGHT     8

#define MATRIX_OFF        0x000000
#define MATRIX_RED        0x200000
#define MATRIX_GREEN      0x002000
#define MATRIX_BLUE       0x000020
#define MATRIX_YELLOW     0x202000
#define MATRIX_CYAN       0x002020
#define MATRIX_MAGENTA    0x200020
#define MATRIX_WHITE      0x101010


static void matrix_delay(uint32_t cycles)
{
    while (cycles--) {
        __asm__ volatile("nop");
    }
}


/*
 * Escribe un color RGB 0xRRGGBB en el framebuffer de la matriz.
 */
static void matrix_write_pixel(
    uint8_t pixel,
    uint32_t color
)
{
    ws2812_pixel_index_write(pixel);
    ws2812_pixel_color_write(color & 0x00FFFFFF);
    ws2812_write_pixel_write(1);
}


/*
 * Envía el framebuffer completo a la matriz física.
 */
static int matrix_show(void)
{
    uint32_t timeout;

    /*
     * Esperar que no haya una transmisión anterior activa.
     */
    timeout = 2000000;

    while (ws2812_busy_read() && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    /*
     * start.re genera un pulso al escribir el CSR.
     */
    ws2812_start_write(1);

    /*
     * Esperar que BUSY se active para confirmar el arranque.
     */
    timeout = 200000;

    while (!ws2812_busy_read() && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    /*
     * Esperar el final de la transmisión.
     */
    timeout = 3000000;

    while (ws2812_busy_read() && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    return 1;
}


static void matrix_clear_buffer(void)
{
    uint8_t pixel;

    for (pixel = 0; pixel < MATRIX_NUM_PIXELS; pixel++) {
        matrix_write_pixel(pixel, MATRIX_OFF);
    }
}


static void matrix_clear(void)
{
    matrix_clear_buffer();
    matrix_show();
}


/*
 * Cada byte representa una fila de 8 píxeles.
 * El bit 7 corresponde al píxel izquierdo.
 */
static void matrix_show_bitmap(
    const uint8_t bitmap[8],
    uint32_t color
)
{
    uint8_t row;
    uint8_t column;
    uint8_t pixel;
    uint8_t mask;

    for (row = 0; row < MATRIX_HEIGHT; row++) {

        for (column = 0; column < MATRIX_WIDTH; column++) {

            pixel = (uint8_t)(
                (row * MATRIX_WIDTH)
                + column
            );

            mask = (uint8_t)(
                1u << (7u - column)
            );

            if (bitmap[row] & mask) {
                matrix_write_pixel(pixel, color);
            } else {
                matrix_write_pixel(pixel, MATRIX_OFF);
            }
        }
    }

    matrix_show();
}


/*
 * Símbolo +
 */
static const uint8_t bitmap_plus[8] = {
    0b00000000,
    0b00011000,
    0b00011000,
    0b01111110,
    0b01111110,
    0b00011000,
    0b00011000,
    0b00000000
};


/*
 * Símbolo -
 */
static const uint8_t bitmap_minus[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b01111110,
    0b01111110,
    0b00000000,
    0b00000000,
    0b00000000
};


/*
 * Símbolo de multiplicación X
 */
static const uint8_t bitmap_multiply[8] = {
    0b01000010,
    0b00100100,
    0b00011000,
    0b00011000,
    0b00011000,
    0b00100100,
    0b01000010,
    0b00000000
};


/*
 * Símbolo /
 */
static const uint8_t bitmap_divide[8] = {
    0b00000011,
    0b00000110,
    0b00001100,
    0b00011000,
    0b00110000,
    0b01100000,
    0b11000000,
    0b00000000
};


/*
 * Símbolo aproximado de raíz.
 */
static const uint8_t bitmap_sqrt[8] = {
    0b00000000,
    0b00000011,
    0b00000101,
    0b01001001,
    0b00110001,
    0b00010001,
    0b00001110,
    0b00000000
};


static void matrix_show_operator(char operation)
{
    switch (operation) {

        case '+':
            matrix_show_bitmap(
                bitmap_plus,
                MATRIX_GREEN
            );
            break;

        case '-':
            matrix_show_bitmap(
                bitmap_minus,
                MATRIX_YELLOW
            );
            break;

        case '*':
            matrix_show_bitmap(
                bitmap_multiply,
                MATRIX_MAGENTA
            );
            break;

        case '/':
            matrix_show_bitmap(
                bitmap_divide,
                MATRIX_CYAN
            );
            break;

        case 'R':
            matrix_show_bitmap(
                bitmap_sqrt,
                MATRIX_BLUE
            );
            break;

        default:
            matrix_clear();
            break;
    }
}


/*
 * Animación usada con el comando de texto.
 *
 * Se mantiene un solo LED encendido y se desplaza
 * linealmente desde el píxel 0 hasta el 63.
 */
static void matrix_moving_pixel(void)
{
    uint8_t pixel;

    matrix_clear_buffer();
    matrix_show();

    for (pixel = 0; pixel < MATRIX_NUM_PIXELS; pixel++) {

        if (pixel > 0) {
            matrix_write_pixel(
                (uint8_t)(pixel - 1),
                MATRIX_OFF
            );
        }

        matrix_write_pixel(
            pixel,
            MATRIX_BLUE
        );

        matrix_show();

        /*
         * Aproximadamente visible como animación
         * sin hacer demasiado lenta la demo.
         */
        matrix_delay(700000);
    }

    matrix_clear();
}


/*
 * Muestra la imagen generada previamente desde Kirby.
 */
static void matrix_show_help_image(void)
{
    uint8_t pixel;

    for (pixel = 0; pixel < MATRIX_NUM_PIXELS; pixel++) {
        matrix_write_pixel(
            pixel,
            imagen_ws2812[pixel]
        );
    }

    matrix_show();
}


#endif