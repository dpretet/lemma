module combinational_composed_netlist (
    input wire [3:0] a,
    input wire [3:0] b,
    input wire [3:0] c,
    output logic [3:0] result_or,
    output logic [3:0] result_xor,
    input logic clk,
    input logic areset
);
    wire [3:0] shared;

    assign shared = a & b;
    always @(posedge clk or posedge areset) begin
        if (areset) begin
            result_or <= 4'b0;
            result_xor <= 4'b0;
        end else begin
            result_or <= shared | c;
            result_xor <= shared ^ c;
        end
    end
endmodule
