module vector_unsigned_comparisons (
    input wire [3:0] a,
    input wire [3:0] b,
    output logic equal_result,
    output logic not_equal_result,
    output logic less_result,
    output logic less_equal_result,
    output logic greater_result,
    output logic greater_equal_result,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            equal_result <= 1'b0;
            not_equal_result <= 1'b0;
            less_result <= 1'b0;
            less_equal_result <= 1'b0;
            greater_result <= 1'b0;
            greater_equal_result <= 1'b0;
        end else begin
            equal_result <= a == b;
            not_equal_result <= a != b;
            less_result <= a < b;
            less_equal_result <= a <= b;
            greater_result <= a > b;
            greater_equal_result <= a >= b;
        end
    end
endmodule
