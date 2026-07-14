#ifndef HUB75_UI_H
#define HUB75_UI_H


#include <stdint.h>

#include <generated/csr.h>

#include "hub75_asset.h"


#define HUB75_WIDTH             64u
#define HUB75_HEIGHT            64u
#define HUB75_SCAN_ROWS         32u
#define HUB75_WORD_COUNT        2048u

/*
 * Color RGB444:
 *
 *   bits 11:8 = rojo
 *   bits  7:4 = verde
 *   bits  3:0 = azul
 */
#define HUB75_BLACK             0x000u
#define HUB75_RED               0xF00u
#define HUB75_GREEN             0x0F0u
#define HUB75_BLUE              0x00Fu
#define HUB75_YELLOW            0xFF0u
#define HUB75_CYAN              0x0FFu
#define HUB75_MAGENTA           0xF0Fu
#define HUB75_WHITE             0xFFFu
#define HUB75_ORANGE            0xF80u


// -----------------------------------------------------------------------------
// RETARDOS
// -----------------------------------------------------------------------------

static void hub75_ui_delay_cycles(uint32_t cycles)
{
    while (cycles--) {
        __asm__ volatile("nop");
    }
}


static void hub75_ui_delay_ms(uint32_t milliseconds)
{
    uint32_t index;

    while (milliseconds--) {
        for (index = 0; index < 12000u; index++) {
            __asm__ volatile("nop");
        }
    }
}


// -----------------------------------------------------------------------------
// ACCESO AL FRAMEBUFFER
// -----------------------------------------------------------------------------

static void hub75_ui_write_word(
    uint16_t address,
    uint32_t data
)
{
    hub75_pixel_addr_write(address);

    hub75_pixel_data_write(
        data & 0x00FFFFFFu
    );

    hub75_write_pixel_write(1);
}


static uint32_t hub75_ui_pack_pixels(
    uint16_t top,
    uint16_t bottom
)
{
    return (
        ((uint32_t)(top & 0x0FFFu) << 12)
        | ((uint32_t)bottom & 0x0FFFu)
    );
}


static void hub75_clear(void)
{
    uint16_t address;

    hub75_blank_write(1);

    for (
        address = 0;
        address < HUB75_WORD_COUNT;
        address++
    ) {
        hub75_ui_write_word(
            address,
            0x000000u
        );
    }

    hub75_blank_write(0);
}


// -----------------------------------------------------------------------------
// GIF / IMAGEN GENERADA
// -----------------------------------------------------------------------------

static void hub75_show_asset_frame(uint32_t frame_index)
{
    uint16_t address;

    if (frame_index >= HUB75_ASSET_FRAME_COUNT) {
        frame_index = 0;
    }

    hub75_blank_write(1);

    for (
        address = 0;
        address < HUB75_WORD_COUNT;
        address++
    ) {
        hub75_ui_write_word(
            address,
            hub75_asset_frames[frame_index][address]
        );
    }

    hub75_blank_write(0);
}


static void hub75_play_asset(uint32_t loops)
{
    uint32_t loop;
    uint32_t frame;

    for (loop = 0; loop < loops; loop++) {

        for (
            frame = 0;
            frame < HUB75_ASSET_FRAME_COUNT;
            frame++
        ) {
            hub75_show_asset_frame(frame);

            hub75_ui_delay_ms(
                hub75_asset_delay_ms[frame]
            );
        }
    }
}


// -----------------------------------------------------------------------------
// FUENTE 5x7
// -----------------------------------------------------------------------------

