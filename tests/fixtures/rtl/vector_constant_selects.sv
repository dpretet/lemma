module vector_constant_selects (
    input wire [7:0] value,
    output logic selected_bit,
    output logic [3:0] selected_part
);
    assign selected_bit = value[3];
    assign selected_part = value[5:2];
endmodule
