`timescale 1ns/1ps

/*
    ws2812_framebuffer.v

    Driver WS2812B para enviar una imagen completa.

    La imagen está en un framebuffer externo:
        pixel 0  -> color 0
        pixel 1  -> color 1
        ...
        pixel 63 -> color 63

    Este driver no decide los colores.
    Solo recorre los LEDs y pide el color correspondiente.

    Funcionamiento:
        - pixel_addr indica qué pixel se está enviando.
        - pixel_color recibe el color de ese pixel.
        - El driver convierte RGB a GRB y genera la señal WS2812B.
*/

module ws2812_framebuffer #(
    parameter CLK_HZ   = 27000000,
    parameter NUM_LEDS = 64
)(
    input  wire        clk,
    input  wire        init,

    output reg  [5:0]  pixel_addr,
    input  wire [23:0] pixel_color,

    output reg         D_OUT,
    output reg         busy,
    output reg         done
);

    /*
        Para Colorlight i9 normalmente:
            clk = 27 MHz

        Ciclos aproximados:
            T0H ≈ 0.35 us
            T0L ≈ 0.80 us
            T1H ≈ 0.70 us
            T1L ≈ 0.60 us
            RESET > 50 us
    */

    localparam integer CYCLES_PER_US = CLK_HZ / 1000000;

    localparam integer T0H_CYCLES   = (CYCLES_PER_US * 35 + 50) / 100;
    localparam integer T0L_CYCLES   = (CYCLES_PER_US * 80 + 50) / 100;
    localparam integer T1H_CYCLES   = (CYCLES_PER_US * 70 + 50) / 100;
    localparam integer T1L_CYCLES   = (CYCLES_PER_US * 60 + 50) / 100;
    localparam integer RESET_CYCLES = CYCLES_PER_US * 60;

    localparam S_IDLE       = 4'd0;
    localparam S_LOAD       = 4'd1;
    localparam S_PIXEL_PREP = 4'd2;
    localparam S_SEND_HIGH  = 4'd3;
    localparam S_SEND_LOW   = 4'd4;
    localparam S_NEXT_BIT   = 4'd5;
    localparam S_NEXT_PIXEL = 4'd6;
    localparam S_RESET      = 4'd7;
    localparam S_DONE       = 4'd8;

    reg [3:0]  state;
    reg [15:0] time_cnt;
    reg [7:0]  led_cnt;
    reg [4:0]  bit_cnt;
    reg [23:0] shift_reg;

    wire bit_actual;
    wire [15:0] high_time;
    wire [15:0] low_time;

    assign bit_actual = shift_reg[23];

    assign high_time = bit_actual ? T1H_CYCLES[15:0] : T0H_CYCLES[15:0];
    assign low_time  = bit_actual ? T1L_CYCLES[15:0] : T0L_CYCLES[15:0];

    initial begin
        state      = S_IDLE;
        time_cnt   = 16'd0;
        led_cnt    = 8'd0;
        bit_cnt    = 5'd23;
        shift_reg  = 24'd0;
        pixel_addr = 6'd0;
        D_OUT      = 1'b0;
        busy       = 1'b0;
        done       = 1'b0;
    end

    always @(posedge clk) begin
        done <= 1'b0;

        case (state)

            S_IDLE: begin
                D_OUT    <= 1'b0;
                busy     <= 1'b0;
                time_cnt <= 16'd0;

                if (init) begin
                    state <= S_LOAD;
                end
            end

            S_LOAD: begin
                busy      <= 1'b1;
                led_cnt   <= 8'd0;
                pixel_addr <= 6'd0;
                time_cnt  <= 16'd0;
                state     <= S_PIXEL_PREP;
            end

            S_PIXEL_PREP: begin
                busy <= 1'b1;

                /*
                    pixel_color entra en RGB:
                        R = [23:16]
                        G = [15:8]
                        B = [7:0]

                    WS2812B transmite en GRB:
                        G, R, B
                */
                shift_reg <= {pixel_color[15:8], pixel_color[23:16], pixel_color[7:0]};

                bit_cnt  <= 5'd23;
                time_cnt <= 16'd0;
                state    <= S_SEND_HIGH;
            end

            S_SEND_HIGH: begin
                busy  <= 1'b1;
                D_OUT <= 1'b1;

                if (time_cnt >= high_time - 1) begin
                    time_cnt <= 16'd0;
                    state    <= S_SEND_LOW;
                end else begin
                    time_cnt <= time_cnt + 16'd1;
                end
            end

            S_SEND_LOW: begin
                busy  <= 1'b1;
                D_OUT <= 1'b0;

                if (time_cnt >= low_time - 1) begin
                    time_cnt <= 16'd0;
                    state    <= S_NEXT_BIT;
                end else begin
                    time_cnt <= time_cnt + 16'd1;
                end
            end

            S_NEXT_BIT: begin
                busy <= 1'b1;

                if (bit_cnt == 5'd0) begin
                    state <= S_NEXT_PIXEL;
                end else begin
                    bit_cnt   <= bit_cnt - 5'd1;
                    shift_reg <= {shift_reg[22:0], 1'b0};
                    state     <= S_SEND_HIGH;
                end
            end

            S_NEXT_PIXEL: begin
                busy <= 1'b1;

                if (led_cnt == NUM_LEDS - 1) begin
                    led_cnt  <= 8'd0;
                    time_cnt <= 16'd0;
                    state    <= S_RESET;
                end else begin
                    led_cnt    <= led_cnt + 8'd1;
                    pixel_addr <= led_cnt[5:0] + 6'd1;
                    state      <= S_PIXEL_PREP;
                end
            end

            S_RESET: begin
                busy  <= 1'b1;
                D_OUT <= 1'b0;

                if (time_cnt >= RESET_CYCLES[15:0] - 1) begin
                    time_cnt <= 16'd0;
                    state    <= S_DONE;
                end else begin
                    time_cnt <= time_cnt + 16'd1;
                end
            end

            S_DONE: begin
                busy  <= 1'b0;
                done  <= 1'b1;
                state <= S_IDLE;
            end

            default: begin
                state      <= S_IDLE;
                time_cnt   <= 16'd0;
                led_cnt    <= 8'd0;
                bit_cnt    <= 5'd23;
                shift_reg  <= 24'd0;
                pixel_addr <= 6'd0;
                D_OUT      <= 1'b0;
                busy       <= 1'b0;
                done       <= 1'b0;
            end

        endcase
    end

endmodule