static uint8_t hub75_ui_font_column(
    char character,
    uint8_t column
)
{
    switch (character) {

        case '0': {
            const uint8_t f[5] = {
                0x3E, 0x51, 0x49, 0x45, 0x3E
            };

            return f[column];
        }

        case '1': {
            const uint8_t f[5] = {
                0x00, 0x42, 0x7F, 0x40, 0x00
            };

            return f[column];
        }

        case '2': {
            const uint8_t f[5] = {
                0x42, 0x61, 0x51, 0x49, 0x46
            };

            return f[column];
        }

        case '3': {
            const uint8_t f[5] = {
                0x21, 0x41, 0x45, 0x4B, 0x31
            };

            return f[column];
        }

        case '4': {
            const uint8_t f[5] = {
                0x18, 0x14, 0x12, 0x7F, 0x10
            };

            return f[column];
        }

        case '5': {
            const uint8_t f[5] = {
                0x27, 0x45, 0x45, 0x45, 0x39
            };

            return f[column];
        }

        case '6': {
            const uint8_t f[5] = {
                0x3C, 0x4A, 0x49, 0x49, 0x30
            };

            return f[column];
        }

        case '7': {
            const uint8_t f[5] = {
                0x01, 0x71, 0x09, 0x05, 0x03
            };

            return f[column];
        }

        case '8': {
            const uint8_t f[5] = {
                0x36, 0x49, 0x49, 0x49, 0x36
            };

            return f[column];
        }

        case '9': {
            const uint8_t f[5] = {
                0x06, 0x49, 0x49, 0x29, 0x1E
            };

            return f[column];
        }

        case 'A': {
            const uint8_t f[5] = {
                0x7E, 0x11, 0x11, 0x11, 0x7E
            };

            return f[column];
        }

        case 'C': {
            const uint8_t f[5] = {
                0x3E, 0x41, 0x41, 0x41, 0x22
            };

            return f[column];
        }

        case 'E': {
            const uint8_t f[5] = {
                0x7F, 0x49, 0x49, 0x49, 0x41
            };

            return f[column];
        }

        case 'H': {
            const uint8_t f[5] = {
                0x7F, 0x08, 0x08, 0x08, 0x7F
            };

            return f[column];
        }

        case 'L': {
            const uint8_t f[5] = {
                0x7F, 0x40, 0x40, 0x40, 0x40
            };

            return f[column];
        }

        case 'O': {
            const uint8_t f[5] = {
                0x3E, 0x41, 0x41, 0x41, 0x3E
            };

            return f[column];
        }

        case 'P': {
            const uint8_t f[5] = {
                0x7F, 0x09, 0x09, 0x09, 0x06
            };

            return f[column];
        }

        case 'R': {
            const uint8_t f[5] = {
                0x7F, 0x09, 0x19, 0x29, 0x46
            };

            return f[column];
        }

        case 'S': {
            const uint8_t f[5] = {
                0x46, 0x49, 0x49, 0x49, 0x31
            };

            return f[column];
        }

        case 'T': {
            const uint8_t f[5] = {
                0x01, 0x01, 0x7F, 0x01, 0x01
            };

            return f[column];
        }

        case 'X': {
            const uint8_t f[5] = {
                0x63, 0x14, 0x08, 0x14, 0x63
            };

            return f[column];
        }

        case '+': {
            const uint8_t f[5] = {
                0x08, 0x08, 0x3E, 0x08, 0x08
            };

            return f[column];
        }

        case '-': {
            const uint8_t f[5] = {
                0x08, 0x08, 0x08, 0x08, 0x08
            };

            return f[column];
        }

        case '*': {
            const uint8_t f[5] = {
                0x22, 0x14, 0x7F, 0x14, 0x22
            };

            return f[column];
        }

        case '/': {
            const uint8_t f[5] = {
                0x20, 0x10, 0x08, 0x04, 0x02
            };

            return f[column];
        }

        case '=': {
            const uint8_t f[5] = {
                0x14, 0x14, 0x14, 0x14, 0x14
            };

            return f[column];
        }

        case ' ': {
            return 0x00;
        }

        default: {
            const uint8_t f[5] = {
                0x00, 0x00, 0x5F, 0x00, 0x00
            };

            return f[column];
        }
    }
}


static uint8_t hub75_ui_text_length(const char *text)
{
    uint8_t length;

    length = 0;

    while (
        text[length] != '\0'
        && length < 31
    ) {
        length++;
    }

    return length;
}


