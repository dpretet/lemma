module combinational_if_else_mux (
    input wire sel,
    input wire a,
    input wire b,
    output reg y
);
    always @(*) begin
        if (sel)
            y = a;
        else
            y = b;
    end
endmodule
