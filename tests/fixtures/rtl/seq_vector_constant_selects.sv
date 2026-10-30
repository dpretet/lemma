module vector_constant_selects (
    input wire [7:0] value,
    output logic selected_bit,
    output logic [3:0] selected_part,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            selected_bit <= 1'b0;
            selected_part <= 4'b0;
        end else begin
            selected_bit <= value[3];
            selected_part <= value[5:2];
        end
    end
endmodule
