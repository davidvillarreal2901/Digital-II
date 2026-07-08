module divisor (
    input  logic        clk,
    input  logic        rst_n,
    input  logic        iniciar,
    input  logic [15:0] dividendo,
    input  logic [15:0] divisor,
    output logic [31:0] cociente,
    output logic        terminado
);
    typedef enum logic [2:0] {REPOSO, INICIO, DESPLAZAR, VERIFICAR, CHECK_FIN, FIN} estados_t;
    estados_t estado;

    logic [31:0] reg_a;
    logic [15:0] reg_b;
    logic [4:0]  iteracion;
    logic [15:0] diferencia;

    assign diferencia = reg_a[31:16] - reg_b;

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            terminado <= 0; cociente <= 0; estado <= REPOSO;
            reg_a <= 0; reg_b <= 0; iteracion <= 0;
        end else begin
            case (estado)
                REPOSO: begin
                    terminado <= 0;
                    if (iniciar) estado <= INICIO;
                end

                INICIO: begin
                    reg_a <= {16'b0, dividendo};
                    reg_b <= divisor;
                    iteracion <= 16;
                    estado <= DESPLAZAR;
                end

                DESPLAZAR: begin
                    reg_a <= reg_a << 1;
                    iteracion <= iteracion - 1;
                    estado <= VERIFICAR;
                end

                VERIFICAR: begin
                    // Si la parte alta es mayor o igual al divisor, restamos
                    if (reg_a[31:16] >= reg_b) begin
                        reg_a[31:16] <= reg_a[31:16] - reg_b;
                        reg_a[0] <= 1; // Ponemos un 1 en el bit de cociente
                    end
                    estado <= CHECK_FIN;
                end

                CHECK_FIN: begin
                    if (iteracion == 0) estado <= FIN;
                    else estado <= DESPLAZAR;
                end

                FIN: begin
                    cociente <= {16'd0, reg_a[15:0]};
                    terminado <= 1'b1;
                    estado <= REPOSO;
                end
            endcase
        end
    end
endmodule