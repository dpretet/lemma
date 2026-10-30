module continuous_assign_boolean (
    input wire a,
    input wire b,
    input wire c,
    output logic y,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            y <= 1'b0;
        end else begin
            y <= (a && b) || !c;
        end
    end
endmodule
