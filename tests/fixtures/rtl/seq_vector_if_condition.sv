module combinational_vector_if_condition (
    input wire [3:0] condition,
    input wire [3:0] when_nonzero,
    input wire [3:0] when_zero,
    output logic [3:0] result,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            result <= 4'b0;
        end else begin
            if (condition)
                result <= when_nonzero;
            else
                result <= when_zero;
        end
    end
endmodule
