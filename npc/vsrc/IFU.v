module IFU(
    input  wire        clk,
    input  wire        reset,
    input  wire        ds_load_block,
    input  wire        Is_trans, //记得在ID阶段为跳转指令加上ds_valid
    input  wire [63:0] trans_addr,
    input  wire        branch_taken_cancel,

    input  wire        Is_expc,
    input  wire [63:0] ex_addr,
    input  wire        Is_mretpc,
    input  wire [63:0] mret_addr,

    input  wire        ds_allowin,
    output wire [31:0] Instruction,
    output wire [63:0] nextpc,
    output reg  [63:0] fs_pc,
    output wire        fs_to_ds_valid
);

wire [63:0] snpc;
wire [63:0] Inst_extend;
reg  [63:0] nextpc_r;    //由于ds阶段译码出的分支指令只会将Is_trans拉高一拍，而此时fs阶段的取指还没有完成，生成的跳转地址作为nextpc无法生效就会被覆盖，此信号可以将需要生效的nextpc保存下来
reg         keep_nextpc;

reg  fs_valid;
wire fs_ready_go;
wire fs_allowin;

//AXI_LITE
wire [63:0] araddr;   //读请求地址
wire        arvalid;  //读请求地址有效
wire        arready;  //sram可以接受读地址

wire [63:0] rdata;    //读数据    
wire        rvalid;   //读数据有效
wire        rready;   //cpu可以接受读数据

wire [63:0] waddr;    //写请求地址
wire [63:0] wdata;    //写请求数据
wire [7 :0] w_shifter;//写请求mask相关项
wire [7 :0] w_DWHB;   //写请求mask相关项
wire        wvalid;   //写地址和写数据有效
wire        wready;   //sram可以接受写地址和写数据

wire        bvalid;   //写请求响应有效
wire        bready;   //cpu接受写响应

assign fs_ready_go = rvalid;
assign fs_allowin = !fs_valid || (fs_ready_go && ds_allowin);
assign fs_to_ds_valid = fs_valid && fs_ready_go;

always @(posedge clk) begin
    if(reset)begin
        fs_valid <= 1'b0;
    end
    else if(fs_allowin)begin
        fs_valid <= 1'b1;
    end
    else if(branch_taken_cancel)begin
        fs_valid <= 1'b0;
    end
end

always @(posedge clk) begin
    if(reset)begin
        fs_pc <= 64'h80000000;
    end
    else if(~reset && fs_allowin)begin
        fs_pc <= nextpc;
    end
end

always @(posedge clk) begin
    if(reset)begin
        nextpc_r <= 64'b0;
        keep_nextpc <= 1'b0;
    end
    else if(Is_trans && !ds_load_block && !fs_ready_go)begin
        nextpc_r <= nextpc;
        keep_nextpc <= 1'b1;
    end
    else if(fs_ready_go)begin
        nextpc_r <= 64'b0;
        keep_nextpc <= 1'b0;
    end
end

assign araddr = fs_pc;
assign arvalid = fs_valid;
assign Inst_extend = rdata;
assign rready = 1'b1;
assign waddr = 64'b0;
assign wdata = 64'b0;
assign w_shifter = 8'b0;
assign w_DWHB = 8'b0;
assign wvalid = 1'b0;
assign bready = 1'b0;

i_axi_lite_sram u_i_axi_lite_sram(clk, !reset, araddr, arvalid, arready, rdata, rvalid, rready, waddr, wdata, w_shifter, w_DWHB, wvalid, wready, bvalid, bready);

assign Instruction = fs_pc[2]? Inst_extend[63:32] : Inst_extend[31:0];
assign nextpc = Is_expc? ex_addr : Is_mretpc? mret_addr : keep_nextpc? nextpc_r : Is_trans? trans_addr : snpc;
assign snpc = fs_pc + 4;

endmodule
