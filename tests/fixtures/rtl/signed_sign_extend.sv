module signed_sign_extend (
    input logic signed [3:0] value,
    output wire signed [7:0] extended
);
    assign extended = value;
endmodule
