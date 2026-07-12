`timescale 1ns/1ps

/*
    i2c_master.v

    Master I2C basico para escritura de un byte.

    Transaccion implementada:

        START
        address[6:0] + W=0
        ACK
        data[7:0]
        ACK
        STOP

    Las salidas SCL/SDA se manejan como open-drain:

        *_oen = 1 -> liberar linea
        *_oen = 0 -> forzar linea a LOW

    Por tanto:
        SCL fisico = scl_oen ? Z : 0
        SDA fisico = sda_oen ? Z : 0

    En simulacion se modela con pull-up.
*/

module i2c_master #(
    parameter integer CLK_HZ = 60000000,
    parameter integer I2C_HZ = 100000
)(
    input  wire       clk,
    input  wire       reset,

    input  wire       start,
    input  wire [6:0] address,
    input  wire [7:0] data0,
    input  wire [7:0] data1,

    input  wire       sda_in,

    output reg        scl_oen,
    output reg        sda_oen,

    output reg        busy,
    output reg        done,
    output reg        ack_error
);

    localparam integer QUARTER_DIV_RAW = CLK_HZ / (I2C_HZ * 4);
    localparam integer QUARTER_DIV     = (QUARTER_DIV_RAW < 1) ? 1 : QUARTER_DIV_RAW;

    localparam S_IDLE       = 4'd0;
    localparam S_START_A    = 4'd1;
    localparam S_START_B    = 4'd2;
    localparam S_BIT_LOW    = 4'd3;
    localparam S_BIT_HIGH   = 4'd4;
    localparam S_ACK_LOW    = 4'd5;
    localparam S_ACK_HIGH   = 4'd6;
    localparam S_NEXT_BYTE  = 4'd7;
    localparam S_STOP_A     = 4'd8;
    localparam S_STOP_B     = 4'd9;
    localparam S_STOP_C     = 4'd10;
    localparam S_DONE       = 4'd11;

    reg [3:0]  state;
    reg [15:0] div_cnt;
    reg [7:0]  shift_reg;
    reg [3:0]  bit_cnt;
    reg [1:0] byte_sel;

    reg [7:0]  data0_latched;
    reg [7:0]  data1_latched;

    wire tick;

    assign tick = (div_cnt == QUARTER_DIV - 1);


    // -------------------------------------------------------------------------
    // Divisor de tiempo
    // -------------------------------------------------------------------------

    always @(posedge clk) begin

        if (reset) begin

            div_cnt <= 16'd0;

        end else if (busy) begin

            if (tick) begin
                div_cnt <= 16'd0;
            end else begin
                div_cnt <= div_cnt + 16'd1;
            end

        end else begin

            div_cnt <= 16'd0;

        end

    end


    // -------------------------------------------------------------------------
    // FSM I2C
    // -------------------------------------------------------------------------

    always @(posedge clk) begin

        if (reset) begin

            state        <= S_IDLE;
            shift_reg    <= 8'd0;
            bit_cnt      <= 4'd7;
            byte_sel     <= 2'd0;
            data0_latched <= 8'd0;
            data1_latched <= 8'd0;

            scl_oen      <= 1'b1;
            sda_oen      <= 1'b1;

            busy         <= 1'b0;
            done         <= 1'b0;
            ack_error    <= 1'b0;

        end else begin

            done <= 1'b0;


            case (state)

                S_IDLE: begin

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b1;
                    busy    <= 1'b0;

                    if (start) begin

                        shift_reg    <= {address, 1'b0};
                        data0_latched <= data0;
                        data1_latched <= data1;
                        bit_cnt      <= 4'd7;
                        byte_sel     <= 2'd0;

                        ack_error    <= 1'b0;
                        busy         <= 1'b1;

                        state        <= S_START_A;

                    end

                end


                S_START_A: begin

                    /*
                        START:
                        SDA baja mientras SCL permanece alto.
                    */

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b0;

                    if (tick) begin
                        state <= S_START_B;
                    end

                end


                S_START_B: begin

                    /*
                        Bajar SCL y preparar el primer bit.
                    */

                    scl_oen <= 1'b0;
                    sda_oen <= shift_reg[7] ? 1'b1 : 1'b0;

                    if (tick) begin
                        state <= S_BIT_HIGH;
                    end

                end


                S_BIT_LOW: begin

                    /*
                        Con SCL bajo se coloca el bit en SDA.
                    */

                    scl_oen <= 1'b0;
                    sda_oen <= shift_reg[7] ? 1'b1 : 1'b0;

                    if (tick) begin
                        state <= S_BIT_HIGH;
                    end

                end


                S_BIT_HIGH: begin

                    /*
                        Con SCL alto el esclavo lee el bit.
                    */

                    scl_oen <= 1'b1;
                    sda_oen <= shift_reg[7] ? 1'b1 : 1'b0;

                    if (tick) begin

                        if (bit_cnt == 4'd0) begin

                            state <= S_ACK_LOW;

                        end else begin

                            bit_cnt   <= bit_cnt - 4'd1;
                            shift_reg <= {shift_reg[6:0], 1'b0};
                            state     <= S_BIT_LOW;

                        end

                    end

                end


                S_ACK_LOW: begin

                    /*
                        ACK:
                        El master libera SDA.
                        El esclavo debe bajarla.
                    */

                    scl_oen <= 1'b0;
                    sda_oen <= 1'b1;

                    if (tick) begin
                        state <= S_ACK_HIGH;
                    end

                end


                S_ACK_HIGH: begin

                    /*
                        Leer ACK con SCL alto.
                        ACK = 0.
                        NACK = 1.
                    */

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b1;

                    if (tick) begin

                        if (sda_in) begin
                            ack_error <= 1'b1;
                        end

                        state <= S_NEXT_BYTE;

                    end

                end


                S_NEXT_BYTE: begin

                    scl_oen <= 1'b0;
                    sda_oen <= 1'b1;

                    if (tick) begin

                        if (byte_sel == 2'd0) begin

                            shift_reg <= data0_latched;
                            bit_cnt   <= 4'd7;
                            byte_sel  <= 2'd1;

                            state     <= S_BIT_LOW;

                        end else if (byte_sel == 2'd1) begin

                            shift_reg <= data1_latched;
                            bit_cnt   <= 4'd7;
                            byte_sel  <= 2'd2;

                            state     <= S_BIT_LOW;

                        end else begin

                            sda_oen <= 1'b0;
                            state   <= S_STOP_A;

                        end
                    end
                end



                S_STOP_A: begin

                    /*
                        Preparar STOP:
                        SCL bajo, SDA bajo.
                    */

                    scl_oen <= 1'b0;
                    sda_oen <= 1'b0;

                    if (tick) begin
                        state <= S_STOP_B;
                    end

                end


                S_STOP_B: begin

                    /*
                        Subir SCL manteniendo SDA bajo.
                    */

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b0;

                    if (tick) begin
                        state <= S_STOP_C;
                    end

                end


                S_STOP_C: begin

                    /*
                        STOP:
                        SDA sube mientras SCL esta alto.
                    */

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b1;

                    if (tick) begin
                        state <= S_DONE;
                    end

                end


                S_DONE: begin

                    scl_oen <= 1'b1;
                    sda_oen <= 1'b1;

                    busy    <= 1'b0;
                    done    <= 1'b1;

                    state   <= S_IDLE;

                end


                default: begin

                    state     <= S_IDLE;

                    scl_oen   <= 1'b1;
                    sda_oen   <= 1'b1;

                    busy      <= 1'b0;
                    done      <= 1'b0;
                    ack_error <= 1'b1;

                end

            endcase

        end

    end

endmodule