static int hub75_ui_text_pixel(
    const char *text,
    uint8_t x,
    uint8_t y,
    uint8_t start_x,
    uint8_t start_y,
    uint8_t scale
)
{
    uint16_t relative_x;
    uint16_t relative_y;

    uint8_t character_index;
    uint8_t character_column;
    uint8_t character_row;
    uint8_t character_count;

    uint16_t character_width;

    uint8_t bitmap_column;

    character_count = hub75_ui_text_length(text);

    character_width = (uint16_t)(6u * scale);

    if (
        x < start_x
        || y < start_y
    ) {
        return 0;
    }

    relative_x = (uint16_t)x - start_x;
    relative_y = (uint16_t)y - start_y;

    character_index = (uint8_t)(
        relative_x / character_width
    );

    if (character_index >= character_count) {
        return 0;
    }

    character_column = (uint8_t)(
        (relative_x % character_width) / scale
    );

    character_row = (uint8_t)(
        relative_y / scale
    );

    if (
        character_column >= 5
        || character_row >= 7
    ) {
        return 0;
    }

    bitmap_column = hub75_ui_font_column(
        text[character_index],
        character_column
    );

    return (
        bitmap_column
        & (1u << character_row)
    ) != 0;
}


static void hub75_ui_uint_to_text(
    uint32_t value,
    char *output
)
{
    char reverse[11];

    uint8_t count;
    uint8_t index;

    if (value == 0) {
        output[0] = '0';
        output[1] = '\0';
        return;
    }

    count = 0;

    while (
        value > 0
        && count < 10
    ) {
        reverse[count] = (char)(
            '0' + (value % 10)
        );

        value /= 10;
        count++;
    }

    index = 0;

    while (count > 0) {
        count--;

        output[index] = reverse[count];
        index++;
    }

    output[index] = '\0';
}


static uint16_t hub75_ui_operator_color(char operation)
{
    switch (operation) {

        case '+':
            return HUB75_GREEN;

        case '-':
            return HUB75_YELLOW;

        case '*':
            return HUB75_MAGENTA;

        case '/':
            return HUB75_CYAN;

        case 'R':
            return HUB75_BLUE;

        default:
            return HUB75_WHITE;
    }
}


// -----------------------------------------------------------------------------
// MOSTRAR OPERACIÓN Y RESULTADO
// -----------------------------------------------------------------------------

static uint16_t hub75_ui_operation_pixel(
    const char *expression,
    const char *result_text,
    char operation,
    uint8_t x,
    uint8_t y
)
{
    uint8_t expression_length;
    uint8_t result_length;

    uint8_t expression_scale;
    uint8_t result_scale;

    uint8_t expression_width;
    uint8_t result_width;

    uint8_t expression_start_x;
    uint8_t result_start_x;

    uint16_t operation_color;

    expression_length = hub75_ui_text_length(
        expression
    );

    result_length = hub75_ui_text_length(
        result_text
    );

    expression_scale = (
        expression_length <= 5
    ) ? 2 : 1;

    result_scale = (
        result_length <= 5
    ) ? 2 : 1;

    expression_width = (uint8_t)(
        expression_length
        * 6u
        * expression_scale
    );

    result_width = (uint8_t)(
        result_length
        * 6u
        * result_scale
    );

    expression_start_x = (
        expression_width < HUB75_WIDTH
    )
        ? (uint8_t)((HUB75_WIDTH - expression_width) / 2u)
        : 0;

    result_start_x = (
        result_width < HUB75_WIDTH
    )
        ? (uint8_t)((HUB75_WIDTH - result_width) / 2u)
        : 0;

    operation_color = hub75_ui_operator_color(
        operation
    );

    /*
     * Marco.
     */
    if (
        x == 0
        || x == 63
        || y == 0
        || y == 63
    ) {
        return operation_color;
    }

    /*
     * Línea divisoria.
     */
    if (
        y == 30
        || y == 31
    ) {
        return operation_color;
    }

    /*
     * Operación.
     */
    if (
        hub75_ui_text_pixel(
            expression,
            x,
            y,
            expression_start_x,
            8,
            expression_scale
        )
    ) {
        return operation_color;
    }

    /*
     * Resultado.
     */
    if (
        hub75_ui_text_pixel(
            result_text,
            x,
            y,
            result_start_x,
            39,
            result_scale
        )
    ) {
        return HUB75_WHITE;
    }

    return HUB75_BLACK;
}


