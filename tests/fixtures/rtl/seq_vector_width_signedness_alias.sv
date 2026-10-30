module vector_width_signedness_alias (
    input logic signed [3:0] signed_in,
    input logic [3:0] unsigned_in,
    output logic signed [3:0] signed_out,
    output logic [3:0] unsigned_out,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            signed_out <= 4'b0;
            unsigned_out <= 4'b0;
        end else begin
            signed_out <= signed_in;
            unsigned_out <= unsigned_in;
        end
    end
endmodule
