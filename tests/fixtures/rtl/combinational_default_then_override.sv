module combinational_default_then_override (
    input logic a,
    input logic b,
    output logic c
);
    always_comb begin
        c = 1'b0;
        if (a)
            c = b;
    end
endmodule
