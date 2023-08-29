//有一个很要命的问题，这里面寄存器堆的0号位行为未定，也就是说不能使用0号位的寄存器堆，而实际上我们需要进行地址的对齐，这很麻烦
module cache (
    input  wire        clk,
    input  wire        reset,
    input  wire        cache_valid,       //为高表示访存请求有效，若无新的请求，要在下一周期置低
    input  wire        op_read,           //为高表示读，为低表示写
    input  wire [63:0] read_address,      //读数据的地址
    input  wire [63:0] write_address,     //写数据的地址
    input  wire [63:0] write_data,        //写数据
    input  wire [7 :0] write_shifter,         //写请求mask相关项
    input  wire [7 :0] write_DWHB,            //写请求mask相关项
    output reg         cache_ready,       //为高表示cache准备好接受新的请求
    output reg         cache_out_valid,   //为高表示cache输出结果有效
    output wire [63:0] cache_data
);

//cache主结构体的相关参数
parameter DATA_WIDTH = 64;   //64bit
parameter WORLD_WIDTH = 3;   //3bytes
parameter CACHE_SIZE = 4096;  //cacheline的行数
parameter BLOCK_SIZE = 4;    //块大小（字）
parameter LINE_SIZE  = DATA_WIDTH * BLOCK_SIZE; //一个cacheline的比特数

parameter VALID_SIZE  = 1;
parameter OFFSET_BITS = 2;
parameter INDEX_BITS  = 12;
parameter TAG_BITS    = DATA_WIDTH - INDEX_BITS - OFFSET_BITS - WORLD_WIDTH;  //64-7-2-3 = 52

//这里我们将整个cr的深度变为129,0号位不能用，所以在读写cr的时候cpu_index都要加一
reg  [LINE_SIZE - 1:0] cr [CACHE_SIZE + 1:0];  //cr是cache_reg的缩写，是cache的主体存储部分，寄存器堆的位宽为4个字256bit，深度为128
reg  [TAG_BITS - 1:0] tag [CACHE_SIZE + 1:0];  //tag的寄存器堆，位宽为TAG_BITS，深度为CACHE_SIZE
reg  [VALID_SIZE - 1:0] valid [CACHE_SIZE + 1:0];  //valid的寄存器堆，位宽为VALID_BITS，深度为CACHE_SIZE
wire [LINE_SIZE - 1:0] cr_line_data;   //此信号代表一个cache_line中的数据
wire [LINE_SIZE - 1:0] cr_line_wdata;  //此信号表示最终写cache时的行数据

wire [DATA_WIDTH - 1:0] mem_addr;
reg  [DATA_WIDTH - 1:0] read_addr_r;  //读地址的寄存器
reg  [DATA_WIDTH - 1:0] write_addr_r;
reg  [DATA_WIDTH - 1:0] write_data_r;
reg  [7             :0] write_shifter_r;
reg  [7             :0] write_DWHB_r;
wire [TAG_BITS - 1:0] cpu_tag;        //读请求的tag
wire [INDEX_BITS - 1:0] cpu_index;    //读请求的index
wire [INDEX_BITS    :0] cpu_index_extend;  //纯纯为了补0号位的漏，值为index+1
wire [OFFSET_BITS - 1:0] cpu_offset;  //读请求的offset
reg  op_read_r;
wire cache_hit;

wire [DATA_WIDTH - 1:0] hit_data;
wire [DATA_WIDTH - 1:0] data_addr1;   //这四个信号用来表示MISS时向cr填充的一个cacheline的四个字的地址
wire [DATA_WIDTH - 1:0] data_addr2;
wire [DATA_WIDTH - 1:0] data_addr3;
wire [DATA_WIDTH - 1:0] data_addr4;
reg  [DATA_WIDTH - 1:0] data_block1;  //这四个信号用来表示MISS时向cr填充的一个cacheline的四个字
reg  [DATA_WIDTH - 1:0] data_block2;
reg  [DATA_WIDTH - 1:0] data_block3;
reg  [DATA_WIDTH - 1:0] data_block4;

wire [7  :0] mask_shift;   //构建8位写掩码的移位数
wire [7  :0] byte_mask;    //8位写掩码
wire [63 :0] word_mask;    //64位写掩码
wire [LINE_SIZE - 1:0] word_mask_extend;
wire [LINE_SIZE - 1:0] line_mask;  //256位写掩码
wire [LINE_SIZE - 1:0] wdata_extend;  //将要写的单个字数据变为一个cache行的大小
wire [LINE_SIZE - 1:0] line_wdata;; //选出的wdata在cache行中的完整体现

//cache状态机的相关参数
reg [2 :0] current_state;
reg [2 :0] next_state;

parameter IDLE   = 3'b000;   //初始状态
parameter LOOKUP = 3'b001;   //查询状态，是否HIT
parameter MISS   = 3'b010;   //查询状态发现cache miss，将cacheline填充新的数据
parameter WRITE  = 3'b011;   //写数据时发现cache hit，需要更新cache中的内容，进入写状态完成对cache的更新
parameter HIT    = 3'b100;    //命中状态，可以从cache中读取数据

//状态机的时序逻辑
always @(posedge clk) begin
    if(reset)begin
        current_state <= IDLE;
    end
    else begin
        current_state <= next_state;
    end
end

always @(*) begin
    case(current_state)

      IDLE:begin
        if(cache_valid && cache_ready)begin
            next_state = LOOKUP;
        end
        else begin
            next_state = IDLE;
        end
      end

      LOOKUP:begin
        if( (op_read_r && cache_hit) || (!op_read_r && !cache_hit) )begin
            next_state = HIT;
        end
        else if(!op_read_r && cache_hit)begin
            next_state = WRITE;
        end
        else begin
            next_state = MISS;
        end
      end

      WRITE:begin
        next_state = HIT;
      end

      MISS:begin
        next_state = HIT;
      end

      HIT:begin
        next_state = IDLE;
      end
    
      default: begin
        // 处理未覆盖到的状态值
        next_state = IDLE;
      end

    endcase
