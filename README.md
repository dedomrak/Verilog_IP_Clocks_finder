
# Verilog_IP_Clocks_finder

Verilog IP clocks finder is a tool that can automatically find clocks in an IP design and report them in a list in the console. The clocktree is traced from the top module to the pin of the submodule to which this clock reaches. Each clock starting in the top module is considered a master clock and the others whose sources are deeper in the hierarchy are labeled as generated/derived clocks. The connection of these derived clocks to the master clocks can be traced through the clocktree network. Since this is a simple tool that only extracts clocks and their nets, it does not report other parameters such as timing analysis, clock domain crossing or clock gating. These features are reported in proffesional and commercial tools like Synopsis Primetime. The analysis of the IP design graph is also simplified because the elaboration step producing the full design netlist that is performed by the Verific parser is missing.                                                   
Since the Verific library is a commercial source that is paid for, it is not included here.                 
For information about the Verific Verilog parser and library, see www.verific.com. 


## Build
Open the provided .pro file in QTCreator and run the build.
Tested and built in Windows 10, it should open and convert without problems in Linux(Ubuntu,Fedora etc.).
For a successful build, you need the full source code of Verific library.
Under Linux, after opening the .pro file and run qmake, a full Makefile is written, which can also be built with the make command.
## Usage

```javascript
Usage: VIP clock finder:
        VIP_clock_finder         <input Verilog IP files>

```


## Example log 

```javascript
-- Analyzing Verilog file 'F:\XXXXXXXXXXXXXXXXXXXXXXXXX\veri_test\clock_hier.v' (VERI-1482)
F:\XXXXXXXXXXXXXXXXXXXXX\veri_test\clock_hier.v(23): INFO: analyzing included file 'D_flipflop.v' (VERI-1328)
D_flipflop.v(23): INFO: back to file 'D_flipflop.v' (VERI-2320)
D_flipflop.v(24): INFO: analyzing included file 'clock_gen.v' (VERI-1328)
clock_gen.v(24): INFO: back to file 'clock_gen.v' (VERI-2320)
clock_gen.v(21): INFO: Searching module d_ff for clocked statements...
clock_gen.v(206): INFO: Searching module top for clocked statements...
clock_gen.v(166): INFO: visiting this event control statement
clock_gen.v(182): INFO: visiting this event control statement
clock_gen.v(192): INFO: visiting this event control statement
clock_gen.v(198): INFO: visiting this event control statement
clock_gen.v(204): INFO: visiting this event control statement
clock_gen.v(182): INFO:    signal clk is a clock of this statement
clock_gen.v(182): INFO:    signal rst is an asynchronous set/reset condition of this statement
clock_gen.v(192): INFO:    signal clk is a clock of this statement
clock_gen.v(21): INFO: Found module 'd_ff', will visit its items
clock_gen.v(64): INFO: Found module 'clock_gen', will visit its items
clock_gen.v(206): INFO: Found module 'top', will visit its items
clock_gen.v(169): INFO:       => Found instance clk_divider divClk_inst (.clk_50mhz(clk), .rst(rst), .clk_1hz(div_clk_1Hz)) ;
clock_gen.v(170): INFO:       => Found instance d_ff dff_top_inst1 (.q(dff_inst1_out), .d(in[1]), .rst_n(rst), .clk(div_clk_1Hz)) ;
clock_gen.v(171): INFO:       => Found instance clock_gen clk_gen_bl1 (.enable(1'b1), .clk(gen_clk_mod_out)) ;
clock_gen.v(172): INFO:       => Found instance block1 block_inst1 (.clk(clk), .rst(rst), .data({in[7],in[8]}), .blk1_out_reg({out1[7],out1[8]})) ;
clock_gen.v(238): INFO: Found module 'block1', will visit its items
clock_gen.v(221): INFO:       => Found instance clock_gen clk_gen_block_inst1 (.enable(1'b1), .clk(gen_clk_out)) ;
clock_gen.v(222): INFO:       => Found instance d_ff dff_blk1_inst1 (.q(dff_out1), .d(data[0]), .rst_n(rst), .clk(clk)) ;
clock_gen.v(223): INFO:       => Found instance d_ff dff_blk1_inst2 (.q(dff_out2), .d(data[1]), .rst_n(rst), .clk(gen_clk_out)) ;
clock_gen.v(224): INFO:       => Found instance block2 genClk_block_inst1 (.enable(1'b1), .blk2_genClk_out(blk2_genClk_out)) ;
clock_gen.v(258): INFO: Found module 'block2', will visit its items
clock_gen.v(251): INFO:       => Found instance clock_gen clk_gen_block_inst1 (.enable(enable), .clk(genClk_out1)) ;
clock_gen.v(284): INFO: Found module 'clk_divider', will visit its items

 ##############################################################################
 #      Design summary:
 ##############################################################################
 #      Top module:                             top
 ##############################################################################

 ##############################################################################
 #      Master clocks:
 #-----------------------------------------------------------------------------
 #      Clock name:                              clk
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              clk
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              div_clk_1Hz
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              gen_clk_mod_out
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Derived/Generated clocks:
 #-----------------------------------------------------------------------------
 #      Clock name:                              blk2_genClk_out
 #      Clocktree paths:                         top/block_inst1/
 #-----------------------------------------------------------------------------
 #      Clock name:                              clk
 #      Clocktree paths:                         top/dff_top_inst1/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              clk_50mhz
 #      Clocktree paths:                         top/divClk_inst/clk_50mhz
 #-----------------------------------------------------------------------------
 #      Clock name:                              div_clk_1Hz
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              gen_clk_mod_out
 #      Clocktree paths:                         top/clk
 #-----------------------------------------------------------------------------
 #      Clock name:                              gen_clk_out
 #      Clocktree paths:                         top/block_inst1/
 #-----------------------------------------------------------------------------
 #      Clock name:                              start_clk
 #      Clocktree paths:                         top/clk_gen_bl1/
 #-----------------------------------------------------------------------------
 ##############################################################################

 ##############################################################################
 #      End of Design summary!
 ##############################################################################

```


## Screenshots
## Screenshots
Printscreen png files are located in ./doc folder                                                                                 
Report clock hierarchy:

![Alt text](/doc/clock_hierarchy.jpg?raw=true "Optional Title")

Report dual_clocks_FIFO IP:
![Alt text](doc/dual_clock_FIFO1.jpg?raw=true "Optional Title")
![Alt text](doc/dual_clock_FIFO2.jpg?raw=true "Optional Title")


