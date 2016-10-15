// Each counter value consists of 8 bits.
// The first 4 bits represent the BIC, ans the last 4 bits represent the BSC.
// Data status gets updated whenever the BSC turns from 1111 to 0000.

module sramTop(CLOCK_50, SW, KEY, LEDR, serialIn, serialOut);
	wire [7:0] parallelIn;
	reg[7:0] parallelOut = 8'bz;
	wire startSend;
	input serialIn;
	output serialOut;
	input 						CLOCK_50;
	input 	[9:0] 			SW;
	input		[3:0]				KEY;
	output 	[9:0] 			LEDR;
	
	// Each counter value consists of 17 bits.
// The first 4 bits represent the BIC, and the last 13 bits represent the BSC.
// Data status gets updated whenever the BSC turns from 1111111111111 to 0000000000000.
// The input clock signal is CLOCK_50 (50 MHz).
// The BIC counting interval is 163840 nanoseconds.
// About 1638400 nanoseconds of time is needed to send an 8-bit data.


	
	reg[16:0] sendCount = 17'b11110111111111111;
	reg [16:0] receiveCount = 17'b11110111111111111;
	reg[7:0] sendReg = 8'b00000000;
	reg [7:0] receiveReg = 8'b00000000;
	reg startBitOn;
	
			// Generate clk off of CLOCK_50, whichClock picks rate.
	wire [31:0] clk;
	parameter whichClock = 0;
	clock_divider cdiv (CLOCK_50, clk);
	
	initial begin
		sendCount = 17'b11110111111111111;
		receiveCount = 17'b11110111111111111;
		sendReg = 8'b00000000;
		receiveReg = 8'b00000000;
		parallelOut = 8'bz;
	end
	
	
	assign serialOut = (startBitOn | sendReg[0]);
	
	always @(posedge clk[whichClock]) begin
		case(sendCount)
			17'b11111111111111111: //Send the start bit.
			begin
				startBitOn <= 1;
				sendCount <= sendCount + 1;
			end
			17'b00001111111111111: //Stop sending the start bit. Load the data.
			begin
				startBitOn <= 0;
				sendReg <= parallelIn;
				sendCount <= sendCount + 1;
			end
			17'b10011111111111111: //Stop the sending process.
			begin
				sendCount <= 17'b11110111111111111;
				sendReg <= 8'b00000000;
			end
			17'b00011111111111111,
			17'b00101111111111111,
			17'b00111111111111111,
			17'b01001111111111111,
			17'b01011111111111111,
			17'b01101111111111111,
			17'b01111111111111111,
			17'b10001111111111111: //Send a bit.
			begin
				sendReg[7] <= 0;
				sendReg[6] <= sendReg[7];
				sendReg[5] <= sendReg[6];
				sendReg[4] <= sendReg[5];
				sendReg[3] <= sendReg[4];
				sendReg[2] <= sendReg[3];
				sendReg[1] <= sendReg[2];
				sendReg[0] <= sendReg[1];
				sendCount <= sendCount + 1;
			end
			17'b11110111111111111: //Start the sending process if "startSend" is on.
			begin
				sendCount <= sendCount + startSend;
			end
			default: sendCount <= sendCount + 1;
		endcase
		
		case(receiveCount)
			17'b10001111111111111: //Load the data.
			begin
				parallelOut <= receiveReg;
				receiveCount <= receiveCount + 1;
			end
			17'b10011111111111111: //Stop the receiving process.
			begin
				receiveCount <= 17'b11110111111111111;
				receiveReg <= 8'b00000000;
				parallelOut <= 8'bz;
			end
			17'b00001111111111111,
			17'b00011111111111111,
			17'b00101111111111111,
			17'b00111111111111111,
			17'b01001111111111111,
			17'b01011111111111111,
			17'b01101111111111111,
			17'b01111111111111111: //Receive a bit.
			begin
				receiveReg[7] <= serialIn;
				receiveReg[6] <= receiveReg[7];
				receiveReg[5] <= receiveReg[6];
				receiveReg[4] <= receiveReg[5];
				receiveReg[3] <= receiveReg[4];
				receiveReg[2] <= receiveReg[3];
				receiveReg[1] <= receiveReg[2];
				receiveReg[0] <= receiveReg[1];
				receiveCount <= receiveCount + 1;
			end
			17'b11110111111111111: //Start the receiving process if "serialIn" is on.
			begin
				receiveCount <= receiveCount + serialIn;
			end
			default: receiveCount <= receiveCount + 1;
		endcase
	end
	
	
		 niosIIe nios(
		  .clk_clk         		(CLOCK_50),    						//change clock?     
		  .reset_reset_n   		(!SW[9]),   
		  .switches_export 		(), 
		  .leds_export     		(LEDR),   		 
	//	  .data_export     		(DATA),     
	//	  .add_export  	 		(ADD),  
	//	  .we_export       		(WE),     
	//	  .oe_export       		(OE),
		  .parallel_in_export	(parallelIn),
		  .start_send_export 	(startSend),
		  .isreceiving_export	(isReceiving),
		  .issending_export		(isSending),
		  .parallel_out_export	(parallel_out)
	 );
	 

endmodule

// divided_clocks[0] = 25MHz, [1] = 12.5Mhz, ... [23] = 3Hz, [24] = 1.5Hz,
//[25] = 0.75Hz, ...
module clock_divider (clock, divided_clocks);
 input clock;
 output reg [31:0] divided_clocks;

 initial
 divided_clocks = 0;

 always @(posedge clock)
 divided_clocks = divided_clocks + 1;
endmodule 

module sramTop_testbench();
 logic clk;
 logic [3:0] KEY;
 logic [9:0] LEDR;
 logic [9:0] SW;
 logic serialIn, serialOut;
 sramTop dut (clk, SW, KEY, LEDR, serialIn, serialOut);

 	initial begin
	SW[9:0] = 10'b0;
	end
	
	
 // Set up the clock.
 parameter CLOCK_PERIOD=100;
 initial begin
 clk <= 0;
 serialIn <= 0;
 forever #(CLOCK_PERIOD/2) clk <= ~clk;
 end
 

	
 initial begin
  		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //1
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk);
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); serialIn <= 1;
		
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //2
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk);
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //3
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //4
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); serialIn <= 0;
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //5
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); serialIn <= 1;
		
		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //6
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk);
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //7
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); //8
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
		
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
		
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
		
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
		
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 	
 		@(posedge clk); @(posedge clk); @(posedge clk); @(posedge clk); 

		$stop; // End the simulation.
	end
		
		
endmodule

