module MEMU (
    input  wire        clk,
    input  wire        reset,
    input  reg  [63:0] es_pc,
    input  reg  [31:0] es_inst,

    input  wire        ws_allowin,
    input  wire        es_to_ms_valid,
    output wire        ms_LS_result_valid,
    output wire        ms_res_from_mem,
    output wire        ms_to_ws_valid,
    output reg  [63:0] ms_pc,
    output reg  [31:0] ms_inst,
    output wire        ms_allowin,
    output reg         ms_valid,
    output reg         ms_rf_we,
    output reg  [4 :0] ms_rf_dest,

    input  reg         es_rf_we,
    input  reg  [4 :0] es_rf_dest,
    input  reg         es_is_Load,
    input  reg         es_is_Loadu,
    input  reg         es_is_Store,
    input  reg  [3 :0] es_DWHB,
    input  reg  [63:0] es_LS_addr,
    input  wire [63:0] es_mem_wdata,
    input  wire [63:0] es_result,

    output wire [63:0] ms_final_result
);

wire        ms_ready_go;

reg         ms_is_Load;
reg         ms_is_Loadu;
reg         ms_is_Store;
reg  [3 :0] ms_DWHB;
reg  [63:0] ms_LS_addr;
reg  [63:0] ms_mem_wdata;
reg  [63:0] ms_result;  //这里的ms_result是es传过来的ALU和shifter的结果，最终结果在final_result中

wire [63:0] Load_data;
wire [7 :0] LS_offset;
wire [7 :0] LS_shifter;
wire [63:0] Load_shifted_data;
wire [7 :0] lb_data;
wire [15:0] lh_data;
wire [31:0] lw_data;
wire [63:0] ld_data;
wire [63:0] Load_result;

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
reg         wvalid_r; //此信号存在的意义在于，如果mem阶段wvalid一直拉高，则可能发送多次写请求，需要在一次请求之后拉低该信号
wire        wready;   //sram可以接受写地址和写数据

wire        bvalid;   //写请求响应有效
wire        bready;   //cpu接受写响应


assign ms_ready_go = !( ( (ms_is_Load | ms_is_Loadu) && !rvalid ) || (ms_is_Store && !bvalid) ) ;
assign ms_allowin = !ms_valid || (ms_ready_go && ws_allowin);
assign ms_to_ws_valid = ms_valid && ms_ready_go;

always @(posedge clk ) begin
    if(reset) begin
        ms_valid <= 1'b0;
    end

    else if (ms_allowin)begin
        ms_valid <= es_to_ms_valid;
    end
end

always @(posedge clk) begin
    if(es_to_ms_valid && ms_allowin)begin
        ms_pc <= es_pc;
        ms_inst <= es_inst;
        ms_rf_we <= es_rf_we;
        ms_rf_dest <= es_rf_dest;
        ms_result <= es_result;
        ms_is_Load <= es_is_Load;
        ms_is_Loadu <= es_is_Loadu;
        ms_is_Store <= es_is_Store;
        ms_DWHB <= es_DWHB;
        ms_LS_addr <= es_LS_addr;
        ms_mem_wdata <= es_mem_wdata;
    end
end

always @(posedge clk) begin
    if(wvalid_r)begin
        wvalid_r <= 1'b0;
    end
    else if(es_to_ms_valid && ms_allowin)begin
        wvalid_r <= es_is_Store;
    end
end

//DPI-C实现访存
i_axi_lite_sram u_i_axi_lite_sram(clk, !reset, araddr, arvalid, arready, rdata, rvalid, rready, waddr, wdata, w_shifter, w_DWHB, wvalid, wready, bvalid, bready);

assign LS_offset = {5'b0,ms_LS_addr[2:0]};
assign LS_shifter = LS_offset << 2'b11; //左移3位,相当于乘以8,根据偏移的地址确定要移位的位数
assign Load_shifted_data = Load_data >> LS_shifter;
assign lb_data = Load_shifted_data[7 :0];
assign lh_data = Load_shifted_data[15:0];
assign lw_data = Load_shifted_data[31:0];
assign ld_data = Load_shifted_data[63:0];
assign Load_result = (ms_is_Load && ms_DWHB[0])? { {56{lb_data[ 7]}},lb_data } : (ms_is_Loadu && ms_DWHB[0])? { 56'b0,lb_data } :
                     (ms_is_Load && ms_DWHB[1])? { {48{lh_data[15]}},lh_data } : (ms_is_Loadu && ms_DWHB[1])? { 48'b0,lh_data } :
                     (ms_is_Load && ms_DWHB[2])? { {32{lw_data[31]}},lw_data } : (ms_is_Loadu && ms_DWHB[2])? { 32'b0,lw_data } :
                     (ms_is_Load && ms_DWHB[3])? ld_data : 64'b0;
  
assign araddr = ms_LS_addr;
assign arvalid = (ms_is_Load | ms_is_Loadu) & ms_valid;
assign Load_data = rdata;
assign rready = 1'b1;
assign waddr = ms_LS_addr;
assign wdata = ms_mem_wdata;
assign w_shifter = LS_shifter;
assign w_DWHB = {4'b0,ms_DWHB};
assign wvalid = wvalid_r;
assign bready = 1'b1;

assign ms_LS_result_valid = ms_ready_go;
assign ms_res_from_mem = ms_is_Load || ms_is_Loadu;

assign ms_final_result = (ms_is_Load || ms_is_Loadu)? Load_result : ms_result;
    
endmodule