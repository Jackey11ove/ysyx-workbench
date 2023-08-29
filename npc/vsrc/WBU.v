import "DPI-C" function void get_cpu_ws_valid(input byte valid);

module WBU (
    input  wire        clk,
    input  wire        reset,
    input  reg  [63:0] ms_pc,
    input  reg  [31:0] ms_inst,

    input  wire        ms_to_ws_valid,
    input  reg         ms_rf_we,
    input  reg  [4 :0] ms_rf_dest,  
    input  wire [63:0] ms_final_result,
    output reg  [63:0] ws_pc,
    output reg  [31:0] ws_inst,
    output wire        ws_allowin,
    output reg         ws_valid,

    output reg         ws_rf_we,
    output reg  [4 :0] ws_rf_dest,
    output reg  [63:0] ws_final_result
);

wire        ws_ready_go;
wire [7:0]  ws_byte_valid;

assign ws_ready_go = 1'b1;
assign ws_allowin = !ws_valid || ws_ready_go;
assign ws_byte_valid = { 7'b0, ws_valid };

always @(posedge clk)begin
    if(reset)begin
        ws_valid <= 1'b0;
    end
    else if(ws_allowin)begin
        ws_valid <= ms_to_ws_valid;
    end
end

always @(posedge clk) begin
    if(ms_to_ws_valid && ws_allowin)begin
        ws_pc <= ms_pc;
        ws_inst <= ms_inst;
        ws_rf_we <= ms_rf_we;
        ws_rf_dest <= ms_rf_dest;
        ws_final_result <= ms_final_result;
    end
end

always @(*) begin
    get_cpu_ws_valid(ws_byte_valid);
end
    
endmodule