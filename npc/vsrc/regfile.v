import "DPI-C" function void set_gpr_ptr(input logic [63:0] a []);

module regfile #(ADDR_WIDTH = 1, DATA_WIDTH = 1) ( //32个通用寄存器，每个是64位,ADDR_WIDTH应该是5,DATA_WIDTH应该是64
  input clk,
  //READ PORT 1
  input wire [4 :0] raddr1,
  output wire [DATA_WIDTH-1:0] rdata1,
  //READ PORT 2
  input wire [4 :0] raddr2,
  output wire [DATA_WIDTH-1:0] rdata2,
  //WRITE PORT
  input wire [4 :0] waddr,
  input wire [DATA_WIDTH-1:0] wdata,
  input wire wen
);

  reg [DATA_WIDTH-1:0] rf [ADDR_WIDTH-1:0];

  always @(posedge clk) begin
    if (wen) rf[waddr] <= wdata;
    rf[0] <= 64'b0;
  end

  assign rdata1 = (raddr1 == 5'b0)? 64'b0 : rf[raddr1];
  assign rdata2 = (raddr2 == 5'b0)? 64'b0 : rf[raddr2];

  initial set_gpr_ptr(rf); // rf为通用寄存器的二维数组变量

endmodule
