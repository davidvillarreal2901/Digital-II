module multiplicador (
    input  logic        clk,
    input  logic        rst_n,
    input  logic        iniciar,
    input  logic [15:0] operando_a,
    input  logic [15:0] operando_b,
    output logic [31:0] producto,
    output logic        terminado
);
    logic [15:0] b_temp;
    logic [4:0]  contador;
    logic        ocupado;

    always_ff @(posedge clk) begin
        if (!rst_n) begin
            terminado <= 0; ocupado <= 0; producto <= 0; b_temp <= 0; contador <= 0;
        end else begin
            if (iniciar && !ocupado) begin
                b_temp   <= operando_b;
                producto <= 0;
                ocupado  <= 1;
                terminado <= 0;
                contador <= 0;
            end else if (ocupado) begin
                if (contador == 16) begin
                    terminado <= 1;
                    ocupado   <= 0;
                end else begin
                    if (b_temp[0]) begin
                        // Convertimos a 32 bits (32') ANTES de desplazar para no perder datos
                        producto <= producto + (32'(operando_a) << contador);
                    end
                    b_temp   <= b_temp >> 1;
                    contador <= contador + 1;
                end
            end
        end
    end
endmodule