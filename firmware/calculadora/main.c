#include <stdint.h>
#include <generated/csr.h>

#define OP_ADD   0
#define OP_SUB   1
#define OP_MUL   2
#define OP_DIV   3
#define OP_SQRT  4

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

static char uart_getc_raw(void)
{
    char c;

    while (uart_rxempty_read()) {
        delay(10);
    }

    c = (char)(uart_rxtx_read() & 0xff);

    uart_ev_pending_write(0xffffffff);

    return c;
}

static void read_line(char *buf, int max_len)
{
    int i = 0;

    while (1) {
        char c = uart_getc_raw();

        if (c == '\r' || c == '\n') {
            uart_puts_raw("\r\n");
            break;
        }

        if (c == 127 || c == 8) {
            if (i > 0) {
                i--;
                uart_puts_raw("\b \b");
            }
            continue;
        }

        if (i < max_len - 1) {
            buf[i++] = c;
            uart_putc_raw(c);
        }
    }

    buf[i] = '\0';
}

static void skip_spaces(const char **p)
{
    while (**p == ' ' || **p == '\t') {
        (*p)++;
    }
}

static int parse_uint(const char **p, uint32_t *value)
{
    uint32_t r = 0;
    int ok = 0;

    skip_spaces(p);

    while (**p >= '0' && **p <= '9') {
        r = r * 10 + (uint32_t)(**p - '0');
        (*p)++;
        ok = 1;
    }

    *value = r;
    return ok;
}

static void uart_put_uint(uint32_t v)
{
    char buf[11];
    int i = 0;

    if (v == 0) {
        uart_putc_raw('0');
        return;
    }

    while (v > 0 && i < 10) {
        buf[i++] = '0' + (v % 10);
        v /= 10;
    }

    while (i > 0) {
        uart_putc_raw(buf[--i]);
    }
}

static uint32_t calc_exec(uint32_t a, uint32_t b, uint32_t op, int *ok)
{
    uint32_t timeout = 10000000;

    calculator_op1_write(a);
    calculator_op2_write(b);
    calculator_opcode_write(op);

    calculator_start_write(1);
    delay(100);
    calculator_start_write(0);

    while (!calculator_done_read()) {
        if (timeout == 0) {
            *ok = 0;
            return 0;
        }

        timeout--;
    }

    if (calculator_error_read()) {
        *ok = 0;
        return 0;
    }

    *ok = 1;
    return calculator_result_read();
}

int main(void)
{
    char line[64];

    uart_ev_pending_write(0xffffffff);

    uart_puts_raw("\r\nCalculadora LiteX lista\r\n");
    uart_puts_raw("Ejemplos:\r\n");
    uart_puts_raw("  12+5\r\n");
    uart_puts_raw("  12-5\r\n");
    uart_puts_raw("  7*6\r\n");
    uart_puts_raw("  100/4\r\n");
    uart_puts_raw("  81sqr\r\n\r\n");

    while (1) {
        const char *p;
        uint32_t a = 0;
        uint32_t b = 0;
        uint32_t r = 0;
        char op;
        int ok = 0;

        uart_puts_raw("calc> ");
        read_line(line, sizeof(line));

        p = line;

        if (!parse_uint(&p, &a)) {
            uart_puts_raw("Entrada invalida\r\n");
            continue;
        }

        skip_spaces(&p);
        op = *p;

        if (op == '\0') {
            uart_puts_raw("Falta operador\r\n");
            continue;
        }

        p++;

        if (op == '+') {
            if (!parse_uint(&p, &b)) {
                uart_puts_raw("Falta segundo operando\r\n");
                continue;
            }

            r = calc_exec(a, b, OP_ADD, &ok);
        } else if (op == '-') {
            if (!parse_uint(&p, &b)) {
                uart_puts_raw("Falta segundo operando\r\n");
                continue;
            }

            r = calc_exec(a, b, OP_SUB, &ok);
        } else if (op == '*') {
            if (!parse_uint(&p, &b)) {
                uart_puts_raw("Falta segundo operando\r\n");
                continue;
            }

            r = calc_exec(a, b, OP_MUL, &ok);
        } else if (op == '/') {
            if (!parse_uint(&p, &b)) {
                uart_puts_raw("Falta segundo operando\r\n");
                continue;
            }

            r = calc_exec(a, b, OP_DIV, &ok);
        } else if (op == 's') {
            if (p[0] == 'q' && p[1] == 'r') {
                r = calc_exec(a, 0, OP_SQRT, &ok);
            } else {
                uart_puts_raw("Operacion no valida. Usa Asqr, ejemplo: 81sqr\r\n");
                continue;
            }
        } else {
            uart_puts_raw("Operacion no valida\r\n");
            continue;
        }

        if (!ok) {
            uart_puts_raw("Error o timeout en hardware\r\n");
            continue;
        }

        uart_puts_raw("Resultado: ");
        uart_put_uint(r);
        uart_puts_raw("\r\n");
    }

    return 0;
}