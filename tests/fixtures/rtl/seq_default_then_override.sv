module combinational_default_then_override (
    input logic a,
    input logic b,
    output logic c,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            c <= 1'b0;
        end else begin
            c <= 1'b0;
            if (a)
                c <= b;
        end
    end
endmodule
