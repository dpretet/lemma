module signed_sign_extend (
    input logic signed [3:0] value,
    output logic signed [7:0] extended,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            extended <= 8'b0;
        end else begin
            extended <= value;
        end
    end
endmodule
