module vector_variable_selects (
    input wire [63:0] data,
    input wire [5:0] bit_index,
    input wire [1:0] io_selc,
    output logic selected_bit,
    output logic [31:0] selected_chunk,
    output logic [4:0] fixed_plus_select,
    output logic [4:0] fixed_range_select,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            selected_bit <= 1'b0;
            selected_chunk <= 32'b0;
            fixed_plus_select <= 5'b0;
            fixed_range_select <= 5'b0;
        end else begin
            selected_bit <= data[bit_index];
            selected_chunk <= data[8'd8 * {6'b0, io_selc} +: 32];
            fixed_plus_select <= data[3 +: 5];
            fixed_range_select <= data[7:3];
        end
    end
endmodule
