`timescale 1ns/1ps


module ws2812_matrix_tb;


    // -------------------------------------------------------------------------
    // Configuracion
    // -------------------------------------------------------------------------

    localparam integer CLK_HZ     = 60_000_000;
    localparam integer NUM_LEDS   = 64;
    localparam integer TOTAL_BITS = NUM_LEDS * 24;

    localparam integer T0H_CYCLES = 21;
    localparam integer T1H_CYCLES = 42;

    localparam integer RESET_MIN_CYCLES = 3600;


    // -------------------------------------------------------------------------
    // DUT
    // -------------------------------------------------------------------------

    reg         clk;
    reg         reset;

    reg  [5:0]  pixel_index;
    reg  [23:0] pixel_color;
    reg         pixel_we;

    reg         start;

    wire        D_OUT;
    wire        busy;
    wire        done;


    ws2812_matrix_core #(
        .CLK_HZ   (CLK_HZ),
        .NUM_LEDS (NUM_LEDS)
    ) dut (
        .clk         (clk),
        .reset       (reset),

        .pixel_index (pixel_index),
        .pixel_color (pixel_color),
        .pixel_we    (pixel_we),

        .start       (start),

        .D_OUT       (D_OUT),
        .busy        (busy),
        .done        (done)
    );


    // -------------------------------------------------------------------------
    // Clock 60 MHz
    // -------------------------------------------------------------------------

    initial begin
        clk = 1'b0;
    end

    always #8.333 clk = ~clk;


    // -------------------------------------------------------------------------
    // Variables de validacion
    // -------------------------------------------------------------------------

    integer errors;
    integer pulse_count;
    integer high_count;
    integer low_after_last;
    integer timeout;

    integer current_pixel;
    integer current_bit;
    integer expected_cycles;

    reg [23:0] expected_word;
    reg        expected_bit;

    reg previous_dout;
    reg tail_started;


    // -------------------------------------------------------------------------
    // Imagen esperada
    //
    // Pixel 0  = rojo
    // Pixel 1  = verde
    // Pixel 63 = azul
    //
    // La entrada al core es RGB.
    // Aquí usamos directamente el valor GRB esperado en la salida serial.
    // -------------------------------------------------------------------------

    function automatic [23:0] expected_grb(
        input integer pixel
    );

        begin

            case (pixel)

                // RGB 0x200000
                // GRB 0x002000
                0:
                    expected_grb = 24'h002000;


                // RGB 0x002000
                // GRB 0x200000
                1:
                    expected_grb = 24'h200000;


                // RGB 0x000020
                // GRB 0x000020
                63:
                    expected_grb = 24'h000020;


                default:
                    expected_grb = 24'h000000;

            endcase

        end

    endfunction


    // -------------------------------------------------------------------------
    // Escritura de pixel
    // -------------------------------------------------------------------------

    task automatic write_pixel(
        input [5:0]  index,
        input [23:0] color
    );

        begin

            @(negedge clk);

            pixel_index = index;
            pixel_color = color;
            pixel_we    = 1'b1;


            @(negedge clk);

            pixel_we = 1'b0;

        end

    endtask


    // -------------------------------------------------------------------------
    // Pulso START
    // -------------------------------------------------------------------------

    task automatic pulse_start;

        begin

            @(negedge clk);

            start = 1'b1;


            @(negedge clk);

            start = 1'b0;

        end

    endtask


    // -------------------------------------------------------------------------
    // Monitor serial
    //
    // Cada pulso HIGH corresponde a un bit WS2812.
    //
    // 0:
    //     HIGH = 21 ciclos
    //
    // 1:
    //     HIGH = 42 ciclos
    // -------------------------------------------------------------------------

    always @(negedge clk) begin

        if (D_OUT) begin
            high_count = high_count + 1;
        end


        // ---------------------------------------------------------------------
        // Flanco de bajada
        // ---------------------------------------------------------------------

        if (
            previous_dout
            && !D_OUT
        ) begin

            if (pulse_count < TOTAL_BITS) begin

                current_pixel = pulse_count / 24;
                current_bit   = 23 - (pulse_count % 24);

                expected_word = expected_grb(
                    current_pixel
                );

                expected_bit = expected_word[
                    current_bit
                ];


                if (expected_bit) begin
                    expected_cycles = T1H_CYCLES;
                end else begin
                    expected_cycles = T0H_CYCLES;
                end


                if (high_count != expected_cycles) begin

                    $display(
                        "[ERROR] pixel=%0d bit=%0d valor=%0d HIGH=%0d esperado=%0d",
                        current_pixel,
                        current_bit,
                        expected_bit,
                        high_count,
                        expected_cycles
                    );

                    errors = errors + 1;

                end


                pulse_count = pulse_count + 1;


                if (pulse_count == TOTAL_BITS) begin

                    tail_started = 1'b1;

                end

            end


            high_count = 0;

        end


        // ---------------------------------------------------------------------
        // Tiempo LOW posterior al ultimo bit
        // ---------------------------------------------------------------------

        if (
            tail_started
            && !D_OUT
            && !done
        ) begin

            low_after_last = low_after_last + 1;

        end


        previous_dout = D_OUT;

    end


    // -------------------------------------------------------------------------
    // Test principal
    // -------------------------------------------------------------------------

    initial begin

        $dumpfile(
            "ws2812_matrix_tb.vcd"
        );

        $dumpvars(
            0,
            ws2812_matrix_tb
        );


        errors          = 0;
        pulse_count     = 0;
        high_count      = 0;
        low_after_last  = 0;

        previous_dout   = 1'b0;
        tail_started    = 1'b0;


        reset       = 1'b1;

        pixel_index = 6'd0;
        pixel_color = 24'd0;
        pixel_we    = 1'b0;

        start       = 1'b0;


        repeat (10) @(negedge clk);


        reset = 1'b0;


        repeat (10) @(negedge clk);


        $display("");
        $display("========================================");
        $display(" TESTBENCH WS2812 MATRIZ 8x8");
        $display("========================================");


        // ---------------------------------------------------------------------
        // Preparar framebuffer
        // ---------------------------------------------------------------------

        write_pixel(
            6'd0,
            24'h200000
        );


        write_pixel(
            6'd1,
            24'h002000
        );


        write_pixel(
            6'd63,
            24'h000020
        );


        // ---------------------------------------------------------------------
        // Iniciar envio
        // ---------------------------------------------------------------------

        pulse_start();


        // ---------------------------------------------------------------------
        // Verificar BUSY
        // ---------------------------------------------------------------------

        timeout = 0;

        while (
            !busy
            && timeout < 100
        ) begin

            @(negedge clk);

            timeout = timeout + 1;

        end


        if (!busy) begin

            $display(
                "[ERROR] BUSY nunca se activo"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] BUSY activo"
            );

        end


        // ---------------------------------------------------------------------
        // Intentar modificar framebuffer durante transmision
        //
        // El core debe ignorar esta escritura.
        //
        // Pixel 63 debe continuar siendo AZUL.
        // ---------------------------------------------------------------------

        write_pixel(
            6'd63,
            24'h200000
        );


        // ---------------------------------------------------------------------
        // Esperar DONE
        // ---------------------------------------------------------------------

        timeout = 0;

        while (
            !done
            && timeout < 200000
        ) begin

            @(negedge clk);

            timeout = timeout + 1;

        end


        if (!done) begin

            $display(
                "[ERROR] Timeout esperando DONE"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] DONE recibido"
            );

        end


        // ---------------------------------------------------------------------
        // Numero total de bits
        // ---------------------------------------------------------------------

        if (pulse_count != TOTAL_BITS) begin

            $display(
                "[ERROR] Bits enviados=%0d esperado=%0d",
                pulse_count,
                TOTAL_BITS
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] Bits enviados=%0d",
                pulse_count
            );

        end


        // ---------------------------------------------------------------------
        // BUSY debe quedar inactivo
        // ---------------------------------------------------------------------

        if (busy) begin

            $display(
                "[ERROR] BUSY permanece activo"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] BUSY inactivo al finalizar"
            );

        end


        // ---------------------------------------------------------------------
        // RESET / LATCH
        // ---------------------------------------------------------------------

        if (
            low_after_last
            < RESET_MIN_CYCLES
        ) begin

            $display(
                "[ERROR] RESET LOW=%0d ciclos esperado >=%0d",
                low_after_last,
                RESET_MIN_CYCLES
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] RESET LOW=%0d ciclos",
                low_after_last
            );

        end


        // ---------------------------------------------------------------------
        // DONE debe permanecer activo
        // ---------------------------------------------------------------------

        repeat (10) @(negedge clk);


        if (!done) begin

            $display(
                "[ERROR] DONE no permanece activo"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] DONE sticky"
            );

        end


        // ---------------------------------------------------------------------
        // Nuevo START limpia DONE
        // ---------------------------------------------------------------------

        pulse_start();


        @(negedge clk);


        if (done) begin

            $display(
                "[ERROR] START no limpio DONE"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] START limpia DONE"
            );

        end


        // ---------------------------------------------------------------------
        // Resultado
        // ---------------------------------------------------------------------

        $display("");
        $display("========================================");


        if (errors == 0) begin

            $display(
                " TODOS LOS TESTS PASARON"
            );

        end else begin

            $display(
                " TESTS FALLIDOS: %0d",
                errors
            );

        end


        $display("========================================");
        $display("");


        $finish;

    end


endmodule