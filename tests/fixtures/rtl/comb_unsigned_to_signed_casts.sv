module unsigned_to_signed_casts (
    input logic [3:0] unsigned_in,
    output wire signed [3:0] cast_to_signed,
    output wire signed [3:0] function_to_signed
);
    assign cast_to_signed = signed'(unsigned_in);
    assign function_to_signed = $signed(unsigned_in);
endmodule
