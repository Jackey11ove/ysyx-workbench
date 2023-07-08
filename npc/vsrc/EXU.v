module EXU (
    input  wire        clk,
    input  wire        reset,
    input  reg  [63:0] ds_pc,
    input  wire [31:0] ds_inst,

    input  wire        ms_allowin,
    input  wire        ds_to_es_valid,
    input  wire        es_related_cancel,
    output wire        es_to_ms_valid,
    output reg  [63:0] es_pc,
    output reg  [31:0] es_inst,
    output wire        es_allowin,

    input  wire [4 :0] rf_dest,
    input  wire        rf_we,
    input  wire [63:0] oprand1,
    input  wire [63:0] oprand2,

    input  wire [13:0] alu_op,
    input  wire [5 :0] shifter_op,
    input  wire        Is_alu,
    input  wire        RWI_type,
    input  wire        op_mul,
    input  wire        op_div,
    input  wire        op_divu,
    input  wire        op_rem,
    input  wire        op_remu,

    input  wire        Load,
    input  wire        Loadu,
    input  wire        Store,
    input  wire [3 :0] DWHB,
    input  wire [63:0] LS_addr,

    input  wire        Is_csr,
    input  wire [63:0] csr_result,

    output reg         es_valid,
    output reg         es_rf_we,
    output reg  [4 :0] es_rf_dest,
    output reg         es_is_Load,
    output reg         es_is_Loadu,
    output wire        es_res_from_mem,
    output reg         es_is_Store,
    output reg  [3 :0] es_DWHB,
    output reg  [63:0] es_LS_addr,
    output wire [63:0] es_mem_wdata,
    output wire [63:0] es_result
);

wire        es_ready_go;

reg  [63:0] es_oprand1;
reg  [63:0] es_oprand2;
reg         es_Is_alu;
reg         es_RWI_type;
reg  [13:0] es_alu_op;
reg  [5 :0] es_shifter_op;

reg         es_Is_csr;
reg  [63:0] es_csr_result;

wire [63:0] alu_src1;
wire [63:0] alu_src2;
wire [63:0] alu_result;

wire [63:0] shifter_src1;
wire [63:0] shifter_src2;
wire [63:0] shifter_result;

wire [63:0] RW_result;

//乘除法器相关信号
//乘除法指令进入的第一个周期与乘除法器进行握手，ID传来的乘除法相关的信号只会保持一个周期，下一个周期就会变，需要保存相关信号
reg         mul_valid;    //为高表示数据有效，如果没有新的乘法输入，在乘法被接受的下一个周期要置低
wire [63:0] multiplicand; //被乘数
wire [63:0] multiplier;   //乘数
reg         mul_ready;    //为高表示乘法器准备好，表示可以输入数据
reg         mul_out_valid;    //为高表示乘法器输出结果有效
wire [63:0] result_hi;    //乘法结果的高位
wire [63:0] result_lo;    //乘法结果的低位

//除法器
reg         div_valid;     //为高表示数据有效，如果没有新的除法输入，在除法被接受的下一个周期要置低
wire [63:0] dividend;      //被除数
wire [63:0] divisor;       //除数
wire        div_signed;    //符号
reg         div_ready;     //为高表示除法器准备好，表示可以输入数据
reg         div_out_valid; //为高表示除法器输出结果有效
wire [63:0] quotient;      //商
wire [63:0] remainder;     //余数

reg  es_op_mul;
reg  es_op_div;
reg  es_op_divu;
reg  es_op_rem;
reg  es_op_remu;

//es_wait = (es_op_mul && !mul_out_valid) || ( (es_op_div|es_op_divu|es_op_rem|es_op_remu) && !div_out_valid )
assign es_ready_go = !( (es_op_mul && !mul_out_valid) || ( (es_op_div|es_op_divu|es_op_rem|es_op_remu) && !div_out_valid ) );
assign es_allowin = !es_valid || (es_ready_go && ms_allowin);
assign es_to_ms_valid = es_valid && es_ready_go;

always @(posedge clk)begin
  if(reset)begin
    es_valid <= 1'b0;
  end
  else if(es_related_cancel && ms_allowin)begin
    es_valid <= 1'b0;
  end
  else if(es_allowin)begin
    es_valid <= ds_to_es_valid;
  end
end

always @(posedge clk)begin
  if(ds_to_es_valid && es_allowin)begin
    es_pc <= ds_pc;
    es_inst <= ds_inst;
    es_alu_op <= alu_op;
    es_shifter_op <= shifter_op;
    es_oprand1 <= oprand1;
    es_oprand2 <= oprand2;
    es_Is_alu <= Is_alu;
    es_RWI_type <= RWI_type;
    es_rf_we <= rf_we;
    es_rf_dest <= rf_dest;
    es_is_Load <= Load;
    es_is_Loadu <= Loadu;
    es_is_Store <= Store;
    es_DWHB <= DWHB;
    es_LS_addr <= LS_addr;
    es_Is_csr <= Is_csr;
    es_csr_result <= csr_result;
    es_op_mul <= op_mul;
    es_op_div <= op_div;
    es_op_divu <= op_divu;
    es_op_rem <= op_rem;
    es_op_remu <= op_remu;
  end
end

always @(posedge clk) begin
  if(reset)begin
    mul_valid <= 1'b0;
  end
  else if(ds_to_es_valid && es_allowin)begin
    mul_valid <= op_mul;
  end
  else if(!op_mul)begin
    mul_valid <= 1'b0;
  end
end

always @(posedge clk) begin
  if(reset)begin
    div_valid <= 1'b0;
  end
  else if(ds_to_es_valid && es_allowin)begin
    div_valid <= (op_div | op_divu | op_rem | op_remu);
  end
  else if( !(op_div | op_divu | op_rem | op_remu) )begin
    div_valid <= 1'b0;
  end
end

assign alu_src1 = es_oprand1;
assign alu_src2 = es_oprand2;

assign shifter_src1 = es_oprand1;
assign shifter_src2 = es_oprand2;

assign multiplicand = es_oprand1;
assign multiplier = es_oprand2;

assign dividend = es_oprand1;
assign divisor = es_oprand2;
assign div_signed = es_op_div | es_op_rem;

assign es_mem_wdata = es_oprand2;
assign es_res_from_mem = es_is_Load || es_is_Loadu;

alu u_alu(es_alu_op, alu_src1, alu_src2, alu_result);
shifter u_shifter(es_shifter_op, shifter_src1, shifter_src2, shifter_result);
mul u_mul(clk, reset, mul_valid, multiplicand, multiplier, mul_ready, mul_out_valid, result_hi, result_lo);
div u_div(clk, reset, div_valid, dividend, divisor, div_signed, div_ready, div_out_valid, quotient, remainder);

assign RW_result = (es_op_div|es_op_divu)? { {32{quotient[31]}},quotient[31:0] } :
                   (es_op_rem|es_op_remu)? { {32{remainder[31]}},remainder[31:0] } :
                                es_op_mul? { {32{result_lo[31]}},result_lo[31:0] } :
                                es_Is_alu? { {32{alu_result[31]}},alu_result[31:0] } : { {32{shifter_result[31]}},shifter_result[31:0] };

assign es_result = es_Is_csr? es_csr_result : 
                   es_RWI_type? RW_result : 
                  (es_op_div|es_op_divu)? quotient :
                  (es_op_rem|es_op_remu)? remainder :
                   es_op_mul? result_lo :
                   es_Is_alu? alu_result : shifter_result;

endmodule