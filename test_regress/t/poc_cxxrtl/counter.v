// Simple counter for CXXRTL server POC testing
module counter (
    input wire clk,
    input wire rst,
    output reg [7:0] count
);
    always @(posedge clk or posedge rst) begin
        if (rst)
            count <= 8'h0;
        else
            count <= count + 1'b1;
    end
endmodule
