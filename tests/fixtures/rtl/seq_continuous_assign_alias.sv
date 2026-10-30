module continuous_assign_alias (
    input wire a,
    output logic y,
    input logic clk,
    input logic areset
);
    always @(posedge clk or posedge areset) begin
        if (areset) begin
            y <= 1'b0;
        end else begin
            y <= a;
        end
    end
endmodule
