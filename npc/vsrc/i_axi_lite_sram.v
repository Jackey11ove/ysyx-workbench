module i_axi_lite_sram(
    input  wire clk,
    input  wire rstn,

    input  wire [63:0] araddr,   //读请求地址
    input  wire        arvalid,  //读请求地址有效
    output wire        arready,  //sram可以接受读地址

    output wire [63:0] rdata,    //读数据    
    output wire        rvalid,   //读数据有效
    input  wire        rready,   //cpu可以接受读数据

    input  wire [63:0] waddr,    //写请求地址
    input  wire [63:0] wdata,    //写请求数据
    input  wire [7 :0] w_shifter,//写请求mask相关项
    input  wire [7 :0] w_DWHB,   //写请求mask相关项
    input  wire        wvalid,   //写地址和写数据有效
    output wire        wready,   //sram可以接受写地址和写数据

    output wire        bvalid,   //写请求响应有效
    input  wire        bready   //cpu接受写响应
);

wire [63:0] mem_addr;
wire        Is_IO;   //此信号拉高表示读写为IO

reg [63:0] araddr_r;
reg        arready_r;
reg [63:0] rdata_r;
reg        rvalid_r;

reg [63:0] waddr_r;
reg [63:0] wdata_r;
reg [7 :0] w_shifter_r;
reg [7 :0] w_DWHB_r;
reg        wready_r;
reg        bvalid_r;

/******cache related******/
wire        cache_valid;       //为高表示访存请求有效，若无新的请求，要在下一周期置低
wire        op_read;           //为高表示读，为低表示写
wire [63:0] read_address;      //读数据的地址
wire [63:0] write_address;     //写数据的地址
wire [63:0] write_data;        //写数据
reg         cache_ready;       //为高表示cache准备好接受新的请求
reg         cache_out_valid;   //为高表示cache输出结果有效
wire [63:0] cache_data;

reg [1 :0] current_state;
reg [1 :0] next_state;

parameter IDLE = 2'b0;
parameter MEM_READ = 2'b01;
parameter MEM_WRITE = 2'b10;

always @(posedge clk) begin
    if(!rstn)begin
        current_state <= IDLE;
    end
    else begin
        current_state <= next_state;
    end
end

always @(*) begin
    case (current_state)

        IDLE:begin
            if(arvalid && arready)begin
                next_state = MEM_READ;
            end
            else if(wvalid && wready)begin
                next_state = MEM_WRITE;
            end
            else begin
                next_state = IDLE;
            end
        end

        MEM_READ:begin
            if(rvalid && rready)begin
                next_state = IDLE;
            end
            else begin
                next_state = MEM_READ;
            end
        end

        MEM_WRITE:begin
            if(bvalid && bready)begin
                next_state = IDLE;
            end
            else begin
                next_state = MEM_WRITE;
            end
        end

        default: next_state = IDLE;
    endcase
end

always @(posedge clk) begin
    if(!rstn)begin
        arready_r <= 1'b0;
        rvalid_r <=1'b0;
        wready_r <= 1'b0;
        bvalid_r <= 1'b0;
    end
    else begin
        if(next_state == IDLE)begin
            arready_r <= 1'b1;
            wready_r <= 1'b1;
            rvalid_r <= 1'b0;
            bvalid_r <= 1'b0;
        end
        else if(next_state == MEM_READ)begin
            rvalid_r <= 1'b1;
            araddr_r <= araddr;
        end
        else if(next_state == MEM_WRITE)begin
            waddr_r <= waddr;
            wdata_r <= wdata;
            w_shifter_r <= w_shifter;
            w_DWHB_r <= w_DWHB;
            bvalid_r <= 1'b1;
        end
    end
end

always @(*) begin
    if(current_state == MEM_READ && Is_IO)begin
        mem_read(araddr_r, rdata_r);
    end
    else begin
        rdata_r = 64'b0;
    end
end

always @(*) begin
    if(current_state == MEM_WRITE && Is_IO)begin
        mem_write(waddr_r, wdata_r, w_shifter_r, w_DWHB_r);
    end
end

cache u_cache(clk, !rstn, cache_valid, op_read, read_address, write_address, write_data, w_shifter, w_DWHB, cache_ready, cache_out_valid, cache_data);

assign mem_addr = op_read? araddr : waddr;
assign Is_IO = mem_addr[29];

//cache赋值
assign cache_valid = (arvalid || wvalid) && !Is_IO;
assign read_address = araddr;
assign write_address = waddr;
assign write_data = wdata;
assign op_read = arvalid;

assign arready = Is_IO? arready_r : cache_ready;
assign rvalid = Is_IO? rvalid_r : cache_out_valid;
assign rdata = Is_IO? rdata_r : cache_data;
assign wready = Is_IO? wready_r : cache_ready;
assign bvalid = Is_IO? bvalid_r : cache_out_valid;

endmodule