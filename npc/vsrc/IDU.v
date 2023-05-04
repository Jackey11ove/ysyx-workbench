module IDU(
    input  wire clk,
    input  wire reset,
    input  wire [31:0] Instruction,
    input  reg  [63:0] pc,

    input  wire [63:0] rf_src1,
    input  wire [63:0] rf_src2,
    output wire [4 :0] rs1,
    output wire [4 :0] rs2,
    output wire [4 :0] rf_dest,
    output wire rf_we,

    output wire [63:0] oprand1,
    output wire [63:0] oprand2,
    output wire [13:0] alu_op,
    output wire [5 :0] shifter_op,
    output wire Is_alu, //告诉执行级阶段操作是alu还是shifter
    output wire RWI_type,

    output wire Load,
    output wire Loadu,
    output wire Store,
    output wire [3:0] DWHB, //告诉执行级访存指令的类型
    output wire [7:0] mask,
    output wire [63:0] LS_addr,

    output wire Is_trans, //判断是否跳转的信号
    output wire [63:0] trans_addr, //跳转的地址
    
    output wire Is_csr,
    output wire [63:0] csr_result,

    output wire Is_expc,
    output wire [63:0] ex_addr,
    output wire Is_mretpc,
    output wire [63:0] mret_addr
);

wire [63:0] current_pc;

wire [6 :0] opcode;
wire [2 :0] funct3;
wire [6 :0] funct7;
wire [11:0] funct12;

wire [63:0] imm;
wire [63:0] I_imm;
wire [63:0] S_imm;
wire [63:0] B_imm;
wire [63:0] U_imm;
wire [63:0] J_imm;

wire RW_type;
wire R_type;
wire I_type;
wire S_type;
wire B_type;
wire U_type;
wire J_type;

wire Inst_ebreak;

wire Inst_auipc;
wire Inst_lui;
wire Inst_addi;
wire Inst_slti;
wire Inst_sltiu;
wire Inst_xori;
wire Inst_ori;
wire Inst_andi;
wire Inst_slli;
wire Inst_srli;
wire Inst_srai;

wire Inst_add;
wire Inst_sub;
wire Inst_sll;
wire Inst_slt;
wire Inst_sltu;
wire Inst_xor;
wire Inst_srl;
wire Inst_sra;
wire Inst_or;
wire Inst_and;

wire Inst_mul;
wire Inst_div;
wire Inst_divu;
wire Inst_rem;
wire Inst_remu;

wire Inst_addiw;
wire Inst_addw;
wire Inst_subw;
wire Inst_mulw;
wire Inst_divw;
wire Inst_divuw;
wire Inst_remw;
wire Inst_remuw;
wire Inst_sllw;
wire Inst_srlw;
wire Inst_sraw;
wire Inst_slliw;
wire Inst_srliw;
wire Inst_sraiw;

wire Inst_beq;
wire Inst_bne;
wire Inst_blt;
wire Inst_bge;
wire Inst_bltu;
wire Inst_bgeu;

wire Inst_jal;
wire Inst_jalr;

wire Inst_lb;
wire Inst_lbu;
wire Inst_sb;
wire Inst_lh;
wire Inst_lhu;
wire Inst_sh;
wire Inst_lw;
wire Inst_lwu;
wire Inst_sw;
wire Inst_ld;
wire Inst_sd;

wire Inst_ecall;
wire Inst_mret;
wire Inst_csrrs;
wire Inst_csrrw;
wire Inst_csrrc;

wire [63:0] jump_addr;
wire [63:0] j_addr;
wire [63:0] branch_addr;
wire Is_branch;

wire [1 :0] csr_raddr;
wire [63:0] csr_rdata;
wire [1 :0] csr_waddr1;
wire [63:0] csr_wdata1;
wire [1 :0] csr_waddr2;
wire [63:0] csr_wdata2;
wire csr_wen1;
wire csr_wen2;

//decode
assign current_pc = pc;

assign opcode  = Instruction[6 :0];
assign funct3  = Instruction[14:12];
assign funct7  = Instruction[31:25];
assign funct12 = Instruction[31:20];

