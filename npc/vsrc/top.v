import "DPI-C" function void get_cpu_inst(input int inst);
import "DPI-C" function void get_cpu_pc(input longint pc);
import "DPI-C" function void mem_read(input longint raddr, output longint rdata);
import "DPI-C" function void mem_write(input longint waddr, input longint wdata, input byte shift, input byte DWHB);

module top(
    input  wire        clk,
    input  wire        reset,
    // inst sram interface
    output wire [63:0] inst_sram_addr,
    output wire [31:0] inst_sram_rdata
);

//regfile
wire [4 :0] rf_raddr1;
wire [4 :0] rf_raddr2;
wire [63:0] rf_rdata1;
wire [63:0] rf_rdata2;
wire [4 :0] rf_waddr;
wire [63:0] rf_wdata;
wire [0 :0] rf_we;

//IFU
wire [63:0] nextpc;

//IDU
wire [31:0] Instruction;
wire [63:0] oprand1;
wire [63:0] oprand2;
wire [13:0] alu_op;
wire [5 :0] shifter_op;
wire Is_alu;
wire RWI_type;
wire op_mul;
wire op_div;
wire op_divu;
wire op_rem;
wire op_remu;
wire Load;
wire Loadu;
wire Store;
wire [3:0] DWHB;
wire [63:0]LS_addr;

wire Is_csr;
wire [63:0] csr_result;
wire Is_expc;
wire [63:0] ex_addr;
wire Is_mretpc;
wire [63:0] mret_addr;

wire ds_load_block;
wire Is_trans;
wire [63:0] trans_addr;
wire branch_taken_cancel;

//EXU
wire [63:0] es_result;

//流水级间信号
reg  [63:0] fs_pc;
reg  [63:0] ds_pc;
reg  [63:0] es_pc;
reg  [63:0] ms_pc;
reg  [63:0] ws_pc;
wire        ds_allowin;
wire        es_allowin;
wire        ms_allowin;
wire        ws_allowin;
wire        fs_to_ds_valid;
wire        ds_to_es_valid;
wire        es_to_ms_valid;
wire        ms_to_ws_valid;
reg         es_valid;
reg         ms_valid;
reg         ws_valid;
reg  [4 :0] es_rf_dest;
reg  [4 :0] ms_rf_dest;
reg  [4 :0] ws_rf_dest;
reg         es_rf_we;
reg         ms_rf_we;
reg         ws_rf_we;
reg  [31:0] ds_inst;
reg  [31:0] es_inst;
reg  [31:0] ms_inst;
reg  [31:0] ws_inst;
wire [63:0] es_result;
wire [63:0] ms_final_result;
wire [63:0] ws_final_result;
reg         es_is_Load;
reg         es_is_Loadu;
reg         es_is_Store;
wire        ms_LS_result_valid;
wire        es_related_cancel;
wire        es_res_from_mem;
wire        ms_res_from_mem;
reg  [3 :0] es_DWHB;
reg  [63:0] es_LS_addr;
wire [63:0] es_mem_wdata;


always @(*) begin
    get_cpu_inst(ws_inst);
end

always @(*) begin
    get_cpu_pc(ws_pc);
end

assign inst_sram_addr = fs_pc;
assign inst_sram_rdata = Instruction;
assign rf_wdata = ws_final_result;


regfile #(32,64) u_regfile(clk, rf_raddr1, rf_rdata1, rf_raddr2, rf_rdata2, ws_rf_dest, rf_wdata, ws_rf_we);
IFU u_IFU(clk, reset, ds_load_block, Is_trans, trans_addr, branch_taken_cancel, Is_expc, ex_addr, Is_mretpc, mret_addr, ds_allowin, Instruction, nextpc, fs_pc, fs_to_ds_valid);
IDU u_IDU(clk, reset, Instruction, fs_pc, es_allowin, fs_to_ds_valid, ds_to_es_valid, ds_pc, ds_inst, ds_allowin, ms_LS_result_valid, ms_res_from_mem, es_res_from_mem, es_rf_dest, ms_rf_dest, ws_rf_dest, es_rf_we, ms_rf_we, ws_rf_we, es_valid, ms_valid, ws_valid, es_result, ms_final_result, ws_final_result, 
          rf_rdata1, rf_rdata2, rf_raddr1, rf_raddr2, rf_waddr, rf_we, oprand1, oprand2, alu_op, shifter_op, Is_alu, RWI_type, op_mul, op_div, op_divu, op_rem, op_remu, Load, Loadu, Store, DWHB, LS_addr, ds_load_block, Is_trans, trans_addr, branch_taken_cancel, es_related_cancel, Is_csr, csr_result, Is_expc, ex_addr, Is_mretpc, mret_addr);
EXU u_EXU(clk, reset, ds_pc, ds_inst, ms_allowin, ds_to_es_valid, es_related_cancel, es_to_ms_valid, es_pc, es_inst, es_allowin, rf_waddr, rf_we, oprand1, oprand2, alu_op, shifter_op, Is_alu, RWI_type, op_mul, op_div, op_divu, op_rem, op_remu, Load, Loadu, Store, DWHB, LS_addr, 
          Is_csr, csr_result, es_valid, es_rf_we, es_rf_dest, es_is_Load, es_is_Loadu, es_res_from_mem, es_is_Store, es_DWHB, es_LS_addr, es_mem_wdata, es_result);
MEMU u_MEMU(clk, reset, es_pc, es_inst, ws_allowin, es_to_ms_valid, ms_LS_result_valid, ms_res_from_mem, ms_to_ws_valid, ms_pc, ms_inst, ms_allowin, ms_valid, ms_rf_we, ms_rf_dest, es_rf_we, es_rf_dest, es_is_Load, es_is_Loadu, es_is_Store, es_DWHB, es_LS_addr, es_mem_wdata, es_result, ms_final_result);
WBU u_WBU(clk, reset, ms_pc, ms_inst, ms_to_ws_valid, ms_rf_we, ms_rf_dest, ms_final_result, ws_pc, ws_inst, ws_allowin, ws_valid, ws_rf_we, ws_rf_dest, ws_final_result);

endmodule