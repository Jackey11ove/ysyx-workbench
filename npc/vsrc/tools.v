import "DPI-C" function void ebreak();
module EBREAK(
    input wire Inst_ebreak,
    input wire [63:0] current_pc
);
always @(*) begin
    if(Inst_ebreak)begin
       $display("At pc = 0x%h",current_pc); 
       ebreak();      
    end  
end
endmodule