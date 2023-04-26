import "DPI-C" function void inst_fetch(input longint raddr, output int rdata);
import "DPI-C" function void get_cpu_pc(input longint pc);
import "DPI-C" function void get_cpu_inst(input int inst);

module top(
    input  wire        clk,
    input  wire        reset,
    // inst sram interface
    output wire [63:0] inst_sram_addr,
    output wire [31:0] inst_sram_rdata
);

reg  [63:0] pc;
wire [31:0] Instruction;

//regfile
wire [4 :0] rf_raddr1;
wire [4 :0] rf_raddr2;
wire [63:0] rf_rdata1;
wire [63:0] rf_rdata2;
wire [4 :0] rf_waddr;
wire [63:0] rf_wdata;
wire [0 :0] rf_we;

//IDU
wire [63:0] oprand1;
wire [63:0] oprand2;
wire [13:0] alu_op;
wire [8 :0] shifter_op;
wire Is_alu;
wire RWI_type;
wire Load;
wire Loadu;
wire Store;
wire [3:0] DWHB;
wire [7:0] mask;
wire [63:0]LS_addr;

wire Is_trans;
wire [63:0] trans_addr;

//EXU
wire [63:0] exu_result;

always @(*) begin
    inst_fetch(pc,Instruction);
    get_cpu_pc(pc);
    get_cpu_inst(Instruction);
end

assign inst_sram_addr = pc;
assign inst_sram_rdata = Instruction;
assign rf_wdata = exu_result;


regfile #(32,64) u_regfile(clk, rf_raddr1, rf_rdata1, rf_raddr2, rf_rdata2, rf_waddr, rf_wdata, rf_we);
IFU u_IFU(clk, reset, Is_trans, trans_addr, pc);
IDU u_IDU(Instruction, pc, rf_rdata1, rf_rdata2, rf_raddr1, rf_raddr2, rf_waddr, rf_we, oprand1, oprand2, alu_op, shifter_op, Is_alu, RWI_type, Load, Loadu, Store, DWHB, mask, LS_addr, Is_trans, trans_addr);
EXU u_EXU(pc, oprand1, oprand2, alu_op, shifter_op, Is_alu, RWI_type, Load, Loadu, Store, DWHB, mask, LS_addr, exu_result);

endmodule
