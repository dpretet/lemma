module combinational_assignment_order (
    input logic a,
    input logic b,
    input logic c,
    input logic x,
    input logic z,
    output logic y,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            y <= 1'b0;
        end else begin
            if (a)
                y <= x;
            else
                y <= z;

            if (c)
                y <= b;
        end
    end
endmodule
