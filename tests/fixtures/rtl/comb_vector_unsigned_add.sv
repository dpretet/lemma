module vector_unsigned_add (
    input logic [3:0] a,
    input logic [3:0] b,
    output wire [3:0] sum
);
    assign sum = a + b;
endmodule
