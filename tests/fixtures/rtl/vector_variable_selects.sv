module vector_variable_selects (
    input wire [63:0] data,
    input wire [5:0] bit_index,
    input wire [1:0] io_selc,
    output logic selected_bit,
    output logic [31:0] selected_chunk,
    output logic [4:0] fixed_plus_select,
    output logic [4:0] fixed_range_select
);
    assign selected_bit = data[bit_index];
    assign selected_chunk = data[8'd8 * {6'b0, io_selc} +: 32];
    assign fixed_plus_select = data[3 +: 5];
    assign fixed_range_select = data[7:3];
endmodule
