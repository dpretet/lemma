module combinational_if_else_boolean (
    input wire a,
    input wire b,
    input wire c,
    output reg y
);
    always @(*) begin
        if (a || b)
            y = !c;
        else
            y = 1'b0;
    end
endmodule
