#include <stdint.h>

#include <generated/csr.h>
#include "ws2812_ui.h"


#define OLED_ADDR   0x3C
#define OLED_WIDTH  128
#define OLED_PAGES  8

#define OP_ADD      0
#define OP_SUB      1
#define OP_MUL      2
#define OP_DIV      3
#define OP_SQRT     4

#define LINE_SIZE   32


// -----------------------------------------------------------------------------
// DELAY
// -----------------------------------------------------------------------------

static void delay(unsigned int n)
{
    while (n--) {
        __asm__ volatile("nop");
    }
}


// -----------------------------------------------------------------------------
// UART
// -----------------------------------------------------------------------------

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


static void uart_put_uint(uint32_t value)
{
    char buffer[11];
    int index;

    if (value == 0) {
        uart_putc_raw('0');
        return;
    }

    index = 0;

    while (value > 0 && index < 10) {
        buffer[index] = (char)('0' + (value % 10));
        value /= 10;
        index++;
    }

    while (index > 0) {
        index--;
        uart_putc_raw(buffer[index]);
    }
}


/*
 * En este SoC, leer uart_rxtx_read() sin limpiar el evento puede dejar
 * disponible el mismo carácter y producir repeticiones.
 *
 * Esta función:
 *   1. espera un carácter;
 *   2. lo lee;
 *   3. limpia el evento RX;
 *   4. drena repeticiones inmediatas.
 */
static char uart_getc_filtered(void)
{
    char received;
    char discarded;
    uint32_t guard;

    while (uart_rxempty_read()) {
        delay(10);
    }

    received = (char)uart_rxtx_read();

    uart_ev_pending_write(0xffffffff);

    /*
     * Menos de 1 ms aproximadamente a 60 MHz.
     * Es suficiente para limpiar la repetición sin afectar escritura manual.
     */
    delay(50000);

    guard = 5000;

    while (!uart_rxempty_read() && guard > 0) {
        discarded = (char)uart_rxtx_read();
        (void)discarded;

        uart_ev_pending_write(0xffffffff);

        guard--;
    }

    return received;
}


static void uart_drain_rx(void)
{
    volatile char discarded;
    uint32_t guard;

    uart_ev_pending_write(0xffffffff);

    guard = 10000;

    while (!uart_rxempty_read() && guard > 0) {
        discarded = (char)uart_rxtx_read();
        uart_ev_pending_write(0xffffffff);
        guard--;
    }

    (void)discarded;
}


// -----------------------------------------------------------------------------
// I2C
// -----------------------------------------------------------------------------

