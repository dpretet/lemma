module vector_unsigned_multiply (
    input logic [3:0] a,
    input logic [3:0] b,
    output logic [3:0] product,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            product <= 4'b0;
        end else begin
            product <= a * b;
        end
    end
endmodule
