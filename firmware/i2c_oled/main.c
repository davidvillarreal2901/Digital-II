#include <stdint.h>

#include <generated/csr.h>


#define OLED_ADDR 0x3C

#define OLED_WIDTH  128
#define OLED_HEIGHT 64
#define OLED_PAGES  8


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


static void uart_put_hex4(uint32_t value)
{
    const char hex[] = "0123456789ABCDEF";

    uart_putc_raw(hex[(value >> 4) & 0xF]);
    uart_putc_raw(hex[value & 0xF]);
}


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

    i2c_oled_start_write(1);

    timeout = 1000000;

    while (
        !i2c_oled_done_read()
        && timeout
    ) {
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


static int oled_command(uint8_t cmd)
{
    int result;

    result = i2c_send2(
        OLED_ADDR,
        0x00,
        cmd
    );

    if (result != 1) {
        uart_puts_raw("CMD fallo 0x");
        uart_put_hex4(cmd);
        uart_puts_raw("\r\n");
    }

    return result;
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
    uint8_t col
)
{
    oled_command(0xB0 | (page & 0x07));
    oled_command(0x00 | (col & 0x0F));
    oled_command(0x10 | ((col >> 4) & 0x0F));
}


static void oled_clear(void)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < OLED_PAGES; page++) {

        oled_set_pos(page, 0);

        for (col = 0; col < OLED_WIDTH; col++) {
            oled_data(0x00);
        }
    }
}


static void oled_fill(uint8_t value)
{
    uint8_t page;
    uint8_t col;

    for (page = 0; page < OLED_PAGES; page++) {

        oled_set_pos(page, 0);

        for (col = 0; col < OLED_WIDTH; col++) {
            oled_data(value);
        }
    }
}


static void oled_test_pattern(void)
{
    uint8_t page;
    uint8_t col;
    uint8_t value;

    for (page = 0; page < OLED_PAGES; page++) {

        oled_set_pos(page, 0);

        for (col = 0; col < OLED_WIDTH; col++) {

            if (page == 0) {
                value = 0xFF;
            } else if (page == 1) {
                value = 0x00;
            } else if (page == 2) {
                value = 0xAA;
            } else if (page == 3) {
                value = 0x55;
            } else if (col < 32) {
                value = 0xFF;
            } else if (col < 64) {
                value = 0x00;
            } else if (col < 96) {
                value = 0xAA;
            } else {
                value = 0x55;
            }

            oled_data(value);
        }
    }
}


static void oled_init(void)
{
    delay(1000000);

    oled_command(0xAE); // display OFF

    oled_command(0xD5); // display clock divide
    oled_command(0x80);

    oled_command(0xA8); // multiplex
    oled_command(0x3F);

    oled_command(0xD3); // display offset
    oled_command(0x00);

    oled_command(0x40); // start line

    oled_command(0x8D); // charge pump
    oled_command(0x14);

    oled_command(0x20); // memory mode
    oled_command(0x02); // page addressing mode

    oled_command(0xA1); // segment remap
    oled_command(0xC8); // COM scan direction

    oled_command(0xDA); // COM pins
    oled_command(0x12);

    oled_command(0x81); // contrast
    oled_command(0x7F);

    oled_command(0xD9); // pre-charge
    oled_command(0xF1);

    oled_command(0xDB); // VCOM detect
    oled_command(0x40);

    oled_command(0xA4); // resume RAM display
    oled_command(0xA6); // normal display

    oled_command(0x2E); // deactivate scroll

    oled_command(0xAF); // display ON
}


int main(void)
{
    uart_ev_pending_write(0xffffffff);

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " OLED SSD1306 - init + patron\r\n"
        " I2C SCL -> E5\r\n"
        " I2C SDA -> F5\r\n"
        "========================================\r\n"
        "\r\n"
    );

    uart_puts_raw("Inicializando OLED...\r\n");

    oled_init();

    uart_puts_raw("Limpiando pantalla...\r\n");

    oled_clear();

    delay(5000000);

    uart_puts_raw("Mostrando patron...\r\n");

    oled_test_pattern();

    while (1) {

        delay(20000000);

        uart_puts_raw("Fill 0xFF\r\n");
        oled_fill(0xFF);

        delay(20000000);

        uart_puts_raw("Clear\r\n");
        oled_clear();

        delay(20000000);

        uart_puts_raw("Patron\r\n");
        oled_test_pattern();
    }

    return 0;
}