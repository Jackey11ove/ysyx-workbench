import "DPI-C" function void get_cpu_pc(input longint pc);

module IFU(
    input  wire clk,
    input  wire reset,
    input  wire Is_trans,
    input  wire [63:0] trans_addr,

    input  wire Is_expc,
    input  wire [63:0] ex_addr,
    input  wire Is_mretpc,
    input  wire [63:0] mret_addr,

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

always @(*) begin
    get_cpu_pc(pc);
end

assign nextpc = Is_expc? ex_addr : Is_mretpc? mret_addr : Is_trans? trans_addr : snpc;
assign snpc = pc + 4;

endmodule
