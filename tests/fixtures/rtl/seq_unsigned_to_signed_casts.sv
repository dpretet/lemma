module unsigned_to_signed_casts (
    input logic [3:0] unsigned_in,
    output logic signed [3:0] cast_to_signed,
    output logic signed [3:0] function_to_signed,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            cast_to_signed <= 4'b0;
            function_to_signed <= 4'b0;
        end else begin
            cast_to_signed <= signed'(unsigned_in);
            function_to_signed <= $signed(unsigned_in);
        end
    end
endmodule
