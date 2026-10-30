module unsigned_truncate (
    input logic [7:0] value,
    output wire [3:0] truncated
);
    assign truncated = value;
endmodule
