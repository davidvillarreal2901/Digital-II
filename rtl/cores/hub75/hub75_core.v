module hub75_core #(
    parameter integer SYS_CLK_HZ   = 60000000,
    parameter integer PANEL_CLK_HZ = 6000000,
    parameter integer NUM_COLS     = 64,
    parameter integer NUM_ADDR     = 2048,
    parameter integer DELAY_VALUE  = 50
)(
    input   wire        clk,
    input   wire        reset,
    input   wire        blank,

    /*
     * Puerto de escritura del framebuffer.
     *
     * Hay 2048 palabras:
     *   32 parejas de filas x 64 columnas.
     *
     * Formato RGB444:
     *
     *   [23:20] R0
     *   [19:16] G0
     *   [15:12] B0
     *   [11:8]  R1
     *   [7:4]   G1
     *   [3:0]   B1
     */
    input   wire [10:0] pixel_addr,
    input   wire [23:0] pixel_data,
    input   wire        pixel_we,

    /*
     * Señales HUB75E.
     */
    output wire        LP_CLK,
    output wire        LATCH,
    output wire        NOE,
    output wire [4:0]  ROW,
    output wire [2:0]  RGB0,
    output wire [2:0]  RGB1
);


    // -------------------------------------------------------------------------
    // RELOJ DEL CONTROLADOR
    //
    // El diseño original recibía 25 MHz y generaba aproximadamente 6.25 MHz.
    // Aquí partimos de 60 MHz y generamos aproximadamente 6 MHz.
    // -------------------------------------------------------------------------

    localparam integer HALF_DIV_RAW =
        SYS_CLK_HZ / (PANEL_CLK_HZ * 2);

    localparam integer HALF_DIV =
        (HALF_DIV_RAW < 1) ? 1 : HALF_DIV_RAW;

    localparam integer DIV_WIDTH =
        (HALF_DIV <= 1) ? 1 : $clog2(HALF_DIV);


    reg [DIV_WIDTH-1:0] clk_counter;
    reg                 panel_clk;


    always @(posedge clk) begin

        if (reset) begin

            clk_counter <= {DIV_WIDTH{1'b0}};
            panel_clk   <= 1'b0;

        end else begin

            if (clk_counter == HALF_DIV - 1) begin

                clk_counter <= {DIV_WIDTH{1'b0}};
                panel_clk   <= ~panel_clk;

            end else begin

                clk_counter <= clk_counter + 1'b1;

            end

        end

    end


    // -------------------------------------------------------------------------
    // RESET DEL DOMINIO DEL PANEL
    //
    // Mantiene el controlador en reset durante varios flancos reales de
    // panel_clk después de liberar el reset principal.
    // -------------------------------------------------------------------------

    reg [2:0] panel_reset_count;


    always @(posedge panel_clk or posedge reset) begin

        if (reset) begin

            panel_reset_count <= 3'd0;

        end else if (!panel_reset_count[2]) begin

            panel_reset_count <= panel_reset_count + 1'b1;

        end

    end


    wire panel_reset;

    assign panel_reset =
        reset
        || !panel_reset_count[2];


    // -------------------------------------------------------------------------
    // SEÑALES INTERNAS DEL CONTROLADOR
    // -------------------------------------------------------------------------

    wire w_ZR;
    wire w_ZC;
    wire w_ZD;
    wire w_ZI;

    wire w_LD;
    wire w_SHD;

    wire w_RST_R;
    wire w_RST_C;
    wire w_RST_D;
    wire w_RST_I;

    wire w_INC_R;
    wire w_INC_C;
    wire w_INC_D;
    wire w_INC_I;

    wire [10:0] count_delay;
    wire [10:0] delay_value;

    wire [1:0] index;
    wire [5:0] column;

    wire [10:0] framebuffer_read_addr;
    wire [23:0] framebuffer_read_data;

    wire tmp_noe;
    wire tmp_latch;
    wire px_clk_en;


    assign framebuffer_read_addr = {
        ROW,
        column
    };


    /*
     * En el diseño original LATCH se invierte externamente.
     */
    assign LATCH = ~tmp_latch;
    
    /*
    * NOE es activo bajo.
    * blank=1 mantiene el panel apagado mientras se cambia el frame.
    */
    assign NOE = blank ? 1'b1 : tmp_noe;

    assign LP_CLK =
        panel_clk
        & px_clk_en;


    // -------------------------------------------------------------------------
    // FRAMEBUFFER
    //
    // ram_dual tiene puertos de dirección de 12 bits, pero el controlador
    // utiliza solamente 2048 palabras. Se agrega un cero como MSB.
    // -------------------------------------------------------------------------

    ram_dual video_ram (
        .clk    (clk),

        .we_a   (pixel_we),
        .addr_a ({1'b0, pixel_addr}),
        .data_a (pixel_data),

        .addr_b ({1'b0, framebuffer_read_addr}),
        .data_b (framebuffer_read_data)
    );


    // -------------------------------------------------------------------------
    // CONTADORES
    // -------------------------------------------------------------------------

    count #(
        .width(
            $clog2(NUM_ADDR / NUM_COLS) - 1
        )
    ) count_row (
        .clk   (panel_clk),
        .reset (w_RST_R),
        .inc   (w_INC_R),
        .outc  (ROW),
        .zero  (w_ZR)
    );


    count #(
        .width(
            $clog2(NUM_COLS) - 1
        )
    ) count_column (
        .clk   (panel_clk),
        .reset (w_RST_C),
        .inc   (w_INC_C),
        .outc  (column),
        .zero  (w_ZC)
    );


    count #(
        .width(10)
    ) count_pwm_delay (
        .clk   (panel_clk),
        .reset (w_RST_D),
        .inc   (w_INC_D),
        .outc  (count_delay),
        .zero  ()
    );


    count #(
        .width(1)
    ) count_bitplane (
        .clk   (panel_clk),
        .reset (w_RST_I),
        .inc   (w_INC_I),
        .outc  (index),
        .zero  (w_ZI)
    );


    // -------------------------------------------------------------------------
    // TEMPORIZACIÓN BCM
    // -------------------------------------------------------------------------

    lsr_led #(
        .init_value(DELAY_VALUE),
        .width(10)
    ) pwm_delay_shift (
        .clk   (panel_clk),
        .load  (w_LD),
        .shift (w_SHD),
        .s_A   (delay_value)
    );


    comp_4k #(
        .width(10)
    ) pwm_delay_compare (
        .in1 (delay_value),
        .in2 (count_delay),
        .out (w_ZD)
    );


    // -------------------------------------------------------------------------
    // SELECCIÓN DE LOS PLANOS RGB444
    // -------------------------------------------------------------------------

    mux_led rgb_bitplane_mux (
        .in0  (framebuffer_read_data),
        .sel  (index),
        .out0 ({RGB0, RGB1})
    );


    // -------------------------------------------------------------------------
    // FSM DEL PANEL
    // -------------------------------------------------------------------------

    ctrl_lp4k panel_controller (
        .clk       (panel_clk),
        .init      (1'b1),
        .rst       (panel_reset),

        .ZR        (w_ZR),
        .ZC        (w_ZC),
        .ZD        (w_ZD),
        .ZI        (w_ZI),

        .RST_R     (w_RST_R),
        .RST_C     (w_RST_C),
        .RST_D     (w_RST_D),
        .RST_I     (w_RST_I),

        .INC_R     (w_INC_R),
        .INC_C     (w_INC_C),
        .INC_D     (w_INC_D),
        .INC_I     (w_INC_I),

        .LD        (w_LD),
        .SHD       (w_SHD),

        .LATCH     (tmp_latch),
        .NOE       (tmp_noe),
        .PX_CLK_EN (px_clk_en)
    );


endmodule