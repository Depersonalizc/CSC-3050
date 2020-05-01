`timescale 1ns/1ps

module alu_test;

reg[31 : 0] i_datain, gr1, gr2;
wire[31 : 0] out, hi, lo;
wire zero, overflow, neg;

alu testalu(i_datain, gr1, gr2, out, hi, lo, zero, overflow, neg);

initial
begin
$display("instruction:op:func:   gr1  :   gr2  :  srcA  :  srcB  :   out  :   hi   :   lo   :z:o:n:PCSrc:memW:regW:regD:mem2Reg:ALUctrl:PC");
$monitor("%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%h:%1d:%h",
        i_datain, testalu.opcode, testalu.funct, gr1, gr2, testalu.srcA, testalu.srcB, out, hi, lo, zero, overflow, neg, testalu.PCSrc, testalu.memWrite, testalu.memWrite, testalu.regDst,testalu.memToReg, testalu.ALUctrl,testalu.PC);

#10 // add
gr1 <= 32'b1000_1001_1001_1001_1001_1001_1001_1001;
gr2 <= 32'b0101_1101_1101_1101_1101_1101_1101_1101;
i_datain <= 32'h014B4820;

#10 // addu
i_datain <= 32'b00000010001100101000000000100001;

#10 // sub
i_datain <= 32'h014B4822;

// subu
#10 i_datain <= 32'b00000010001100101000000000100011;

// beq
#10 i_datain <= 32'h112A0001;

#10 // and
i_datain <= 32'h014B4824;

#10 // or
i_datain <= 32'b00000010001100101000000000100101;

// nor
#10 i_datain <= 32'b00000010001100101000000000100111;

// xor
#10 i_datain <= 32'b00000001001010100100000000100110;

#10 // div
i_datain <= 32'h012A001A;

#10 // divu
i_datain <= 32'h012A001B;

#10 // mult
i_datain <= 32'h012A0018;

#10 // multu
i_datain <= 32'h012A0019;

#10 // sll
i_datain <= 32'b0000_0000_0000_0001_0001_0000_0100_0000;

// srl (imm = 10)
#10 i_datain <= 32'b00000000000100011000001010000010;

// srlv
#10 i_datain <= 32'b00000010011100011000000000000110;

// sra (imm = 10)
#10 i_datain <= 32'b00000000000100011000001010000011;

// srav
#10 i_datain <= 32'b00000010011100011000000000000111;

// slt
#10 i_datain <= 32'b00000010001100111000000000101010;

// sltu
#10 i_datain <= 32'b00000010001100111000000000101011;

// addi (imm = 0x8000)
#10 i_datain <= 32'h21498000;

// addiu (imm = 10)
#10 i_datain <= 32'b00100110001100000000000001100100;

// andi (imm = 10)
#10 i_datain <= 32'b00110010001100000000000001100100;

// ori (imm = 10)
#10 i_datain <= 32'b00110110001100000000000001100100;

// xori (imm = 10)
#10 i_datain <= 32'b00111010001100000000000001100100;

// slti (imm = 10)
#10 i_datain <= 32'b00101010001100000000000001100100;

// sltiu (imm = 10)
#10 i_datain <= 32'b00101110001100000000000001100100;

// beq (imm = -1)
#10 i_datain <= 32'b00010010000100011111111111111111;

// bne (imm = -1)
#10 i_datain <= 32'b00010110000100011111111111111111;

// lw (imm = 100)
#10 i_datain <= 32'b10001110001100000000000001100100;

// sw (imm = 100)
#10 i_datain <= 32'b10101110001100000000000001100100;

// sllv
#10 i_datain <= 32'b00000001010010010100000000000100;

#10 $finish;
end
endmodule