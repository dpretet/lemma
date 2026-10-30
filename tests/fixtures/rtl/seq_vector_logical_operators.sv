module vector_logical_operators (
    input wire [3:0] a,
    input wire [3:0] b,
    output logic logical_not,
    output logic logical_and,
    output logic logical_or,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            logical_not <= 1'b0;
            logical_and <= 1'b0;
            logical_or <= 1'b0;
        end else begin
            logical_not <= !a;
            logical_and <= a && b;
            logical_or <= a || b;
        end
    end
endmodule
