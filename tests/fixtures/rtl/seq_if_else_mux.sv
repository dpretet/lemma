module combinational_if_else_mux (
    input wire sel,
    input wire a,
    input wire b,
    output reg y,
    input logic clk,
    input logic areset
);
    always @(posedge clk or posedge areset) begin
        if (areset) begin
            y <= 1'b0;
        end else begin
            if (sel)
                y <= a;
            else
                y <= b;
        end
    end
endmodule
