module vector_bitwise_operators (
    input logic [3:0] a,
    input logic [3:0] b,
    output wire [3:0] and_result,
    output wire [3:0] or_result,
    output wire [3:0] xor_result,
    output wire [3:0] xnor_tilde_caret_result,
    output wire [3:0] xnor_caret_tilde_result,
    output wire [3:0] complement_result
);
    assign and_result = a & b;
    assign or_result = a | b;
    assign xor_result = a ^ b;
    assign xnor_tilde_caret_result = a ~^ b;
    assign xnor_caret_tilde_result = a ^~ b;
    assign complement_result = ~a;
endmodule
