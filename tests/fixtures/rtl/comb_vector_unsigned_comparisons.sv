module vector_unsigned_comparisons (
    input wire [3:0] a,
    input wire [3:0] b,
    output logic equal_result,
    output logic not_equal_result,
    output logic less_result,
    output logic less_equal_result,
    output logic greater_result,
    output logic greater_equal_result
);
    assign equal_result = a == b;
    assign not_equal_result = a != b;
    assign less_result = a < b;
    assign less_equal_result = a <= b;
    assign greater_result = a > b;
    assign greater_equal_result = a >= b;
endmodule
