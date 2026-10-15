module signed_truncate (
    input logic signed [7:0] value,
    output wire signed [3:0] truncated
);
    assign truncated = value;
endmodule
