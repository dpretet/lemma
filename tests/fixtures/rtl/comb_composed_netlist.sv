module combinational_composed_netlist (
    input wire [3:0] a,
    input wire [3:0] b,
    input wire [3:0] c,
    output logic [3:0] result_or,
    output logic [3:0] result_xor
);
    wire [3:0] shared;

    assign shared = a & b;
    assign result_or = shared | c;
    assign result_xor = shared ^ c;
endmodule
