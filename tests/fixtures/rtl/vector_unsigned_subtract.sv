module vector_unsigned_subtract (
    input logic [3:0] a,
    input logic [3:0] b,
    output wire [3:0] difference
);
    assign difference = a - b;
endmodule
