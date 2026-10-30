module vector_bitwise_operators (
    input logic [3:0] a,
    input logic [3:0] b,
    output logic [3:0] and_result,
    output logic [3:0] or_result,
    output logic [3:0] xor_result,
    output logic [3:0] xnor_tilde_caret_result,
    output logic [3:0] xnor_caret_tilde_result,
    output logic [3:0] complement_result,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            and_result <= 4'b0;
            or_result <= 4'b0;
            xor_result <= 4'b0;
            xnor_tilde_caret_result <= 4'b0;
            xnor_caret_tilde_result <= 4'b0;
            complement_result <= 4'b0;
        end else begin
            and_result <= a & b;
            or_result <= a | b;
            xor_result <= a ^ b;
            xnor_tilde_caret_result <= a ~^ b;
            xnor_caret_tilde_result <= a ^~ b;
            complement_result <= ~a;
        end
    end
endmodule
