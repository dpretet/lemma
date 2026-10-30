module signed_to_unsigned_casts (
    input logic signed [3:0] signed_in,
    output logic [3:0] cast_to_unsigned,
    output logic [3:0] function_to_unsigned,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            cast_to_unsigned <= 4'b0;
            function_to_unsigned <= 4'b0;
        end else begin
            cast_to_unsigned <= unsigned'(signed_in);
            function_to_unsigned <= $unsigned(signed_in);
        end
    end
endmodule
