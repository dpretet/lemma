module vector_concatenation (
    input wire [1:0] upper,
    input wire [2:0] lower,
    output logic [4:0] combined
);
    assign combined = {upper, lower};
endmodule
