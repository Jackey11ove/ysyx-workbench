module axi_lite_sram(
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
    if(current_state == MEM_READ)begin
        mem_read(araddr_r, rdata_r);
    end
    else begin
        rdata_r = 64'b0;
    end
end

always @(*) begin
    if(current_state == MEM_WRITE)begin
        mem_write(waddr_r, wdata_r, w_shifter_r, w_DWHB_r);
    end
end


assign arready = arready_r;
assign rdata = rdata_r;
assign rvalid = rvalid_r;
assign wready = wready_r;
assign bvalid = bvalid_r;

endmodule