module div (
    input  wire        clk,
    input  wire        reset,
    input  reg         div_valid,     //为高表示数据有效，如果没有新的除法输入，在除法被接受的下一个周期要置低
    input  wire [63:0] dividend,      //被除数
    input  wire [63:0] divisor,       //除数
    input  wire        div_signed,    //符号
    output reg         div_ready,     //为高表示除法器准备好，表示可以输入数据
    output reg         div_out_valid, //为高表示除法器输出结果有效
    output wire [63:0] quotient,      //商
    output wire [63:0] remainder      //余数
);

reg [5  :0] counter;  //计数器，记录除法周期数

reg [127:0] dividend_r;
reg [63 :0] divisor_r,quotient_r,remain_r;
reg         divisor_s,dividend_s;

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
        div_ready <= 1'b1;
    end
    else if(div_valid && div_ready)begin
        div_ready <= 1'b0;
    end
    else if(div_out_valid)begin
        div_ready <= 1'b1;
    end
end

always @(posedge clk) begin
    if(reset)begin
        div_out_valid <= 1'b0;
    end
    else if(div_out_valid)begin
        div_out_valid <= 1'b0;
    end
    else if(calculate_done)begin
        div_out_valid <= 1'b1;
    end
end

//状态转换模块
always @(*) begin
    case (current_state)

        IDLE:begin
            if(div_ready && div_valid)begin
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

always @(posedge clk) begin
    if (reset) begin
        divisor_s <= 1'b0;
        dividend_s <= 1'b0;        
    end
    else if (current_state == IDLE && next_state == CALCULATE) begin
        divisor_s <= div_signed & divisor[63];
        dividend_s <= div_signed & dividend[63];
    end    
end

wire sub_cout;
wire [64:0] sub_result;
wire op_correct;
wire [63:0] quotient_correct,remain_correct;
wire quotient_need_correct,remain_need_correct;
wire [63:0] dividend_neg,divisor_neg,dividend_abs,divisor_abs;  //传入的除数和被除数的负值和绝对值
wire [63:0] dividend_adder_src1,divisor_adder_src1; //基本上就是传入的除数和被除数

assign dividend_adder_src1 = op_correct ? dividend_r[127:64] : dividend;
assign divisor_adder_src1 = op_correct ? quotient_r : divisor;

//两个加法器用来做取反加一的操作，算出来的是相反数
adder_64 dividend_adder(
    .src1  (~dividend_adder_src1),
    .src2  ({64'b1}),
    .cin    (1'b0),
    .cout   (),
    .result (dividend_neg)
);

adder_64 divisor_adder(
    .src1  (~divisor_adder_src1),
    .src2  ({64'b1}),
    .cin    (1'b0),
    .cout   (),
    .result (divisor_neg)
);

adder_65 suber(
    .src1  (dividend_r[127:63]),
    .src2  ({1'b1,~divisor_r}),
    .cin    (1'b1),
    .cout   (sub_cout),
    .result (sub_result)
);

//计算除数和被除数的绝对值
assign dividend_abs = (div_signed & dividend[63]) ? dividend_neg : dividend;
assign divisor_abs = (div_signed & divisor[63]) ? divisor_neg : divisor;

always @(posedge clk) begin
    if(current_state == IDLE && next_state == CALCULATE)begin
        dividend_r <= {64'b0 , dividend_abs};
        divisor_r <= divisor_abs;
    end
    else if(current_state == CALCULATE)begin
        dividend_r <= sub_cout? {sub_result[63:0],dividend_r[62:0],1'b0} : {dividend_r[126:0],1'b0};
    end
end

//计数器
always @(posedge clk) begin
    if(reset || div_out_valid)begin
        counter <= 6'b0;
    end
    else if(current_state == CALCULATE)begin
        counter <= counter + 1'b1;
    end
end

assign calculate_done = (&counter) && (current_state == CALCULATE);

assign op_correct = calculate_done;  //op_correct信号指示该周期应该修正商和余数的符号，而此周期正应该是计算结束的周期
assign remain_correct  = dividend_neg;
assign quotient_correct = divisor_neg;
assign quotient_need_correct = ~dividend_s & divisor_s | dividend_s & ~divisor_s; //被除数和除数符号不同的时候要修改商符
assign remain_need_correct  = dividend_s;  //余数的符号和被除数的符号相同

always @(posedge clk) begin
    if(op_correct)begin
        remain_r <= remain_need_correct? remain_correct : dividend_r[127:64];
        quotient_r <= quotient_need_correct? quotient_correct : quotient_r;
    end
    if(current_state == CALCULATE)begin
        quotient_r <= {quotient_r[62:0],sub_cout};
    end
end

assign quotient = quotient_need_correct? ~quotient_r + 1 : quotient_r;
assign remainder = remain_need_correct? remain_correct : dividend_r[127:64];

endmodule


module adder_64 (
    // 64-bit adder
    input [63:0] src1,
    input [63:0] src2,
    input        cin,
    output       cout,
    output [63:0] result
);
assign {cout, result} = src1 + src2 + {63'b0,cin};

endmodule

module adder_65 (
    // 65-bit adder
    input [64:0] src1,
    input [64:0] src2,
    input        cin,
    output       cout,
    output [64:0] result
);
assign {cout, result} = src1 + src2 + {64'b0,cin};

endmodule