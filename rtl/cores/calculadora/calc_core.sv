module calc_core (
    input  logic        clk,
    input  logic        rst,

    input  logic [31:0] op1,
    input  logic [31:0] op2,
    input  logic [2:0]  opcode,
    input  logic        start,

    output logic [31:0] result,
    output logic        done,
    output logic        busy,
    output logic        error
);

    localparam OP_ADD  = 3'd0;
    localparam OP_SUB  = 3'd1;
    localparam OP_MUL  = 3'd2;
    localparam OP_DIV  = 3'd3;
    localparam OP_SQRT = 3'd4;

    typedef enum logic [1:0] {
        IDLE,
        WAIT_RESULT
    } state_t;

    state_t state;

    logic rst_n;
    logic [2:0] active_opcode;

    assign rst_n = ~rst;

    logic start_mult;
    logic start_div;
    logic start_raiz;

    logic [31:0] res_mult;
    logic [31:0] res_div;
    logic [15:0] res_raiz;

    logic fin_mult;
    logic fin_div;
    logic fin_raiz;

    multiplicador u_mult (
        .clk(clk),
        .rst_n(rst_n),
        .iniciar(start_mult),
        .operando_a(op1[15:0]),
        .operando_b(op2[15:0]),
        .producto(res_mult),
        .terminado(fin_mult)
    );

    divisor u_div (
        .clk(clk),
        .rst_n(rst_n),
        .iniciar(start_div),
        .dividendo(op1[15:0]),
        .divisor(op2[15:0]),
        .cociente(res_div),
        .terminado(fin_div)
    );

    raiz u_raiz (
        .clk(clk),
        .rst_n(rst_n),
        .iniciar(start_raiz),
        .radicando(op1[15:0]),
        .raiz_res(res_raiz),
        .terminado(fin_raiz)
    );

    always_ff @(posedge clk) begin
        if (rst) begin
            state         <= IDLE;
            active_opcode <= OP_ADD;
            result        <= 32'd0;
            done          <= 1'b0;
            busy          <= 1'b0;
            error         <= 1'b0;
            start_mult    <= 1'b0;
            start_div     <= 1'b0;
            start_raiz    <= 1'b0;
        end else begin
            start_mult <= 1'b0;
            start_div  <= 1'b0;
            start_raiz <= 1'b0;

            case (state)

                IDLE: begin
                    busy <= 1'b0;

                    if (start) begin
                        active_opcode <= opcode;
                        done          <= 1'b0;
                        error         <= 1'b0;
                        busy          <= 1'b1;

                        case (opcode)

                            OP_ADD: begin
                                result <= op1 + op2;
                                done   <= 1'b1;
                                busy   <= 1'b0;
                            end

                            OP_SUB: begin
                                result <= op1 - op2;
                                done   <= 1'b1;
                                busy   <= 1'b0;
                            end

                            OP_MUL: begin
                                start_mult <= 1'b1;
                                state      <= WAIT_RESULT;
                            end

                            OP_DIV: begin
                                if (op2 == 0) begin
                                    result <= 32'd0;
                                    error  <= 1'b1;
                                    done   <= 1'b1;
                                    busy   <= 1'b0;
                                end else begin
                                    start_div <= 1'b1;
                                    state     <= WAIT_RESULT;
                                end
                            end

                            OP_SQRT: begin
                                start_raiz <= 1'b1;
                                state      <= WAIT_RESULT;
                            end

                            default: begin
                                result <= 32'd0;
                                error  <= 1'b1;
                                done   <= 1'b1;
                                busy   <= 1'b0;
                            end

                        endcase
                    end
                end

                WAIT_RESULT: begin
                    case (active_opcode)

                        OP_MUL: begin
                            if (fin_mult) begin
                                result <= res_mult;
                                done   <= 1'b1;
                                busy   <= 1'b0;
                                state  <= IDLE;
                            end
                        end

                        OP_DIV: begin
                            if (fin_div) begin
                                result <= res_div;
                                done   <= 1'b1;
                                busy   <= 1'b0;
                                state  <= IDLE;
                            end
                        end

                        OP_SQRT: begin
                            if (fin_raiz) begin
                                result <= {16'd0, res_raiz};
                                done   <= 1'b1;
                                busy   <= 1'b0;
                                state  <= IDLE;
                            end
                        end

                        default: begin
                            error <= 1'b1;
                            done  <= 1'b1;
                            busy  <= 1'b0;
                            state <= IDLE;
                        end

                    endcase
                end

            endcase
        end
    end

endmodule