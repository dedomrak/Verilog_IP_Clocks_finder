/*

** prep benchmark 3 -- small state machine

** benchmark suite #1 -- version 1.2 -- March
28, 1993

** Programmable Electronics Performance Corporation

**

** binary state assignment -- highly encoded

*/


`include "D_flipflop.v"
`include "clock_gen.v"

`timescale 1ns/1ps

module top (

//input clk, rst ,
input wire clk,
input wire rst ,
input [7:0] in ,
output reg [7:0] out
) ;


parameter [2:0] // synopsys enum code

    START = 3'd0 ,
    SA    = 3'd1 ,
    SB    = 3'd2 ,
    SC    = 3'd3 ,
    SD    = 3'd4 ,
    SE    = 3'd5 ,
    SF    = 3'd6 ,
    SG    = 3'd7 ;
	
	wire div_clk_1Hz;
	wire gen_clk_mod_out;
	wire  dff_inst1_out;


// synopsys state_vector state

reg [2:0] // synopsys enum code

    state, next_state ;

reg [7:0] out1, next_out ;



always @ (in or state) begin

    // default values
    next_state = START ;
    next_out = 8'bx ;
    // state machine

    case (state) // synopsys parallel_case full_case

    START:
	if (in == 8'h3c) begin
	    next_state = SA ;
	    next_out = 8'h82 ;
	    end

	else begin
	    next_state = START ;
	    next_out = 8'h00 ;
	    end

    SA:

	case (in) // synopsys parallel_case full_case
	    8'h2a:
		begin
		next_state = SC ;
		next_out = 8'h40 ;
		end

	    8'h1f:

		begin
		next_state = SB ;
		next_out = 8'h20 ;
		end

	    default:

		begin
		next_state = SA ;
		next_out = 8'h04 ;
		end
	    endcase



    SB:

	if (in == 8'haa) begin
	    next_state = SE ;
	    next_out = 8'h11 ;
	    end

	else begin
	    next_state = SF ;
	    next_out = 8'h30 ;
	    end



    SC:

	begin
	next_state = SD ;
	next_out = 8'h08 ;
	end



    SD:

	begin
	next_state = SG ;
	next_out = 8'h80 ;
	end



    SE:

	begin
	next_state = START ;
	next_out = 8'h40 ;
	end



    SF:

	begin
	next_state = SG ;
	next_out = 8'h02 ;
	end



    SG:

	begin
	next_state = START ;
	next_out = 8'h01 ;
	end
    endcase

    end


clk_divider  divClk_inst(.clk_50mhz (clk), .rst(rst) , .clk_1hz(div_clk_1Hz));
d_ff   dff_top_inst1( .q(dff_inst1_out), .d(in[1]),  .rst_n(rst) ,  .clk(div_clk_1Hz));
clock_gen clk_gen_bl1( .enable(1'b1), .clk(gen_clk_mod_out));
block1   block_inst1( .clk(clk),  .rst(rst), .data({in[7],in[8]}), .blk1_out_reg({out1[7],out1[8]}));


assign out1[1]  = dff_inst1_out;
// build the state flip-flops

always @ (posedge clk or negedge rst)
    begin
    if (!rst)   state <= #1 START ;
    else        state <= #1 next_state ;
    end



// build the output flip-flops

always @ (posedge clk or negedge rst)
    begin
    if (!rst)   out <= #1 8'b0 ;
    else        out <= #1 next_out ;
    end

always @ (posedge div_clk_1Hz or negedge rst)
    begin
    if (!rst)   out <= #1 8'b0 ;
    else        out <= #1 next_out ;
    end
	
	always @ (posedge gen_clk_mod_out or negedge rst)
    begin
    if (!rst)   out[0] <= #1 1'b1 ;
    else        out[0] <= #1 next_out[0] ;
    end

	endmodule


module block1 
  (
    input clk,
    input rst,
	input [1:0]data,
    output reg [2:0] blk1_out_reg
);
    
    wire gen_clk_out ;
	wire dff_out1,dff_out2;
	wire blk2_genClk_out;

	clock_gen clk_gen_block_inst1( .enable(1'b1), .clk(gen_clk_out));
    d_ff   dff_blk1_inst1( .q(dff_out1), .d(data[0]),  .rst_n(rst) ,  .clk(clk));
	d_ff   dff_blk1_inst2( .q(dff_out2), .d(data[1]),  .rst_n(rst) ,  .clk(gen_clk_out));
	block2  genClk_block_inst1(.enable(1'b1),  .blk2_genClk_out(blk2_genClk_out));
	
	always @ (posedge gen_clk_out or negedge rst)
    begin
    if (!rst)   blk1_out_reg[0] <= #1 dff_out1 ;
    else        blk1_out_reg[0] <= #1 dff_out2;
    end
	
	always @ (posedge blk2_genClk_out or negedge rst)
    begin
    if (!rst)   blk1_out_reg[1] <= #1 dff_out1 ;
    else        blk1_out_reg[1] <= #1 dff_out2;
    end
 	
  endmodule
	

	module block2 
  (
    input enable,
    output  blk2_genClk_out
);
    
    reg test1 ;
	wire genClk_out1;
	assign blk2_genClk_out = test1;

	clock_gen clk_gen_block_inst1( .enable(enable), .clk(genClk_out1));
	
	always @ (posedge genClk_out1 )
    begin
       test1 <= #1 genClk_out1;
    end
 	
  endmodule
	
	
module clk_divider (
    input clk_50mhz,
    input rst,
    output reg clk_1hz
);

parameter COUNT_MAX = 25000000; // To get 1Hz from 50MHz (50,000,000 / 2 = 25,000,000)
reg [24:0] counter = 0;

always @(posedge clk_50mhz or rst) begin
    if (rst) begin
        counter <= 0;
        clk_1hz <= 0;
    end else begin
        if (counter == COUNT_MAX - 1) begin
            counter <= 0;
            clk_1hz <= ~clk_1hz; // Toggle the clock
        end else begin
            counter <= counter + 1;
        end
    end
end

endmodule