static int i2c_send2(
    uint8_t address,
    uint8_t data0,
    uint8_t data1
)
{
    uint32_t timeout;

    i2c_oled_address_write(address);
    i2c_oled_data0_write(data0);
    i2c_oled_data1_write(data1);

    /*
     * start.re genera el pulso al escribir el CSR.
     */
    i2c_oled_start_write(1);

    timeout = 1000000;

    while (!i2c_oled_done_read() && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    if (i2c_oled_ack_error_read()) {
        return -1;
    }

    return 1;
}


// -----------------------------------------------------------------------------
// SSD1306
// -----------------------------------------------------------------------------

static int oled_command(uint8_t command)
{
    return i2c_send2(
        OLED_ADDR,
        0x00,
        command
    );
}


static int oled_data(uint8_t data)
{
    return i2c_send2(
        OLED_ADDR,
        0x40,
        data
    );
}


static void oled_set_pos(
    uint8_t page,
    uint8_t column
)
{
    oled_command((uint8_t)(0xB0 | (page & 0x07)));
    oled_command((uint8_t)(0x00 | (column & 0x0F)));
    oled_command((uint8_t)(0x10 | ((column >> 4) & 0x0F)));
}


static void oled_clear(void)
{
    uint8_t page;
    uint8_t column;

    for (page = 0; page < OLED_PAGES; page++) {
        oled_set_pos(page, 0);

        for (column = 0; column < OLED_WIDTH; column++) {
            oled_data(0x00);
        }
    }
}


static void oled_init(void)
{
    delay(1000000);

    oled_command(0xAE); // Display OFF

    oled_command(0xD5); // Clock divide
    oled_command(0x80);

    oled_command(0xA8); // Multiplex
    oled_command(0x3F);

    oled_command(0xD3); // Display offset
    oled_command(0x00);

    oled_command(0x40); // Start line

    oled_command(0x8D); // Charge pump
    oled_command(0x14);

    oled_command(0x20); // Memory addressing mode
    oled_command(0x02); // Page addressing mode

    oled_command(0xA1); // Segment remap
    oled_command(0xC8); // COM scan direction

    oled_command(0xDA); // COM pins
    oled_command(0x12);

    oled_command(0x81); // Contrast
    oled_command(0x7F);

    oled_command(0xD9); // Pre-charge
    oled_command(0xF1);

    oled_command(0xDB); // VCOM detect
    oled_command(0x40);

    oled_command(0xA4); // Display follows RAM
    oled_command(0xA6); // Normal display
    oled_command(0x2E); // Disable scroll
    oled_command(0xAF); // Display ON
}


// -----------------------------------------------------------------------------
// FUENTE 5x7
// -----------------------------------------------------------------------------

static uint8_t font5x7(
    char character,
    uint8_t column
)
{
    switch (character) {

        case '0': {
            const uint8_t f[5] = {0x3E, 0x51, 0x49, 0x45, 0x3E};
            return f[column];
        }

        case '1': {
            const uint8_t f[5] = {0x00, 0x42, 0x7F, 0x40, 0x00};
            return f[column];
        }

        case '2': {
            const uint8_t f[5] = {0x42, 0x61, 0x51, 0x49, 0x46};
            return f[column];
        }

        case '3': {
            const uint8_t f[5] = {0x21, 0x41, 0x45, 0x4B, 0x31};
            return f[column];
        }

        case '4': {
            const uint8_t f[5] = {0x18, 0x14, 0x12, 0x7F, 0x10};
            return f[column];
        }

        case '5': {
            const uint8_t f[5] = {0x27, 0x45, 0x45, 0x45, 0x39};
            return f[column];
        }

        case '6': {
            const uint8_t f[5] = {0x3C, 0x4A, 0x49, 0x49, 0x30};
            return f[column];
        }

        case '7': {
            const uint8_t f[5] = {0x01, 0x71, 0x09, 0x05, 0x03};
            return f[column];
        }

        case '8': {
            const uint8_t f[5] = {0x36, 0x49, 0x49, 0x49, 0x36};
            return f[column];
        }

        case '9': {
            const uint8_t f[5] = {0x06, 0x49, 0x49, 0x29, 0x1E};
            return f[column];
        }

        case 'A': {
            const uint8_t f[5] = {0x7E, 0x11, 0x11, 0x11, 0x7E};
            return f[column];
        }

        case 'B': {
            const uint8_t f[5] = {0x7F, 0x49, 0x49, 0x49, 0x36};
            return f[column];
        }

        case 'C': {
            const uint8_t f[5] = {0x3E, 0x41, 0x41, 0x41, 0x22};
            return f[column];
        }

        case 'D': {
            const uint8_t f[5] = {0x7F, 0x41, 0x41, 0x22, 0x1C};
            return f[column];
        }

        case 'E': {
            const uint8_t f[5] = {0x7F, 0x49, 0x49, 0x49, 0x41};
            return f[column];
        }

        case 'F': {
            const uint8_t f[5] = {0x7F, 0x09, 0x09, 0x09, 0x01};
            return f[column];
        }

        case 'G': {
            const uint8_t f[5] = {0x3E, 0x41, 0x49, 0x49, 0x7A};
            return f[column];
        }

        case 'H': {
            const uint8_t f[5] = {0x7F, 0x08, 0x08, 0x08, 0x7F};
            return f[column];
        }

        case 'I': {
            const uint8_t f[5] = {0x00, 0x41, 0x7F, 0x41, 0x00};
            return f[column];
        }

        case 'J': {
            const uint8_t f[5] = {0x20, 0x40, 0x41, 0x3F, 0x01};
            return f[column];
        }

        case 'K': {
            const uint8_t f[5] = {0x7F, 0x08, 0x14, 0x22, 0x41};
            return f[column];
        }

        case 'L': {
            const uint8_t f[5] = {0x7F, 0x40, 0x40, 0x40, 0x40};
            return f[column];
        }

        case 'M': {
            const uint8_t f[5] = {0x7F, 0x02, 0x0C, 0x02, 0x7F};
            return f[column];
        }

        case 'N': {
            const uint8_t f[5] = {0x7F, 0x04, 0x08, 0x10, 0x7F};
            return f[column];
        }

        case 'O': {
            const uint8_t f[5] = {0x3E, 0x41, 0x41, 0x41, 0x3E};
            return f[column];
        }

        case 'P': {
            const uint8_t f[5] = {0x7F, 0x09, 0x09, 0x09, 0x06};
            return f[column];
        }

        case 'Q': {
            const uint8_t f[5] = {0x3E, 0x41, 0x51, 0x21, 0x5E};
            return f[column];
        }

        case 'R': {
            const uint8_t f[5] = {0x7F, 0x09, 0x19, 0x29, 0x46};
            return f[column];
        }

        case 'S': {
            const uint8_t f[5] = {0x46, 0x49, 0x49, 0x49, 0x31};
            return f[column];
        }

        case 'T': {
            const uint8_t f[5] = {0x01, 0x01, 0x7F, 0x01, 0x01};
            return f[column];
        }

        case 'U': {
            const uint8_t f[5] = {0x3F, 0x40, 0x40, 0x40, 0x3F};
            return f[column];
        }

        case 'V': {
            const uint8_t f[5] = {0x1F, 0x20, 0x40, 0x20, 0x1F};
            return f[column];
        }

        case 'W': {
            const uint8_t f[5] = {0x7F, 0x20, 0x18, 0x20, 0x7F};
            return f[column];
        }

        case 'X': {
            const uint8_t f[5] = {0x63, 0x14, 0x08, 0x14, 0x63};
            return f[column];
        }

        case 'Y': {
            const uint8_t f[5] = {0x07, 0x08, 0x70, 0x08, 0x07};
            return f[column];
        }

        case 'Z': {
            const uint8_t f[5] = {0x61, 0x51, 0x49, 0x45, 0x43};
            return f[column];
        }

        case '+': {
            const uint8_t f[5] = {0x08, 0x08, 0x3E, 0x08, 0x08};
            return f[column];
        }

        case '-': {
            const uint8_t f[5] = {0x08, 0x08, 0x08, 0x08, 0x08};
            return f[column];
        }

        case '*': {
            const uint8_t f[5] = {0x22, 0x14, 0x7F, 0x14, 0x22};
            return f[column];
        }

        case '/': {
            const uint8_t f[5] = {0x20, 0x10, 0x08, 0x04, 0x02};
            return f[column];
        }

        case '=': {
            const uint8_t f[5] = {0x14, 0x14, 0x14, 0x14, 0x14};
            return f[column];
        }

        case ':': {
            const uint8_t f[5] = {0x00, 0x36, 0x36, 0x00, 0x00};
            return f[column];
        }

        case '.': {
            const uint8_t f[5] = {0x00, 0x60, 0x60, 0x00, 0x00};
            return f[column];
        }

        case '?': {
            const uint8_t f[5] = {0x02, 0x01, 0x51, 0x09, 0x06};
            return f[column];
        }

        case ' ': {
            return 0x00;
        }

        default: {
            const uint8_t f[5] = {0x00, 0x00, 0x5F, 0x00, 0x00};
            return f[column];
        }
    }
}


static void oled_draw_char(
    uint8_t page,
    uint8_t column,
    char character
)
{
    uint8_t index;

    oled_set_pos(page, column);

    for (index = 0; index < 5; index++) {
        oled_data(font5x7(character, index));
    }

    oled_data(0x00);
}


static void oled_draw_text(
    uint8_t page,
    uint8_t column,
    const char *text
)
{
    while (*text && column < 122) {
        oled_draw_char(page, column, *text);

        column += 6;
        text++;
    }
}


static void oled_draw_text_wrap(
    uint8_t page,
    const char *text
)
{
    uint8_t column;

    column = 0;

    while (*text && page < OLED_PAGES) {
        oled_draw_char(page, column, *text);

        text++;
        column += 6;

        if (column >= 122) {
            column = 0;
            page++;
        }
    }
}


static void oled_draw_uint(
    uint8_t page,
    uint8_t column,
    uint32_t value
)
{
    char buffer[11];
    int count;
    int output_index;

    if (value == 0) {
        oled_draw_char(page, column, '0');
        return;
    }

    count = 0;

    while (value > 0 && count < 10) {
        buffer[count] = (char)('0' + (value % 10));
        value /= 10;
        count++;
    }

    output_index = 0;

    while (count > 0 && column < 122) {
        count--;

        oled_draw_char(
            page,
            (uint8_t)(column + (output_index * 6)),
            buffer[count]
        );

        output_index++;
    }
}


// -----------------------------------------------------------------------------
// CALCULADORA HARDWARE
// -----------------------------------------------------------------------------

static int calculator_exec(
    uint32_t operand1,
    uint32_t operand2,
    uint8_t opcode,
    uint32_t *result
)
{
    uint32_t timeout;

    calculator_op1_write(operand1);
    calculator_op2_write(operand2);
    calculator_opcode_write(opcode);

    calculator_start_write(1);

    /*
     * Evita leer el DONE anterior antes de que el núcleo procese START.
     */
    delay(1000);

    timeout = 2000000;

    while (!calculator_done_read() && timeout > 0) {
        timeout--;
    }

    if (timeout == 0) {
        return 0;
    }

    if (calculator_error_read()) {
        return -1;
    }

    *result = calculator_result_read();

    return 1;
}


// -----------------------------------------------------------------------------
// PARSER
// -----------------------------------------------------------------------------

static char normalize_char(char character)
{
    if (character >= 'a' && character <= 'z') {
        return (char)(character - ('a' - 'A'));
    }

    return character;
}


static void skip_spaces(
    const char *text,
    int *position
)
{
    while (text[*position] == ' ') {
        (*position)++;
    }
}


static int parse_uint(
    const char *text,
    int *position,
    uint32_t *value
)
{
    uint32_t parsed;
    int digit_count;

    parsed = 0;
    digit_count = 0;

    while (
        text[*position] >= '0'
        && text[*position] <= '9'
    ) {
        parsed = (parsed * 10)
            + (uint32_t)(text[*position] - '0');

        (*position)++;
        digit_count++;
    }

    if (digit_count == 0) {
        return 0;
    }

    *value = parsed;

    return 1;
}


// -----------------------------------------------------------------------------
// PANTALLAS OLED
// -----------------------------------------------------------------------------

static void oled_show_home(void)
{
    oled_clear();

    oled_draw_text(0, 0, "CALC FPGA");
    oled_draw_text(2, 0, "ESCRIBE OP");
    oled_draw_text(4, 0, "Y ENTER");
}


static void oled_show_help(void)
{
    oled_clear();

    oled_draw_text(0, 0, "COMANDOS:");
    oled_draw_text(1, 0, "12+5  7*6");
    oled_draw_text(2, 0, "100/4");
    oled_draw_text(3, 0, "R144");
    oled_draw_text(4, 0, "T HOLA");
    oled_draw_text(5, 0, "C LIMPIAR");
}


static void oled_show_error(const char *message)
{
    oled_clear();

    oled_draw_text(0, 0, "ERROR:");
    oled_draw_text_wrap(2, message);
}


static void oled_show_result(
    const char *expression,
    uint32_t result
)
{
    oled_clear();

    oled_draw_text(0, 0, "CALC FPGA");
    oled_draw_text(2, 0, expression);

    oled_draw_text(4, 0, "RES:");
    oled_draw_uint(4, 30, result);
}


static void oled_show_text(const char *text)
{
    oled_clear();

    oled_draw_text(0, 0, "TEXTO:");
    oled_draw_text_wrap(2, text);
}


// -----------------------------------------------------------------------------
// PROCESAMIENTO DE COMANDOS
// -----------------------------------------------------------------------------

static void process_line(char *line)
{
    int position;
    uint32_t operand1;
    uint32_t operand2;
    uint32_t result;
    uint8_t opcode;
    char operation;
    int status;

    /*
     * C: limpiar pantalla.
     */
    if (
        line[0] == 'C'
        && line[1] == '\0'
    ) {
        oled_clear();
        matrix_clear();
        uart_puts_raw("OLED y matrix limpias.\r\n");
        return;
    }

    /*
     * H: ayuda.
     */
    if (
        line[0] == 'H'
        && line[1] == '\0'
    ) {
        oled_show_help();
        matrix_show_help_image();
        uart_puts_raw("Ayuda en OLED e imagen en WS2812.\r\n");
        return;
    }

    /*
     * T texto: texto libre.
     */
    if (
        line[0] == 'T'
        && line[1] == ' '
    ) {
        oled_show_text(&line[2]);

        uart_puts_raw(
            "Ejecutando animacion WS2812...\r\n"
        );

        matrix_moving_pixel();

        uart_puts_raw(
            "Animacion terminada.\r\n"
        );

        return;
    }

    /*
     * R144 o R 144: raíz cuadrada.
     */
    if (line[0] == 'R') {
        position = 1;

        skip_spaces(line, &position);

        if (!parse_uint(line, &position, &operand1)) {
            oled_show_error("RAIZ INVALIDA");
            uart_puts_raw("Error: raiz invalida.\r\n");
            return;
        }

        skip_spaces(line, &position);

        if (line[position] != '\0') {
            oled_show_error("RAIZ INVALIDA");
            uart_puts_raw("Error: caracteres extra.\r\n");
            return;
        }

        matrix_show_operator('R');

        status = calculator_exec(
            operand1,
            0,
            OP_SQRT,
            &result
        );

        if (status == 1) {
            oled_show_result(line, result);

            uart_puts_raw("Resultado: ");
            uart_put_uint(result);
            uart_puts_raw("\r\n");

        } else if (status == -1) {
            oled_show_error("CALC ERROR");
            uart_puts_raw("Error del hardware.\r\n");

        } else {
            oled_show_error("TIMEOUT");
            uart_puts_raw("Timeout de calculadora.\r\n");
        }

        return;
    }

    /*
     * Operación binaria:
     *   12+5
     *   12 - 5
     *   7*6
     *   100/4
     */
    position = 0;

    skip_spaces(line, &position);

    if (!parse_uint(line, &position, &operand1)) {
        oled_show_error("FALTA OP1");
        uart_puts_raw("Error: falta el primer operando.\r\n");
        return;
    }

    skip_spaces(line, &position);

    operation = line[position];

    if (
        operation != '+'
        && operation != '-'
        && operation != '*'
        && operation != '/'
    ) {
        oled_show_error("OPERACION");
        uart_puts_raw("Error: operador no valido.\r\n");
        return;
    }

    position++;

    skip_spaces(line, &position);

    if (!parse_uint(line, &position, &operand2)) {
        oled_show_error("FALTA OP2");
        uart_puts_raw("Error: falta el segundo operando.\r\n");
        return;
    }

    skip_spaces(line, &position);

    if (line[position] != '\0') {
        oled_show_error("ENTRADA");
        uart_puts_raw("Error: caracteres extra.\r\n");
        return;
    }

    switch (operation) {
        case '+':
            opcode = OP_ADD;
            break;

        case '-':
            opcode = OP_SUB;
            break;

        case '*':
            opcode = OP_MUL;
            break;

        case '/':
            opcode = OP_DIV;
            break;

        default:
            oled_show_error("OPERACION");
            return;
    }

    matrix_show_operator(operation);

    status = calculator_exec(
        operand1,
        operand2,
        opcode,
        &result
    );

    if (status == 1) {
        oled_show_result(line, result);

        uart_puts_raw("Resultado: ");
        uart_put_uint(result);
        uart_puts_raw("\r\n");

    } else if (status == -1) {
        oled_show_error("CALC ERROR");
        uart_puts_raw("Error del hardware");

        if (operation == '/' && operand2 == 0) {
            uart_puts_raw(": division por cero");
        }

        uart_puts_raw(".\r\n");

    } else {
        oled_show_error("TIMEOUT");
        uart_puts_raw("Timeout de calculadora.\r\n");
    }
}


// -----------------------------------------------------------------------------
// MAIN
// -----------------------------------------------------------------------------

int main(void)
{
    char character;
    char line[LINE_SIZE];

    uint8_t length;

    uart_ev_pending_write(0xffffffff);
    uart_drain_rx();

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " OLED + CALCULADORA FPGA\r\n"
        " Escribe una operacion y presiona ENTER\r\n"
        "\r\n"
        " Ejemplos:\r\n"
        "   12+5\r\n"
        "   12-5\r\n"
        "   7*6\r\n"
        "   100/4\r\n"
        "   10/0\r\n"
        "   R144\r\n"
        "   T HOLA DAVID\r\n"
        "   H\r\n"
        "   C\r\n"
        "========================================\r\n"
        "\r\n"
    );

    oled_init();
    oled_show_home();

    matrix_clear();

    length = 0;
    line[0] = '\0';

    uart_puts_raw("> ");

    while (1) {
        character = uart_getc_filtered();

        /*
         * ENTER.
         */
        if (
            character == '\r'
            || character == '\n'
        ) {
            uart_puts_raw("\r\n");

            line[length] = '\0';

            if (length > 0) {
                uart_puts_raw("Procesando: ");
                uart_puts_raw(line);
                uart_puts_raw("\r\n");

                process_line(line);
            }

            length = 0;
            line[0] = '\0';

            uart_puts_raw("> ");
        }

        /*
         * BACKSPACE.
         */
        else if (
            character == 8
            || character == 127
        ) {
            if (length > 0) {
                length--;
                line[length] = '\0';

                uart_puts_raw("\b \b");
            }
        }

        /*
         * Carácter visible.
         */
        else if (
            character >= 32
            && character <= 126
        ) {
            if (length < (LINE_SIZE - 1)) {
                character = normalize_char(character);

                line[length] = character;
                length++;
                line[length] = '\0';

                /*
                 * Solo eco en UART.
                 * La OLED se actualiza al presionar ENTER.
                 */
                uart_putc_raw(character);
            } else {
                uart_puts_raw(
                    "\r\nError: linea demasiado larga.\r\n> "
                );

                length = 0;
                line[0] = '\0';
            }
        }
    }

    return 0;
}