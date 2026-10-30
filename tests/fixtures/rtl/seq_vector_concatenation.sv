module vector_concatenation (
    input wire [1:0] upper,
    input wire [2:0] lower,
    output logic [4:0] combined,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            combined <= 5'b0;
        end else begin
            combined <= {upper, lower};
        end
    end
endmodule
