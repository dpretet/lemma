module combinational_case_mux (
    input logic [1:0] sel,
    input logic a,
    input logic b,
    input logic c,
    input logic d,
    output logic y
);
    always @(*) begin
        case (sel)
            2'b00: y = a;
            2'b01: y = b;
            2'b10: y = c;
            default: y = d;
        endcase
    end
endmodule
