#include <stdint.h>

#include <generated/csr.h>


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


static char uart_getc_raw_blocking(void)
{
    char c;

    while (uart_rxempty_read()) {
        delay(10);
    }

    c = (char)uart_rxtx_read();

    /*
        Limpiar evento RX pendiente.
    */

    uart_ev_pending_write(0xffffffff);

    return c;
}


static char uart_getc_filtered(void)
{
    char c;
    char d;
    uint32_t guard;

    c = uart_getc_raw_blocking();

    /*
        Dar un margen corto y drenar repeticiones inmediatas.
        Esto evita que un mismo caracter quede pegado.
    */

    delay(50000);

    guard = 5000;

    while (
        !uart_rxempty_read()
        && guard
    ) {
        d = (char)uart_rxtx_read();

        /*
            Si aparece otro caracter diferente, no lo podemos devolver
            porque no tenemos buffer todavia. Para esta prueba solo
            limpiamos basura/repeticiones.
        */

        (void)d;

        uart_ev_pending_write(0xffffffff);

        guard--;
    }

    return c;
}


int main(void)
{
    char c;

    uart_ev_pending_write(0xffffffff);

    uart_puts_raw(
        "\r\n"
        "========================================\r\n"
        " UART TEST FILTRADO\r\n"
        " Presiona una tecla corta.\r\n"
        " Debe responder una sola vez.\r\n"
        "========================================\r\n"
        "\r\n"
        "> "
    );

    while (1) {

        c = uart_getc_filtered();

        uart_puts_raw("\r\nRecibido: [");
        uart_putc_raw(c);
        uart_puts_raw("]\r\n> ");
    }

    return 0;
}