module signed_to_unsigned_casts (
    input logic signed [3:0] signed_in,
    output wire [3:0] cast_to_unsigned,
    output wire [3:0] function_to_unsigned
);
    assign cast_to_unsigned = unsigned'(signed_in);
    assign function_to_unsigned = $unsigned(signed_in);
endmodule
