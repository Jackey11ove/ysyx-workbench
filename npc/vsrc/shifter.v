module shifter(
    input wire [5 :0] shifter_op,
    input wire [63:0] shifter_src1,
    input wire [63:0] shifter_src2,

    output wire [63:0] shifter_result
);

wire op_sll;
wire op_srl;
wire op_sra;
wire op_sllw;
wire op_srlw;
wire op_sraw;


assign op_sll  = shifter_op[0];
assign op_srl  = shifter_op[1];
assign op_sra  = shifter_op[2];
assign op_sllw = shifter_op[3];
assign op_srlw = shifter_op[4];
assign op_sraw = shifter_op[5];

wire [31:0] shifter_src1_32;

wire [63:0] sll_result;
wire [63:0] srl_result;
wire [63:0] sra_result;
wire [63:0] sllw_result;
wire [31:0] sllw_mid;
wire [63:0] srlw_result;
wire [31:0] srlw_mid;
wire [63:0] sraw_result;
wire [31:0] sraw_mid;

assign sll_result = shifter_src1 <<  shifter_src2[5:0];
assign srl_result = shifter_src1 >>  shifter_src2[5:0];
assign sra_result = $signed(shifter_src1) >>> shifter_src2[5:0];

assign shifter_src1_32 = shifter_src1[31:0];
assign sllw_mid = shifter_src1_32 <<  shifter_src2[4:0];
assign sllw_result = { {32{sllw_mid[31]}},sllw_mid };
assign srlw_mid = shifter_src1_32 >> shifter_src2[4:0];
assign srlw_result = { {32{srlw_mid[31]}},srlw_mid };
assign sraw_mid = $signed(shifter_src1_32) >>> shifter_src2[4:0];
assign sraw_result = { {32{sraw_mid[31]}},sraw_mid };


assign shifter_result = ({64{op_sll}} & sll_result)
                      | ({64{op_srl}} & srl_result)
                      | ({64{op_sra}} & sra_result)
                      | ({64{op_sllw}} & sllw_result)
                      | ({64{op_srlw}} & srlw_result)
                      | ({64{op_sraw}} & sraw_result);

endmodule