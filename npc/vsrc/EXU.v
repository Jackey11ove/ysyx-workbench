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


assign es_ready_go = 1'b1;
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
  end
end

assign alu_src1 = es_oprand1;
assign alu_src2 = es_oprand2;

assign shifter_src1 = es_oprand1;
assign shifter_src2 = es_oprand2;

assign es_mem_wdata = es_oprand2;
assign es_res_from_mem = es_is_Load || es_is_Loadu;

alu u_alu(es_alu_op, alu_src1, alu_src2, alu_result);
shifter u_shifter(es_shifter_op, shifter_src1, shifter_src2, shifter_result);

assign RW_result = es_Is_alu? { {32{alu_result[31]}},alu_result[31:0] } : { {32{shifter_result[31]}},shifter_result[31:0] };

assign es_result = es_Is_csr? es_csr_result : es_RWI_type? RW_result : es_Is_alu? alu_result : shifter_result;

endmodule