module vector_unsigned_subtract (
    input logic [3:0] a,
    input logic [3:0] b,
    output logic [3:0] difference,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            difference <= 4'b0;
        end else begin
            difference <= a - b;
        end
    end
endmodule