assign R_type  = (opcode == 7'b0110011);
assign RW_type = (opcode == 7'b0111011);
assign I_type  = (opcode == 7'b0010011) || (opcode == 7'b0011011) || (opcode == 7'b0010011) || (opcode == 7'b0000011) || Inst_jalr;
assign S_type  = opcode == 7'b0100011;
assign B_type  = opcode == 7'b1100011;
assign U_type  = Inst_auipc | Inst_lui;
assign J_type  = Inst_jal;
assign RWI_type= RW_type | Inst_addiw;

assign I_imm = { {53{Instruction[31]}}, Instruction[30:25], Instruction[24:20]};
assign S_imm = { {53{Instruction[31]}}, Instruction[30:25], Instruction[11:7]};
assign B_imm = { {52{Instruction[31]}}, Instruction[7], Instruction[30:25], Instruction[11:8], 1'b0};
assign U_imm = { {33{Instruction[31]}}, Instruction[30:12], 12'b0}; 
assign J_imm = { {44{Instruction[31]}}, Instruction[19:12], Instruction[20], Instruction[30:21], 1'b0};
assign imm = I_type? I_imm : S_type? S_imm : B_type? B_imm : U_type? U_imm : J_type? J_imm : 64'b0;

assign rs1 = Instruction[19:15];
assign rs2 = Instruction[24:20];
assign rf_dest = Instruction[11:7];
assign rf_we = !(B_type | S_type);

assign Inst_auipc  = (opcode == 7'b0010111);
assign Inst_lui    = (opcode == 7'b0110111);

assign Inst_addi   = (opcode == 7'b0010011) && (funct3 == 3'b000);
assign Inst_slti   = (opcode == 7'b0010011) && (funct3 == 3'b010);
assign Inst_sltiu  = (opcode == 7'b0010011) && (funct3 == 3'b011);
assign Inst_xori   = (opcode == 7'b0010011) && (funct3 == 3'b100);
assign Inst_ori    = (opcode == 7'b0010011) && (funct3 == 3'b110);
assign Inst_andi   = (opcode == 7'b0010011) && (funct3 == 3'b111);
assign Inst_slli   = (opcode == 7'b0010011) && (funct3 == 3'b001) && (funct7[6:1] == 6'b000000);
assign Inst_srli   = (opcode == 7'b0010011) && (funct3 == 3'b101) && (funct7[6:1] == 6'b000000);
assign Inst_srai   = (opcode == 7'b0010011) && (funct3 == 3'b101) && (funct7[6:1] == 6'b010000);

assign Inst_add    = R_type && (funct3 == 3'b000) && (funct7 == 7'b0000000);
assign Inst_sub    = R_type && (funct3 == 3'b000) && (funct7 == 7'b0100000);
assign Inst_sll    = R_type && (funct3 == 3'b001) && (funct7 == 7'b0000000);
assign Inst_slt    = R_type && (funct3 == 3'b010) && (funct7 == 7'b0000000);
assign Inst_sltu   = R_type && (funct3 == 3'b011) && (funct7 == 7'b0000000);
assign Inst_xor    = R_type && (funct3 == 3'b100) && (funct7 == 7'b0000000);
assign Inst_srl    = R_type && (funct3 == 3'b101) && (funct7 == 7'b0000000);
assign Inst_sra    = R_type && (funct3 == 3'b101) && (funct7 == 7'b0100000);
assign Inst_or     = R_type && (funct3 == 3'b110) && (funct7 == 7'b0000000);
assign Inst_and    = R_type && (funct3 == 3'b111) && (funct7 == 7'b0000000);

assign Inst_mul    = R_type && (funct3 == 3'b000) && (funct7 == 7'b0000001);
assign Inst_div    = R_type && (funct3 == 3'b100) && (funct7 == 7'b0000001);
assign Inst_divu   = R_type && (funct3 == 3'b101) && (funct7 == 7'b0000001);
assign Inst_rem    = R_type && (funct3 == 3'b110) && (funct7 == 7'b0000001);
assign Inst_remu   = R_type && (funct3 == 3'b111) && (funct7 == 7'b0000001);

assign Inst_addiw  = (opcode == 7'b0011011) && (funct3 == 3'b000);
assign Inst_addw   = RW_type && (funct3 == 3'b000) && (funct7 == 7'b0000000);
assign Inst_subw   = RW_type && (funct3 == 3'b000) && (funct7 == 7'b0100000);
assign Inst_mulw   = RW_type && (funct3 == 3'b000) && (funct7 == 7'b0000001);
assign Inst_divw   = RW_type && (funct3 == 3'b100) && (funct7 == 7'b0000001);
assign Inst_divuw  = RW_type && (funct3 == 3'b101) && (funct7 == 7'b0000001);
assign Inst_remw   = RW_type && (funct3 == 3'b110) && (funct7 == 7'b0000001);
assign Inst_remuw  = RW_type && (funct3 == 3'b111) && (funct7 == 7'b0000001);
assign Inst_sllw   = RW_type && (funct3 == 3'b001) && (funct7 == 7'b0000000);
assign Inst_srlw   = RW_type && (funct3 == 3'b101) && (funct7 == 7'b0000000);
assign Inst_sraw   = RW_type && (funct3 == 3'b101) && (funct7 == 7'b0100000);
assign Inst_slliw  = (opcode == 7'b0011011) && (funct3 == 3'b001) && (funct7 == 7'b0000000);
assign Inst_srliw  = (opcode == 7'b0011011) && (funct3 == 3'b101) && (funct7 == 7'b0000000);
assign Inst_sraiw  = (opcode == 7'b0011011) && (funct3 == 3'b101) && (funct7 == 7'b0100000);

assign Inst_beq    = B_type && (funct3 == 3'b000);
assign Inst_bne    = B_type && (funct3 == 3'b001);
assign Inst_blt    = B_type && (funct3 == 3'b100);
assign Inst_bge    = B_type && (funct3 == 3'b101);
assign Inst_bltu   = B_type && (funct3 == 3'b110);
assign Inst_bgeu   = B_type && (funct3 == 3'b111);

assign Inst_jal    = (opcode == 7'b1101111);
assign Inst_jalr   = (opcode == 7'b1100111);

assign Inst_lb     = (opcode == 7'b0000011) && (funct3 == 3'b000);
assign Inst_lbu    = (opcode == 7'b0000011) && (funct3 == 3'b100);
assign Inst_sb     = (opcode == 7'b0100011) && (funct3 == 3'b000);
assign Inst_lh     = (opcode == 7'b0000011) && (funct3 == 3'b001);
assign Inst_lhu    = (opcode == 7'b0000011) && (funct3 == 3'b101);
assign Inst_sh     = (opcode == 7'b0100011) && (funct3 == 3'b001);
assign Inst_lw     = (opcode == 7'b0000011) && (funct3 == 3'b010);
assign Inst_lwu    = (opcode == 7'b0000011) && (funct3 == 3'b110);
assign Inst_sw     = (opcode == 7'b0100011) && (funct3 == 3'b010);
assign Inst_ld     = (opcode == 7'b0000011) && (funct3 == 3'b011);
assign Inst_sd     = (opcode == 7'b0100011) && (funct3 == 3'b011);

assign Inst_csrrs  = (opcode == 7'b1110011) && (funct3 == 3'b010);
assign Inst_csrrw  = (opcode == 7'b1110011) && (funct3 == 3'b001);
assign Inst_csrrc  = (opcode == 7'b1110011) && (funct3 == 3'b011);
assign Inst_ecall  = Instruction == 32'b00000000000000000000000001110011;
assign Inst_mret   = Instruction == 32'b00110000001000000000000001110011;
assign Inst_ebreak = Instruction == 32'b00000000000100000000000001110011;

assign alu_op[0] = Inst_add | Inst_addi | Inst_addiw | Inst_addw | Inst_auipc | Inst_jal | Inst_jalr;
assign alu_op[1] = Inst_sub | Inst_subw;
assign alu_op[2] = Inst_slt | Inst_slti;
assign alu_op[3] = Inst_sltu | Inst_sltiu;
assign alu_op[4] = Inst_and | Inst_andi;
assign alu_op[5] = 1'b0;
assign alu_op[6] = Inst_or | Inst_ori;
assign alu_op[7] = Inst_xor | Inst_xori;
assign alu_op[8] = Inst_lui;
assign alu_op[9] = Inst_mul | Inst_mulw;
assign alu_op[10] = Inst_div | Inst_divw;
assign alu_op[11] = Inst_divu | Inst_divuw;
assign alu_op[12] = Inst_rem | Inst_remw;
assign alu_op[13] = Inst_remu | Inst_remuw;

assign shifter_op[0] = Inst_sll | Inst_slli;
assign shifter_op[1] = Inst_srl | Inst_srli;
assign shifter_op[2] = Inst_sra | Inst_srai;
assign shifter_op[3] = Inst_sllw | Inst_slliw;
assign shifter_op[4] = Inst_srlw | Inst_srliw;
assign shifter_op[5] = Inst_sraw | Inst_sraiw;

assign oprand1 = (Inst_auipc | Inst_jal | Inst_jalr)? current_pc : rf_src1;
assign oprand2 = (Inst_jal | Inst_jalr)? 64'h0000000000000004 : (R_type | S_type | RW_type)? rf_src2 : imm;
assign Is_alu = alu_op != 14'b0;

assign j_addr = Inst_jal? (pc+imm) : Inst_jalr? rf_src1 + imm : 64'b0;
assign jump_addr = {j_addr[63:2],2'b0};
assign branch_addr = (pc+imm) & ~64'h0000000000000003;
assign Is_branch = ( Inst_beq && (rf_src1 == rf_src2) ) || ( Inst_bne && (rf_src1 != rf_src2) ) || ( Inst_blt && ($signed(rf_src1)<$signed(rf_src2)) )
                || ( Inst_bge && ($signed(rf_src1)>=$signed(rf_src2)) ) || ( Inst_bltu && (rf_src1<rf_src2) ) || ( Inst_bgeu && (rf_src1>=rf_src2) );
assign Is_trans = Inst_jal | Inst_jalr | Is_branch;
assign trans_addr = Is_branch? branch_addr : jump_addr;

assign Load = Inst_lb | Inst_lh | Inst_lw | Inst_ld;
assign Loadu = Inst_lbu | Inst_lhu | Inst_lwu;
assign Store = Inst_sb | Inst_sh | Inst_sw | Inst_sd;
assign DWHB[0] = Inst_lb | Inst_lbu | Inst_sb;
assign DWHB[1] = Inst_lh | Inst_lhu | Inst_sh;
assign DWHB[2] = Inst_lw | Inst_lwu | Inst_sw;
assign DWHB[3] = Inst_ld | Inst_sd;
assign LS_addr = rf_src1 + imm;

assign mask = (Inst_lb | Inst_lbu | Inst_sb)? (8'h01<<LS_addr[2:0]) : (Inst_lh | Inst_lhu | Inst_sh)? (8'h03<<LS_addr[2:0]) 
            : (Inst_lw | Inst_sw)? (8'h0f<<LS_addr[2:0]) : (Inst_ld | Inst_sd)? 8'hff : 8'h0;


assign Is_csr = Inst_csrrc | Inst_csrrs | Inst_csrrw;
assign csr_wen1 = Inst_csrrs | Inst_csrrw | Inst_csrrc | Inst_ecall;
assign csr_wen2 = Inst_ecall;
assign csr_raddr = (funct12 == 12'h341 || Inst_mret)? 2'b00 : (funct12 == 12'h300)? 2'b01 : (funct12 == 12'h342)? 2'b10 : (funct12 == 12'h305 || Inst_ecall)? 2'b11 : 2'b0;
assign csr_waddr1 = ((funct12 == 12'h341 && Is_csr) || Inst_ecall)? 2'b00 : (funct12 == 12'h300 && Is_csr)? 2'b01 : (funct12 == 12'h342 && Is_csr)? 2'b10 : (funct12 == 12'h305 && Is_csr)? 2'b11 : 2'b0;
assign csr_waddr2 = 2'b10; //专门为写异常号设置的端口
assign csr_wdata1 = Inst_ecall? current_pc : Inst_csrrs? (rf_src1 | csr_rdata) : Inst_csrrc? (~rf_src1 & csr_rdata) : rf_src1;
assign csr_wdata2 = Inst_ecall? 64'hb : 64'h0;
assign csr_result = csr_rdata;

assign Is_expc = Inst_ecall;
assign ex_addr = csr_rdata; //异常地址入口
assign Is_mretpc = Inst_mret;
assign mret_addr = csr_rdata + 4; //原先存入的pc+4

csr u_csr(clk, reset, csr_raddr, csr_rdata, csr_waddr1, csr_wdata1, csr_wen1, csr_waddr2, csr_wdata2, csr_wen2);
EBREAK u_EBREAK(Inst_ebreak, current_pc);

endmodule
