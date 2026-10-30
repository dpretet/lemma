module vector_reduction_operators (
    input logic [3:0] value,
    output wire and_result,
    output wire nand_result,
    output wire or_result,
    output wire nor_result,
    output wire xor_result,
    output wire xnor_tilde_caret_result,
    output wire xnor_caret_tilde_result
);
    assign and_result = &value;
    assign nand_result = ~&value;
    assign or_result = |value;
    assign nor_result = ~|value;
    assign xor_result = ^value;
    assign xnor_tilde_caret_result = ~^value;
    assign xnor_caret_tilde_result = ^~value;
endmodule
