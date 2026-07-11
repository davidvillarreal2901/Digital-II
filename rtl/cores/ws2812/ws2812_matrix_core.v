`timescale 1ns/1ps

/*
    ws2812_matrix_core.v

    Controlador para una matriz WS2812B de 8x8.

    El módulo contiene un framebuffer de 64 píxeles RGB.

    Flujo de uso:

        1. Seleccionar pixel_index.
        2. Presentar pixel_color.
        3. Generar un pulso pixel_we.
        4. Repetir para los píxeles requeridos.
        5. Generar un pulso start.
        6. Esperar done.

    Formato de color:

        pixel_color[23:16] = R
        pixel_color[15:8]  = G
        pixel_color[7:0]   = B

    El módulo ws2812_framebuffer realiza internamente
    la conversión RGB -> GRB requerida para transmisión.
*/

module ws2812_matrix_core #(
    parameter integer CLK_HZ   = 60000000,
    parameter integer NUM_LEDS = 64
)(
    input  wire        clk,
    input  wire        reset,

    input  wire [5:0]  pixel_index,
    input  wire [23:0] pixel_color,
    input  wire        pixel_we,

    input  wire        start,

    output wire        D_OUT,
    output wire        busy,
    output reg         done
);

    // ------------------------------------------------------------
    // Framebuffer
    // ------------------------------------------------------------

    reg [23:0] framebuffer [0:63];

    integer i;


    // ------------------------------------------------------------
    // Interfaz con transmisor WS2812
    // ------------------------------------------------------------

    wire [5:0]  tx_pixel_addr;
    wire [23:0] tx_pixel_color;

    wire tx_busy;
    wire tx_done;


    assign tx_pixel_color = framebuffer[tx_pixel_addr];

    assign busy = tx_busy;


    // ------------------------------------------------------------
    // Escritura del framebuffer
    // ------------------------------------------------------------

    always @(posedge clk) begin

        if (reset) begin

            done <= 1'b0;

            for (i = 0; i < NUM_LEDS; i = i + 1) begin
                framebuffer[i] <= 24'h000000;
            end

        end else begin

            // ----------------------------------------------------
            // Escritura de píxel
            //
            // Se bloquea mientras se transmite la imagen para
            // evitar modificar el framebuffer durante el envío.
            // ----------------------------------------------------

            if (pixel_we && !tx_busy) begin
                framebuffer[pixel_index] <= pixel_color;
            end


            // ----------------------------------------------------
            // Flag de finalización
            //
            // done permanece activo hasta el siguiente START.
            // ----------------------------------------------------

            if (start) begin
                done <= 1'b0;
            end

            if (tx_done) begin
                done <= 1'b1;
            end

        end

    end


    // ------------------------------------------------------------
    // Transmisor WS2812
    // ------------------------------------------------------------

    ws2812_framebuffer #(
        .CLK_HZ   (CLK_HZ),
        .NUM_LEDS (NUM_LEDS)
    ) ws2812_tx (
        .clk         (clk),
        .init        (start),

        .pixel_addr  (tx_pixel_addr),
        .pixel_color (tx_pixel_color),

        .D_OUT       (D_OUT),
        .busy        (tx_busy),
        .done        (tx_done)
    );


endmodule