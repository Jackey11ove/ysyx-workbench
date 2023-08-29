import "DPI-C" function void set_csr_ptr(input logic [63:0] a []);

module csr(
  input clk,
  input reset,
  //READ PORT
  input  wire [2 :0] csr_raddr, //现阶段共计四个csr寄存器,采用2位的编码
  output wire [63:0] csr_rdata,
  //WRITE PORT1
  input  wire [2 :0] csr_waddr1,
  input  wire [63:0] csr_wdata1,
  input  wire csr_wen1,
  //WRITE PORT2
  input  wire [2 :0] csr_waddr2,
  input  wire [63:0] csr_wdata2,
  input  wire csr_wen2  
);

reg [63:0] csr_rf [4:0]; //csr[1]=mepc存放触发异常的PC,csr[2]=mstatus存放处理器的状态,csr[3]=mcause存放异常触发的原因,csr[4]=mtvec存放异常处理地址

always @(posedge clk)begin
  if(reset)begin
    csr_rf[2] <= 64'ha00001800;
  end
end

always @(posedge clk)begin
  if(csr_wen1) csr_rf[csr_waddr1] <= csr_wdata1;

end

always @(posedge clk)begin
  if(csr_wen2)begin
    csr_rf[csr_waddr2] <= csr_wdata2;
  end
end

assign csr_rdata = csr_rf[csr_raddr];

initial set_csr_ptr(csr_rf);

endmodule