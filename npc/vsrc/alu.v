module alu(
  input  wire [13:0] alu_op,
  input  wire [63:0] alu_src1,
  input  wire [63:0] alu_src2,
  output wire [63:0] alu_result
);

wire op_add;   //add operation
wire op_sub;   //sub operation
wire op_slt;   //signed compared and set less than
wire op_sltu;  //unsigned compared and set less than
wire op_and;   //bitwise and
wire op_nor;   //bitwise nor
wire op_or;    //bitwise or
wire op_xor;   //bitwise xor
wire op_lui;   //Load Upper Immediate
wire op_mul;
wire op_div;
wire op_divu;
wire op_rem;
wire op_remu;

// control code decomposition
assign op_add  = alu_op[ 0];
assign op_sub  = alu_op[ 1];
assign op_slt  = alu_op[ 2];
assign op_sltu = alu_op[ 3];
assign op_and  = alu_op[ 4];
assign op_nor  = alu_op[ 5];
assign op_or   = alu_op[ 6];
assign op_xor  = alu_op[ 7];
assign op_lui  = alu_op[ 8];
assign op_mul  = alu_op[ 9];
assign op_div  = alu_op[10];
assign op_divu = alu_op[11];
assign op_rem  = alu_op[12];
assign op_remu = alu_op[13];

wire [63:0] add_sub_result;
wire [63:0] slt_result;
wire [63:0] sltu_result;
wire [63:0] and_result;
wire [63:0] nor_result;
wire [63:0] or_result;
wire [63:0] xor_result;
wire [63:0] lui_result;
wire [63:0] mul_result;
wire [63:0] div_result;
wire [63:0] divu_result;
wire [63:0] rem_result;
wire [63:0] remu_result;

// 32-bit adder
wire [63:0] adder_a;
wire [63:0] adder_b;
wire [64:0] adder_cin;
wire [63:0] adder_result;
wire        adder_cout;

assign adder_a   = alu_src1;
assign adder_b   = (op_sub | op_slt | op_sltu) ? ~alu_src2 : alu_src2;  //src1 - src2 rj-rk
assign adder_cin = (op_sub | op_slt | op_sltu) ? 65'b1      : 65'b0;
assign {adder_cout, adder_result} = adder_a + adder_b + adder_cin;

// ADD, SUB result
assign add_sub_result = adder_result;

// SLT result
assign slt_result[63:1] = 63'b0;   //rj < rk 1
assign slt_result[0]    = (alu_src1[63] & ~alu_src2[63])
                        | ((alu_src1[63] ^ ~alu_src2[63]) & adder_result[63]);  

// SLTU result
assign sltu_result[63:1] = 63'b0;
assign sltu_result[0]    = ~adder_cout;

// bitwise operation
assign and_result = alu_src1 & alu_src2;
assign or_result  = alu_src1 | alu_src2; 
assign nor_result = ~or_result;
assign xor_result = alu_src1 ^ alu_src2;

//lui result
assign lui_result = { {32{alu_src2[31]}},alu_src2[31:0] };

//mul & div
assign mul_result = alu_src1 * alu_src2;
assign div_result = $signed(alu_src1) / $signed(alu_src2);
assign rem_result = $signed(alu_src1) % $signed(alu_src2);
assign divu_result = alu_src1 / alu_src2;
assign remu_result = alu_src1 % alu_src2;

// final result mux
assign alu_result = ({64{op_add|op_sub}} & add_sub_result)
                  | ({64{op_slt       }} & slt_result)
                  | ({64{op_sltu      }} & sltu_result)
                  | ({64{op_and       }} & and_result)
                  | ({64{op_nor       }} & nor_result)
                  | ({64{op_or        }} & or_result)
                  | ({64{op_xor       }} & xor_result)
                  | ({64{op_lui       }} & lui_result)
                  | ({64{op_mul       }} & mul_result)
                  | ({64{op_div       }} & div_result)
                  | ({64{op_divu      }} & divu_result)
                  | ({64{op_rem       }} & rem_result)
                  | ({64{op_remu      }} & remu_result);

endmodule
