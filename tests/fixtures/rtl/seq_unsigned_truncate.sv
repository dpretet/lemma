module unsigned_truncate (
    input logic [7:0] value,
    output logic [3:0] truncated,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            truncated <= 4'b0;
        end else begin
            truncated <= value;
        end
    end
endmodule
