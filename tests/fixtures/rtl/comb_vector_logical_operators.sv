module vector_logical_operators (
    input wire [3:0] a,
    input wire [3:0] b,
    output logic logical_not,
    output logic logical_and,
    output logic logical_or
);
    assign logical_not = !a;
    assign logical_and = a && b;
    assign logical_or = a || b;
endmodule
