module vector_width_signedness_alias (
    input logic signed [3:0] signed_in,
    input logic [3:0] unsigned_in,
    output wire signed [3:0] signed_out,
    output wire [3:0] unsigned_out
);
    assign signed_out = signed_in;
    assign unsigned_out = unsigned_in;
endmodule
