module vector_reduction_operators (
    input logic [3:0] value,
    output logic and_result,
    output logic nand_result,
    output logic or_result,
    output logic nor_result,
    output logic xor_result,
    output logic xnor_tilde_caret_result,
    output logic xnor_caret_tilde_result,
    input logic clk,
    input logic aresetn
);
    always @(posedge clk or negedge aresetn) begin
        if (!aresetn) begin
            and_result <= 1'b0;
            nand_result <= 1'b0;
            or_result <= 1'b0;
            nor_result <= 1'b0;
            xor_result <= 1'b0;
            xnor_tilde_caret_result <= 1'b0;
            xnor_caret_tilde_result <= 1'b0;
        end else begin
            and_result <= &value;
            nand_result <= ~&value;
            or_result <= |value;
            nor_result <= ~|value;
            xor_result <= ^value;
            xnor_tilde_caret_result <= ~^value;
            xnor_caret_tilde_result <= ^~value;
        end
    end
endmodule