static void hub75_show_operation(
    const char *expression,
    uint32_t result,
    char operation
)
{
    char number_text[11];
    char result_text[12];

    uint8_t index;
    uint8_t result_index;

    uint8_t row;
    uint8_t column;

    uint16_t top_color;
    uint16_t bottom_color;

    uint16_t address;

    hub75_ui_uint_to_text(
        result,
        number_text
    );

    result_text[0] = '=';

    index = 0;
    result_index = 1;

    while (
        number_text[index] != '\0'
        && result_index < 11
    ) {
        result_text[result_index] =
            number_text[index];

        index++;
        result_index++;
    }

    result_text[result_index] = '\0';

    hub75_blank_write(1);

    for (row = 0; row < HUB75_SCAN_ROWS; row++) {

        for (
            column = 0;
            column < HUB75_WIDTH;
            column++
        ) {
            top_color = hub75_ui_operation_pixel(
                expression,
                result_text,
                operation,
                column,
                row
            );

            bottom_color = hub75_ui_operation_pixel(
                expression,
                result_text,
                operation,
                column,
                (uint8_t)(row + 32u)
            );

            address = (uint16_t)(
                (row * HUB75_WIDTH)
                + column
            );

            hub75_ui_write_word(
                address,
                hub75_ui_pack_pixels(
                    top_color,
                    bottom_color
                )
            );
        }
    }

    hub75_blank_write(0);
}


// -----------------------------------------------------------------------------
// MENSAJES
// -----------------------------------------------------------------------------

static uint16_t hub75_ui_message_pixel(
    const char *message,
    uint16_t color,
    uint8_t x,
    uint8_t y
)
{
    uint8_t length;
    uint8_t scale;
    uint8_t width;
    uint8_t start_x;
    uint8_t start_y;

    length = hub75_ui_text_length(message);

    if (length <= 3) {
        scale = 3;
    } else if (length <= 5) {
        scale = 2;
    } else {
        scale = 1;
    }

    width = (uint8_t)(
        length * 6u * scale
    );

    start_x = (
        width < HUB75_WIDTH
    )
        ? (uint8_t)((HUB75_WIDTH - width) / 2u)
        : 0;

    start_y = (uint8_t)(
        (HUB75_HEIGHT - (7u * scale)) / 2u
    );

    if (
        x == 0
        || x == 63
        || y == 0
        || y == 63
    ) {
        return color;
    }

    if (
        hub75_ui_text_pixel(
            message,
            x,
            y,
            start_x,
            start_y,
            scale
        )
    ) {
        return color;
    }

    return HUB75_BLACK;
}


static void hub75_show_message(
    const char *message,
    uint16_t color
)
{
    uint8_t row;
    uint8_t column;

    uint16_t top_color;
    uint16_t bottom_color;

    uint16_t address;

    hub75_blank_write(1);

    for (row = 0; row < HUB75_SCAN_ROWS; row++) {

        for (
            column = 0;
            column < HUB75_WIDTH;
            column++
        ) {
            top_color = hub75_ui_message_pixel(
                message,
                color,
                column,
                row
            );

            bottom_color = hub75_ui_message_pixel(
                message,
                color,
                column,
                (uint8_t)(row + 32u)
            );

            address = (uint16_t)(
                (row * HUB75_WIDTH)
                + column
            );

            hub75_ui_write_word(
                address,
                hub75_ui_pack_pixels(
                    top_color,
                    bottom_color
                )
            );
        }
    }

    hub75_blank_write(0);
}


static void hub75_show_error(void)
{
    hub75_show_message(
        "ERROR",
        HUB75_RED
    );
}


#endif