`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date:    22:49:08 03/29/2010 
// Design Name: 
// Module Name:    clkdiv 
// Project Name: 
// Target Devices: 
// Tool versions: 
// Description: 
//
// Dependencies: 
//
// Revision: 
// Revision 0.01 - File Created
// Additional Comments: 
//
//////////////////////////////////////////////////////////////////////////////////
module clkdiv(
    input clk,
    output clk6,
    output clk8,
    output clkps,
    output clkled
    );
	 
	 reg [31:0] count6;
	 reg [31:0] count8;
	 reg [31:0] countled;
	 reg [31:0] countps;
	 reg clkpso;
	 reg clk6o;
	 reg clk8o;
	 reg clkledo;
	 
	 always @(posedge clk)
	 begin
		count6<=count6+1;
		count8<=count8+1;
		countps<=countps+1;
		countled<=countled+1;
		
		if(count6==4166666)
		begin
			count6<=0;
			clk6o<=!clk6o;
		end		
		if(count8==3125000)
		begin
			count8<=0;
			clk8o<=!clk8o;
		end		
		if(countps==2500000)
		begin
			countps<=0;
			clkpso<=!clkpso;
		end		
		if(countled==25000)
		begin
			countled<=0;
			clkledo<=!clkledo;
		end		
		
	 end
	 
	 assign clkps=clkpso;
	 assign clk6=clk6o;
	 assign clk8=clk8o;
	 assign clkled=clkledo;

endmodule