module mul (
    input  wire        clk,
    input  wire        reset,
    input  reg         mul_valid,    //为高表示数据有效，如果没有新的乘法输入，在乘法被接受的下一个周期要置低
    //input  wire        flush,        //为高表示取消乘法
    input  wire [63:0] multiplicand, //被乘数
    input  wire [63:0] multiplier,   //乘数
    output reg         mul_ready,    //为高表示乘法器准备好，表示可以输入数据
    output reg         mul_out_valid,    //为高表示乘法器输出结果有效
    output wire [63:0] result_hi,    //乘法结果的高位
    output wire [63:0] result_lo     //乘法结果的低位
);
//乘法器模块采取每次将乘数的倍数和部分和相加并将部分和右移的策略

reg  [4  :0] counter;
wire [3  :0] booth_sel_out;
wire [64 :0] round_adder;          //此信号为每一轮中和部分和相加的数据
wire [128:0] round_adder_aligned;  //需要将加和的数据和部分和的高位对齐
wire [128:0] round_result;         //此信号为每轮中部分和加法运算的结果
wire [128:0] round_result_shifted; //此信号代表部分和进行加法运算后的移位

reg  [128:0] partial_sum; 
reg  [64 :0] multiplicand_r;  //被乘数扩展最后一个y-1位
wire [63 :0] multiplier_pos;
wire [63 :0] multiplier_neg;
wire [63 :0] multiplier_double_pos;
wire [63 :0] multiplier_double_neg;
//下面的寄存器会扩展一位的符号位
reg  [64 :0] mul_positive;
reg  [64 :0] mul_negative;
reg  [64 :0] mul_double_positive;
reg  [64 :0] mul_double_negative;

wire calculate_done;

reg current_state;
reg next_state;
parameter IDLE = 1'b0;
parameter CALCULATE = 1'b1;

//状态机跳转模块
always @(posedge clk) begin
    if(reset)begin
        current_state <= IDLE;
    end
    else begin
        current_state <= next_state;
    end
end

//握手信号寄存器赋值模块
always @(posedge clk) begin
    if(reset)begin
        mul_ready <= 1'b1;
    end
    else if(mul_valid && mul_ready)begin
        mul_ready <= 1'b0;
    end
    else if(mul_out_valid)begin
        mul_ready <= 1'b1;
    end
end

always @(posedge clk) begin
    if(reset)begin
        mul_out_valid <= 1'b0;
    end
    else if(mul_out_valid)begin
        mul_out_valid <= 1'b0;
    end
    else if(calculate_done)begin
        mul_out_valid <= 1'b1;
    end
end

//状态转换模块
always @(*) begin
    case (current_state)

        IDLE:begin
            if(mul_ready && mul_valid)begin
                next_state = CALCULATE;
            end
            else begin
                next_state = IDLE;
            end
        end
        
        CALCULATE:begin
            if(calculate_done)begin
                next_state = IDLE;
            end
            else begin
                next_state = CALCULATE;
            end
        end

        default: next_state = IDLE;
    endcase
end

booth_sel u_booth_sel(multiplicand_r[2:0],booth_sel_out);

assign multiplier_pos = multiplier;
assign multiplier_neg = (multiplier == 64'h8000000000000000)? multiplier : ~multiplier + 1; //计算出乘数的相反数的补码
assign multiplier_double_pos = multiplier_pos << 1;
assign multiplier_double_neg = multiplier_neg << 1;
assign round_adder = { {65{booth_sel_out[0]}} & mul_double_positive } |
                     { {65{booth_sel_out[1]}} & mul_double_negative } | 
                     { {65{booth_sel_out[2]}} & mul_positive } |
                     { {65{booth_sel_out[3]}} & mul_negative } ;
assign round_adder_aligned = {round_adder,64'b0};
assign round_result = partial_sum + round_adder_aligned;
assign round_result_shifted = $signed(round_result) >>> 2;
assign calculate_done = (&counter) && (current_state == CALCULATE); //将counter的数值全部与起来也就是全1,代表运算可以结束

//乘法相关参数赋值赋值模块
always @(posedge clk) begin
    if(reset || mul_out_valid)begin
        counter <= 5'b0;
        partial_sum <= 129'b0;
        multiplicand_r <= 65'b0;
        mul_positive <= 65'b0;
        mul_negative <= 65'b0;
        mul_double_positive <= 65'b0;
        mul_double_negative <= 65'b0;
    end
    else if(current_state == IDLE && next_state == CALCULATE)begin
        multiplicand_r <= {multiplicand,1'b0};
        mul_positive <= { {multiplier_pos[63]} , multiplier_pos };
        mul_negative <= { {multiplier_neg[63]} , multiplier_neg };
        mul_double_positive <= { {multiplier_double_pos[63]} , multiplier_double_pos };
        mul_double_negative <= { {multiplier_double_neg[63]} , multiplier_double_neg };
    end
    else if(current_state == CALCULATE)begin
        counter <= counter + 1;
        partial_sum <= round_result_shifted;
        multiplicand_r <= {2'b0,multiplicand_r[64:2]};
    end
end

assign result_hi = partial_sum[127:64];
assign result_lo = partial_sum[63:0];

endmodule


//booth_sel模块的作用是根据乘数后三位的值来选择到底对部分积做哪个操作
module booth_sel(
  input  wire [2:0] src,
  output wire [3:0] sel

);
///y+1,y,y-1///
wire y_add,y,y_sub;
wire sel_negative,sel_double_negative,sel_positive,sel_double_positive;

assign {y_add,y,y_sub} = src;

assign sel_negative =  y_add & (y & ~y_sub | ~y & y_sub);
assign sel_positive = ~y_add & (y & ~y_sub | ~y & y_sub);
assign sel_double_negative =  y_add & ~y & ~y_sub;
assign sel_double_positive = ~y_add &  y &  y_sub;

assign sel={sel_negative,sel_positive,sel_double_negative,sel_double_positive};
endmodule