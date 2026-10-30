module vector_unsigned_multiply (
    input logic [3:0] a,
    input logic [3:0] b,
    output wire [3:0] product
);
    assign product = a * b;
endmodule