end

/************寄存器信号赋值************/
always @(posedge clk) begin
    if(reset)begin
        cache_ready <= 1'b1;
    end
    else if(cache_valid && cache_ready)begin  //请求握手成功后需要将ready信号拉低
        cache_ready <= 1'b0;
    end
    else if(cache_out_valid)begin  //cache的输出有效周期过后将ready拉高
        cache_ready <= 1'b1;
    end
end

always @(posedge clk) begin
    if(reset)begin
        cache_out_valid <= 1'b0;
    end
    else if(cache_out_valid)begin
        cache_out_valid <= 1'b0;
    end
    else if(next_state == HIT)begin
        cache_out_valid <= 1'b1;
    end
end

always @(posedge clk) begin
    if(reset || cache_out_valid)begin
        op_read_r <= 1'b0;
        read_addr_r <= 64'b0;
        write_addr_r <= 64'b0;
        write_data_r <= 64'b0;
        write_shifter_r <= 8'b0;
        write_DWHB_r <= 8'b0;
    end
    else if(current_state == IDLE && next_state == LOOKUP)begin
        op_read_r <= op_read;
        read_addr_r <= read_address;
        write_addr_r <= write_address;
        write_data_r <= write_data;
        write_shifter_r <= write_shifter;
        write_DWHB_r <= write_DWHB;
    end
end

always @(posedge clk) begin
    if(current_state == MISS)begin
        valid[cpu_index_extend] <= 1'b1;
        tag[cpu_index_extend] <= cpu_tag;
        cr[cpu_index_extend] <= {data_block4, data_block3, data_block2, data_block1};
    end
    else if(current_state == WRITE)begin
        cr[cpu_index_extend] <= cr_line_wdata;
    end
end

assign mem_addr = op_read_r ? read_addr_r : write_addr_r;
assign cpu_tag = mem_addr[DATA_WIDTH - 1:INDEX_BITS + OFFSET_BITS + WORLD_WIDTH];
assign cpu_index = mem_addr[INDEX_BITS + OFFSET_BITS + WORLD_WIDTH - 1:OFFSET_BITS + WORLD_WIDTH];
assign cpu_index_extend = cpu_index + 2;
assign cpu_offset = mem_addr[OFFSET_BITS + WORLD_WIDTH - 1:WORLD_WIDTH];
assign cache_hit = (current_state == LOOKUP) && (valid[cpu_index_extend] == 1'b1) && (tag[cpu_index_extend] == cpu_tag);
//cache_hit既可以表示读命中也可以表示写命中

assign data_addr1 = {cpu_tag, cpu_index, 2'b00, 3'b0};
assign data_addr2 = {cpu_tag, cpu_index, 2'b01, 3'b0};
assign data_addr3 = {cpu_tag, cpu_index, 2'b10, 3'b0};
assign data_addr4 = {cpu_tag, cpu_index, 2'b11, 3'b0};
assign hit_data = (cpu_offset == 2'b11)? cr[cpu_index_extend][255:192] : (cpu_offset == 2'b10)? cr[cpu_index_extend][191:128] : (cpu_offset == 2'b01)? cr[cpu_index_extend][127:64] : cr[cpu_index_extend][63:0];
assign cache_data = hit_data;

//写数据相关
assign cr_line_data = cr[cpu_index_extend];
assign mask_shift = write_shifter_r >> 2'b11;
assign byte_mask = write_DWHB_r[0]? (8'h01<<mask_shift) : write_DWHB_r[1]? (8'h03<<mask_shift) 
            : write_DWHB_r[2]? (8'h0f<<mask_shift) : write_DWHB_r[3]? 8'hff : 8'h0;
assign word_mask = { {8{byte_mask[7]}} , {8{byte_mask[6]}} , {8{byte_mask[5]}} , {8{byte_mask[4]}} , {8{byte_mask[3]}} , {8{byte_mask[2]}} , {8{byte_mask[1]}} , {8{byte_mask[0]}} };
assign word_mask_extend = {192'b0,word_mask};
assign line_mask = (cpu_offset == 2'b11)? word_mask_extend << 8'hc0 : (cpu_offset == 2'b10)? word_mask_extend << 8'h80 : 
                   (cpu_offset == 2'b01)? word_mask_extend << 8'h40 : word_mask_extend;
assign wdata_extend = {192'b0,write_data_r << write_shifter_r};
assign line_wdata = (cpu_offset == 2'b11)? wdata_extend << 8'hc0 : (cpu_offset == 2'b10)? wdata_extend << 8'h80 : 
                    (cpu_offset == 2'b01)? wdata_extend << 8'h40 : wdata_extend;
assign cr_line_wdata = (cr_line_data & (~line_mask)) | (line_wdata & line_mask);

always @(*) begin
    if( (current_state == MISS) && !read_addr_r[30] )begin  //!read_addr_r[30]表示不是IO
      mem_read(data_addr1, data_block1);
      mem_read(data_addr2, data_block2);
      mem_read(data_addr3, data_block3);
      mem_read(data_addr4, data_block4);
    end
    else begin
        data_block1 = 64'b0;
        data_block2 = 64'b0;
        data_block3 = 64'b0;
        data_block4 = 64'b0;
    end
end

//写穿透，在写查询的时候直接更新内存
always @(*) begin
    if(!op_read_r && current_state == LOOKUP)begin
        mem_write(write_addr_r, write_data_r, write_shifter_r, write_DWHB_r);
    end
end

endmodule