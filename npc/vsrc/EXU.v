import "DPI-C" function void mem_write(input longint waddr, input longint wdata, input byte shift, input byte DWHB);
import "DPI-C" function void mem_read(input longint raddr, output longint rdata);

module EXU (
    input reg  [63:0] pc,
    input wire [63:0] oprand1,
    input wire [63:0] oprand2,

    input wire [13:0] alu_op,
    input wire [8 :0] shifter_op,
    input wire Is_alu,
    input wire RWI_type,

    input wire Load,
    input wire Loadu,
    input wire Store,
    input wire [3 :0] DWHB,
    input wire [7 :0] mask,
    input wire [63:0] LS_addr,

    output wire [63:0] exu_result
);

wire [63:0] alu_src1;
wire [63:0] alu_src2;
wire [63:0] alu_result;

wire [63:0] shifter_src1;
wire [63:0] shifter_src2;
wire [63:0] shifter_result;

wire [63:0] RW_result;

reg  [63:0] Load_data;
wire [7 :0] LS_offset;
wire [7 :0] LS_shifter;
wire [63:0] Load_shifted_data;
wire [7 :0] lb_data;
wire [15:0] lh_data;
wire [31:0] lw_data;
wire [63:0] ld_data;
wire [63:0] Load_result;

assign alu_src1 = oprand1;
assign alu_src2 = oprand2;

assign shifter_src1 = oprand1;
assign shifter_src2 = oprand2;

alu u_alu(alu_op, alu_src1, alu_src2, alu_result);
shifter u_shifter(shifter_op, shifter_src1, shifter_src2, shifter_result);

assign RW_result = Is_alu? { {32{alu_result[31]}},alu_result[31:0] } : { {32{shifter_result[31]}},shifter_result[31:0] };

always @(*) begin
  if(Load | Loadu)begin
    mem_read(LS_addr, Load_data);
  end
  else begin
    Load_data = 64'b0;
  end
end

always @(*) begin
  if(Store)begin
    mem_write(LS_addr, oprand2, LS_shifter, {4'b0,DWHB});
  end
end

assign LS_offset = {5'b0,LS_addr[2:0]};
assign LS_shifter = LS_offset << 2'b11; //左移3位,相当于乘以8,根据偏移的地址确定要移位的位数
assign Load_shifted_data = Load_data >> LS_shifter;
assign lb_data = Load_shifted_data[7 :0];
assign lh_data = Load_shifted_data[15:0];
assign lw_data = Load_shifted_data[31:0];
assign ld_data = Load_shifted_data[63:0];
assign Load_result = (Load && DWHB[0])? { {56{lb_data[7]}},lb_data } : (Loadu && DWHB[0])? { 56'b0,lb_data } :
                     (Load && DWHB[1])? { {48{lh_data[15]}},lh_data } : (Loadu && DWHB[1])? { 48'b0,lh_data } :
                     (Load && DWHB[2])? { {32{lw_data[31]}},lw_data } :
                     (Load && DWHB[3])? ld_data : 64'b0;

assign exu_result = (Load | Loadu)? Load_result : RWI_type? RW_result : Is_alu? alu_result : shifter_result;

endmodule
