module vector_unsigned_add (
    input logic [3:0] a,
    input logic [3:0] b,
    output logic [3:0] sum,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            sum <= 4'b0;
        end else begin
            sum <= a + b;
        end
    end
endmodule
