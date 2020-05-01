`timescale 1ns/1ps

// general register
`define gr0  	5'b00000
`define gr1  	5'b00001
`define gr2  	5'b00010
`define gr3 	5'b00011
`define gr4  	5'b00100
`define gr5  	5'b00101
`define gr6  	5'b00110
`define gr7  	5'b00111

module CPU_test;

    // Inputs
	reg clock;
	reg [31:0] d_datain, i_datain;
    wire [31:0] d_dataout;

    alu uut(
        .clock(clock),
        .instr(i_datain),
		.readData(d_datain),
        .writeData(d_dataout)
    );

    initial begin
    // Initialize Input
    clock = 1;

    $display("pc      :  instr :  srcA  :  srcB  :ALUreslt: result :readData:writData:  gr1   :  gr2   :  gr3   :rW");
    $monitor("%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h",
        uut.PC, uut.instr, uut.srcA, uut.srcB, uut.ALUResult, uut.result, d_datain, uut.writeData, uut.gr[1], uut.gr[2], uut.gr[3],uut.regWrite);

    /*Test:*/
    #T
    d_datain <= 32'h0000_00ab;  // readData
    i_datain <= {6'b100011, `gr0, `gr1, 16'h0001};  // instr

    #T
    d_datain <= 32'h0000_3c00;
    i_datain <= {6'b100011, `gr0, `gr2, 16'h0002};

    #T
    i_datain <= {6'b000000, `gr1, `gr2, `gr3, 5'b00000, 6'b100000};

    #T
    i_datain <= 32'hAC03FFFF;

    #T $finish;
    end

parameter T = 10;
always #5 clock = ~clock;
endmodule