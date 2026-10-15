module combinational_vector_if_condition (
    input wire [3:0] condition,
    input wire [3:0] when_nonzero,
    input wire [3:0] when_zero,
    output logic [3:0] result
);
    always @(*) begin
        if (condition)
            result = when_nonzero;
        else
            result = when_zero;
    end
endmodule
