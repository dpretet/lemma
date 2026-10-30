module combinational_assignment_order (
    input logic a,
    input logic b,
    input logic c,
    input logic x,
    input logic z,
    output logic y
);
    always @(*) begin
        if (a)
            y = x;
        else
            y = z;

        if (c)
            y = b;
    end
endmodule
