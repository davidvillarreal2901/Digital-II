`timescale 1ns/1ps

module calc_core_tb;

    localparam OP_ADD  = 3'd0;
    localparam OP_SUB  = 3'd1;
    localparam OP_MUL  = 3'd2;
    localparam OP_DIV  = 3'd3;
    localparam OP_SQRT = 3'd4;

    logic        clk;
    logic        rst;

    logic [31:0] op1;
    logic [31:0] op2;
    logic [2:0]  opcode;
    logic        start;

    logic [31:0] result;
    logic        done;
    logic        busy;
    logic        error;

    integer errores;


    // ------------------------------------------------------------
    // DUT
    // ------------------------------------------------------------

    calc_core dut (
        .clk    (clk),
        .rst    (rst),

        .op1    (op1),
        .op2    (op2),
        .opcode (opcode),
        .start  (start),

        .result (result),
        .done   (done),
        .busy   (busy),
        .error  (error)
    );


    // ------------------------------------------------------------
    // RELOJ
    //
    // LiteX sys_clk = 60 MHz
    // T = 16.666 ns
    // ------------------------------------------------------------

    initial begin
        clk = 1'b0;

        forever #8.333 clk = ~clk;
    end


    // ------------------------------------------------------------
    // TAREA DE EJECUCION
    //
    // Reproduce el comportamiento actual del firmware:
    //
    // calculator_start_write(1);
    // delay(100);
    // calculator_start_write(0);
    //
    // En el wrapper LiteX:
    //
    //     i_start = self.start.re
    //
    // Cada escritura genera un pulso sobre start.
    // ------------------------------------------------------------

    task automatic ejecutar_operacion (
        input logic [2:0]  op,
        input logic [31:0] a,
        input logic [31:0] b,
        input logic [31:0] esperado,
        input logic        error_esperado
    );

        integer timeout;

        begin

            // ----------------------------------------------------
            // Configurar operandos y operacion
            // ----------------------------------------------------

            @(negedge clk);

            opcode = op;
            op1     = a;
            op2     = b;


            // ----------------------------------------------------
            // Primera escritura al CSR START
            // ----------------------------------------------------

            start = 1'b1;

            @(negedge clk);

            start = 1'b0;


            // ----------------------------------------------------
            // Equivalente funcional al delay(100) del firmware
            // ----------------------------------------------------

            repeat (100) @(negedge clk);


            // ----------------------------------------------------
            // Segunda escritura al CSR START
            // ----------------------------------------------------

            start = 1'b1;

            @(negedge clk);

            start = 1'b0;


            // ----------------------------------------------------
            // Esperar terminacion
            // ----------------------------------------------------

            timeout = 0;

            while ((done !== 1'b1) && (timeout < 200)) begin
                @(negedge clk);

                timeout = timeout + 1;
            end


            // ----------------------------------------------------
            // Validacion
            // ----------------------------------------------------

            if (timeout >= 200) begin

                $display(
                    "[ERROR] Timeout: opcode=%0d op1=%0d op2=%0d",
                    op,
                    a,
                    b
                );

                errores = errores + 1;

            end
            else if (error !== error_esperado) begin

                $display(
                    "[ERROR] Error flag incorrecto: opcode=%0d op1=%0d op2=%0d error=%0b esperado=%0b",
                    op,
                    a,
                    b,
                    error,
                    error_esperado
                );

                errores = errores + 1;

            end
            else if (!error_esperado && (result !== esperado)) begin

                $display(
                    "[ERROR] Resultado incorrecto: opcode=%0d op1=%0d op2=%0d result=%0d esperado=%0d",
                    op,
                    a,
                    b,
                    result,
                    esperado
                );

                errores = errores + 1;

            end
            else begin

                $display(
                    "[OK] opcode=%0d op1=%0d op2=%0d result=%0d error=%0b ciclos=%0d",
                    op,
                    a,
                    b,
                    result,
                    error,
                    timeout
                );

            end


            // ----------------------------------------------------
            // Tiempo entre comandos
            //
            // En la implementacion real, las operaciones son
            // solicitadas por el usuario mediante UART. Entre la
            // visualizacion de un resultado y el ingreso del
            // siguiente comando transcurren multiples ciclos.
            // ----------------------------------------------------

            repeat (200) @(negedge clk);

        end

    endtask


    // ------------------------------------------------------------
    // TESTS
    // ------------------------------------------------------------

    initial begin

        $dumpfile("calc_core_tb.vcd");
        $dumpvars(0, calc_core_tb);

        errores = 0;

        rst     = 1'b1;
        op1     = 32'd0;
        op2     = 32'd0;
        opcode  = OP_ADD;
        start   = 1'b0;


        // --------------------------------------------------------
        // RESET
        // --------------------------------------------------------

        repeat (5) @(negedge clk);

        rst = 1'b0;

        repeat (2) @(negedge clk);


        $display("");
        $display("========================================");
        $display(" TESTBENCH CALCULADORA");
        $display("========================================");


        // --------------------------------------------------------
        // SUMA
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_ADD,
            32'd12,
            32'd5,
            32'd17,
            1'b0
        );


        // --------------------------------------------------------
        // RESTA
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_SUB,
            32'd12,
            32'd5,
            32'd7,
            1'b0
        );


        // --------------------------------------------------------
        // MULTIPLICACION
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_MUL,
            32'd7,
            32'd6,
            32'd42,
            1'b0
        );


        // --------------------------------------------------------
        // DIVISION EXACTA
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_DIV,
            32'd100,
            32'd4,
            32'd25,
            1'b0
        );


        // --------------------------------------------------------
        // DIVISION ENTERA
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_DIV,
            32'd7,
            32'd2,
            32'd3,
            1'b0
        );


        ejecutar_operacion(
            OP_DIV,
            32'd10,
            32'd3,
            32'd3,
            1'b0
        );


        // --------------------------------------------------------
        // MAXIMO OPERANDO DE 16 BITS
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_DIV,
            32'd65535,
            32'd1,
            32'd65535,
            1'b0
        );


        // --------------------------------------------------------
        // RAIZ CUADRADA
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_SQRT,
            32'd81,
            32'd0,
            32'd9,
            1'b0
        );


        ejecutar_operacion(
            OP_SQRT,
            32'd144,
            32'd0,
            32'd12,
            1'b0
        );


        // --------------------------------------------------------
        // DIVISION POR CERO
        // --------------------------------------------------------

        ejecutar_operacion(
            OP_DIV,
            32'd10,
            32'd0,
            32'd0,
            1'b1
        );


        // --------------------------------------------------------
        // RESULTADO FINAL
        // --------------------------------------------------------

        $display("");
        $display("========================================");

        if (errores == 0) begin
            $display(" TODOS LOS TESTS PASARON");
        end
        else begin
            $display(" TESTS FALLIDOS: %0d", errores);
        end

        $display("========================================");
        $display("");

        $finish;

    end

endmodule