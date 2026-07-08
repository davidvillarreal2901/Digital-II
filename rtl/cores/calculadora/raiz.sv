module raiz (
    input  logic        clk,
    input  logic        rst_n,
    input  logic        iniciar,
    input  logic [15:0] radicando,
    output logic [15:0] raiz_res,
    output logic        terminado
);

    logic [31:0] op_actual;
    logic [15:0] raiz_temp;
    logic [15:0] uno;
    logic        ocupado;

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            terminado <= 0; ocupado <= 0; raiz_res <= 0;
        end else begin
            if (iniciar && !ocupado) begin
                op_actual <= radicando;
                raiz_temp <= 0;
                uno <= 1 << 14; // Bit más signifdicativo (para 16 bits)
                ocupado <= 1;
                terminado <= 0;
            end else if (ocupado) begin
                if (uno != 0) begin
                    if (op_actual >= (raiz_temp + uno)) begin
                        op_actual <= op_actual - (raiz_temp + uno);
                        raiz_temp <= (raiz_temp >> 1) + uno;
                    end else begin
                        raiz_temp <= raiz_temp >> 1;
                    end
                    uno <= uno >> 2;
                end else begin
                    raiz_res <= raiz_temp;
                    terminado <= 1;
                    ocupado <= 0;
                end
            end
        end
    end
endmodule