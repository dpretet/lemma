module combinational_if_else_boolean (
    input wire a,
    input wire b,
    input wire c,
    output reg y,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            y <= 1'b0;
        end else begin
            if (a || b)
                y <= !c;
            else
                y <= 1'b0;
        end
    end
endmodule
