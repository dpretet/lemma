module combinational_always_comb_mux (
    input logic sel,
    input logic a,
    input logic b,
    output logic y,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            y <= 1'b0;
        end else begin
            if (sel)
                y <= a;
            else
                y <= b;
        end
    end
endmodule
