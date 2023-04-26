module IFU(
    input  wire clk,
    input  wire reset,
    input  wire Is_trans,
    input  wire [63:0] trans_addr,

    output reg  [63:0] pc
);

wire [63:0] nextpc;
wire [63:0] snpc;

always @(posedge clk) begin
    if(reset)begin
        pc <= 64'h80000000;
    end
    else begin
        pc <= nextpc;
    end
end

assign nextpc = Is_trans? trans_addr : snpc;
assign snpc = pc + 4;

endmodule
