module unsigned_zero_extend (
    input logic [3:0] value,
    output wire [7:0] extended
);
    assign extended = value;
endmodule
