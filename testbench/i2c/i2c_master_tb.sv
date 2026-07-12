`timescale 1ns/1ps


module i2c_master_tb;


    localparam integer CLK_HZ = 60000000;
    localparam integer I2C_HZ = 100000;


    reg clk;
    reg reset;

    reg start;
    reg [6:0] address;
    reg [7:0] data0;
    reg [7:0] data1;

    wire scl_oen;
    wire sda_oen;

    wire busy;
    wire done;
    wire ack_error;


    wire scl;
    wire sda;

    reg slave_drive_ack;


    /*
        Modelo open-drain con pull-up ideal.

        Si nadie baja la linea, vale 1.
        Si el master o el esclavo la bajan, vale 0.
    */

    assign scl = scl_oen ? 1'b1 : 1'b0;

    assign sda = (
        (!sda_oen)
        || slave_drive_ack
    ) ? 1'b0 : 1'b1;


    i2c_master #(
        .CLK_HZ (CLK_HZ),
        .I2C_HZ (I2C_HZ)
    ) dut (
        .clk       (clk),
        .reset     (reset),

        .start     (start),
        .address   (address),
        .data0     (data0),
        .data1     (data1),

        .sda_in    (sda),

        .scl_oen   (scl_oen),
        .sda_oen   (sda_oen),

        .busy      (busy),
        .done      (done),
        .ack_error (ack_error)
    );


    initial begin
        clk = 1'b0;
    end

    always #8.333 clk = ~clk;


    integer errors;
    integer timeout;

    reg [7:0] captured_addr;
    reg [7:0] captured_data0;
    reg [7:0] captured_data1;

    integer ack_count;


    // -------------------------------------------------------------------------
    // Pulso START hacia el DUT
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
    // Esperar START I2C real
    //
    // START = SDA baja mientras SCL esta alto.
    // -------------------------------------------------------------------------

    task automatic wait_i2c_start;

        begin

            while (
                !(
                    scl == 1'b1
                    && sda == 1'b0
                )
            ) begin
                @(posedge clk);
            end

        end

    endtask


    // -------------------------------------------------------------------------
    // Leer un byte I2C
    //
    // El dato se captura en flanco de subida de SCL.
    // -------------------------------------------------------------------------

    task automatic read_i2c_byte(
        output reg [7:0] value
    );

        integer bit_index;

        begin

            value = 8'd0;


            for (
                bit_index = 7;
                bit_index >= 0;
                bit_index = bit_index - 1
            ) begin

                @(posedge scl);

                value[bit_index] = sda;

            end

        end

    endtask


    // -------------------------------------------------------------------------
    // Generar ACK del esclavo
    //
    // ACK = el esclavo baja SDA durante el noveno pulso de SCL.
    // -------------------------------------------------------------------------

    task automatic send_ack;

        begin

            /*
                Esperar a que el master baje SCL despues del octavo bit.
                En ese momento el master debe liberar SDA para ACK.
            */

            @(negedge scl);

            slave_drive_ack = 1'b1;


            /*
                El master sube SCL para muestrear ACK.
            */

            @(posedge scl);

            ack_count = ack_count + 1;


            /*
                Soltar SDA despues del pulso de ACK.
            */

            @(negedge scl);

            slave_drive_ack = 1'b0;

        end

    endtask


    // -------------------------------------------------------------------------
    // Esperar STOP I2C real
    //
    // STOP = SDA sube mientras SCL esta alto.
    // -------------------------------------------------------------------------

    task automatic wait_i2c_stop;

        begin

            while (
                !(
                    scl == 1'b1
                    && sda == 1'b1
                    && busy == 1'b0
                )
            ) begin
                @(posedge clk);
            end

        end

    endtask


    // -------------------------------------------------------------------------
    // Modelo de esclavo I2C
    //
    // Espera esta transaccion:
    //
    // START
    // address + W
    // ACK
    // data0
    // ACK
    // data1
    // ACK
    // STOP
    // -------------------------------------------------------------------------

    task automatic fake_i2c_slave;

        begin

            wait_i2c_start();


            read_i2c_byte(
                captured_addr
            );

            send_ack();


            read_i2c_byte(
                captured_data0
            );

            send_ack();


            read_i2c_byte(
                captured_data1
            );

            send_ack();


            wait_i2c_stop();

        end

    endtask


    // -------------------------------------------------------------------------
    // Test principal
    // -------------------------------------------------------------------------

    initial begin

        $dumpfile(
            "i2c_master_tb.vcd"
        );

        $dumpvars(
            0,
            i2c_master_tb
        );


        errors = 0;

        reset  = 1'b1;
        start  = 1'b0;

        /*
            Para SSD1306 usual:

                address = 0x3C

            En bus I2C se transmite:

                0x3C << 1 | W = 0x78

            Payload de prueba:

                data0 = 0x00  -> control byte: comando
                data1 = 0xAE  -> comando: display OFF
        */

        address = 7'h3C;
        data0   = 8'h00;
        data1   = 8'hAE;

        slave_drive_ack = 1'b0;

        captured_addr  = 8'd0;
        captured_data0 = 8'd0;
        captured_data1 = 8'd0;

        ack_count = 0;


        repeat (20) @(negedge clk);

        reset = 1'b0;

        repeat (20) @(negedge clk);


        $display("");
        $display("========================================");
        $display(" TESTBENCH I2C MASTER");
        $display("========================================");


        if (
            scl !== 1'b1
            || sda !== 1'b1
        ) begin

            $display(
                "[ERROR] Bus no esta libre en IDLE"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] Bus libre en IDLE"
            );

        end


        fork
            fake_i2c_slave();
        join_none


        pulse_start();


        timeout = 0;

        while (
            !busy
            && timeout < 1000
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


        timeout = 0;

        while (
            !done
            && timeout < 150000
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


        /*
            Dar unos ciclos para que la FSM vuelva a IDLE
            y libere completamente el bus.
        */

        repeat (4) @(negedge clk);


        if (ack_error) begin

            $display(
                "[ERROR] ACK error activo"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] ACK recibido"
            );

        end


        if (captured_addr !== 8'h78) begin

            $display(
                "[ERROR] Address capturado = 0x%02h esperado 0x78",
                captured_addr
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] Address capturado = 0x%02h",
                captured_addr
            );

        end


        if (captured_data0 !== 8'h00) begin

            $display(
                "[ERROR] Data0 capturada = 0x%02h esperado 0x00",
                captured_data0
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] Data0 capturada = 0x%02h",
                captured_data0
            );

        end


        if (captured_data1 !== 8'hAE) begin

            $display(
                "[ERROR] Data1 capturada = 0x%02h esperado 0xAE",
                captured_data1
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] Data1 capturada = 0x%02h",
                captured_data1
            );

        end


        if (ack_count != 3) begin

            $display(
                "[ERROR] ACKs capturados=%0d esperado 3",
                ack_count
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] ACKs capturados=%0d",
                ack_count
            );

        end


        if (
            scl !== 1'b1
            || sda !== 1'b1
        ) begin

            $display(
                "[ERROR] Bus no queda libre despues de STOP"
            );

            errors = errors + 1;

        end else begin

            $display(
                "[OK] STOP libera el bus"
            );

        end


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