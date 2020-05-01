`timescale 1ns/1ps

`define rt 0
`define rs 1
`define rd 1
`define imm 2
`define signImm 3
`define shamt 4

`define add 0
`define sub 1
/*
`define mult 2
`define multu 3
`define div 4
`define divu 5
*/
`define AND 6
`define NOR 7
`define OR 8
`define XOR 9
`define slt 10
`define sltu 11
`define sll 12
`define srl 13
`define sra 14

module alu(
	input wire clock, start,
	input[31 : 0] instr, // instruction input
	input[31 : 0] readData, // data input
	output[31 : 0] writeData // data output
);

// EIGHT 32-BIT GENERAL REGISTERS
reg[31 : 0]gr[7 : 0];

// SPLITING OF INSTRUCTOIN
reg[5 : 0] opcode, funct; // operation and function codes
reg[5 : 0] rs_no, rt_no, rd_no; // register number, NOT NEEDED IN THIS PROJECT
reg[5 : 0] shamt; // shift amount
reg[15: 0] imm; // immediate value

// CONTROL UNIT
integer ALUctrl; // specifies the operation to be performed
reg branchEQ, branchNE; // enables loading branch target into PC
reg PCSrc = 0;
reg memWrite, regWrite; // enables data write to memory/register
reg regDst; // determines dest. register: 0 -> rt; 1 -> rd
reg memToReg; // determines data source to be written into register: 0 -> ALUResult; 1 -> memory
reg srcActrl; // determines source of operand A: 0 -> rt; 1 -> rs
integer srcBctrl; // determines source of operand B: 0 -> rt; 1 -> rs; 2 -> imm; 3 -> signImm; 4 -> shamt

// SIGN EXTENSION
reg[31 : 0] signImm; // sign extended imm

// MAIN ALU
reg[31 : 0] srcA, srcB; // ALU operands
reg[31 : 0] ALUResult, result;
reg zeroFlag, negFlag, overFlag; // status

// PROGRAM COUNTER
reg[31 : 0] PC = 0;

always @(start) begin
	gr[0] = 0;
end

always @(posedge clock) begin
	// conditonally branch, or unconditonally +4 to PC, depending on PCSrc
	PC <= PCSrc? PC + 4 + (signImm << 2) : PC + 4;
end

always @(PC) begin
	// Splitting
    opcode = instr[31 : 26];
    funct  = instr[05 : 00];
	rs_no  = instr[25 : 21];
	rt_no  = instr[20 : 16];
	rd_no  = instr[15 : 11];
	shamt  = instr[10 : 06];
	imm    = instr[15 : 00];

	// Sign Extend
	signImm = $signed(imm);

	// Control Unit
	branchEQ = 0; // default: no branch
	branchNE = 0; // default: no branch
	memWrite = 0; // default: disable memory write
	regWrite = 1; // default: enable reg write
	memToReg = 0; // default: if write to reg, src = ALUResult
	case(opcode)
		0:	// R type
		begin
		regDst = `rd;
		case(funct)
			0:  //sll
			begin
			ALUctrl = `sll;
			srcActrl = `rt;
			srcBctrl = `shamt;
		    end

            2:  //srl
			begin
			ALUctrl = `srl;
			srcActrl = `rt;
			srcBctrl = `shamt;
		    end

			3:  // sra
			begin
			ALUctrl = `sra;
			srcActrl = `rt;
			srcBctrl = `shamt;
		    end

            4:  // sllv
			begin
			ALUctrl = `sll;
			srcActrl = `rt;
			srcBctrl = `rs;
		    end
			
			6:  // srlv
			begin
			ALUctrl = `srl;
			srcActrl = `rt;
			srcBctrl = `rs;
		    end

			7:  // srav
			begin
			ALUctrl = `sra;
			srcActrl = `rt;
			srcBctrl = `rs;
		    end

			8'h20:  // add
			begin
			ALUctrl = `add;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

			8'h21:	// addu
			begin
			ALUctrl = `add;
			srcActrl = `rs;
			srcBctrl = `rt;    
			end

			8'h22:  // sub
			begin
			ALUctrl = `sub;
			srcActrl = `rs;
			srcBctrl = `rt;
			end

			8'h23:  // subu
			begin
			ALUctrl = `sub;
			srcActrl = `rs;
			srcBctrl = `rt;
			end

			/*8'h18:  // mult
			begin
			ALUctrl = `mult;
			srcActrl = `rs;
			srcBctrl = `rt;
			end

			8'h19:  // multu
			begin
			ALUctrl = `multu;
			srcActrl = `rs;
			srcBctrl = `rt;
			end

			8'h1a:  // div
			begin
			ALUctrl = `div;
			srcActrl = `rs;
			srcBctrl = `rt;
			end

			8'h1b:  // divu
			begin
			ALUctrl = `divu;
			srcActrl = `rs;
			srcBctrl = `rt;
			end*/

			8'h24:  // and
			begin
			ALUctrl = `AND;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

			8'h25:  // or
			begin
			ALUctrl = `OR;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

			8'h26:  // xor
			begin
			ALUctrl = `XOR;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

			8'h27:  // nor
			begin
			ALUctrl = `NOR;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

			8'h2a:  // slt
			begin
			ALUctrl = `slt;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end
			
			8'h2b:  // sltu
			begin
			ALUctrl = `sltu;
			srcActrl = `rs;
			srcBctrl = `rt;
		    end

        endcase

        end

		4:  // beq
		begin
		ALUctrl = `sub;
		srcActrl = `rs;
		srcBctrl = `rt;
		regWrite = 0;
		branchEQ = 1;
		end

		5:  // bne
		begin
		ALUctrl = `sub;
		srcActrl = `rs;
		srcBctrl = `rt;
		regWrite = 0;
		branchNE = 1;
		end
		
		8:  // addi
		begin
		ALUctrl = `add;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regDst = `rt;
		end

		9:  // addiu
		begin
		ALUctrl = `add;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regDst = `rt;
		end

		8'h0a:  // slti
		begin
		ALUctrl = `slt;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regDst = `rt;
		end

		8'h0b:  // sltiu
		begin
		ALUctrl = `sltu;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regDst = `rt;
		end
		
		8'h0c:  // andi
		begin
		ALUctrl = `AND;
		srcActrl = `rs;
		srcBctrl = `imm;
		regDst = `rt;
		end

		8'h0d:  // ori
		begin
		ALUctrl = `OR;
		srcActrl = `rs;
		srcBctrl = `imm;
		regDst = `rt;
		end

		8'h0e:  // xori
		begin
		ALUctrl = `XOR;
		srcActrl = `rs;
		srcBctrl = `imm;
		regDst = `rt;
		end

		8'h23:  // lw
		begin
		ALUctrl = `add;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regDst = `rt;
		memToReg = 1; // source = memory
		end

		8'h2b:  // sw
		begin
		ALUctrl = `add;
		srcActrl = `rs;
		srcBctrl = `signImm;
		regWrite = 0; // disables register write
		memWrite = 1; // enables memory write
		end

	endcase
	
	case (srcActrl)
		`rs: srcA = gr[rs_no];
		`rt: srcA = gr[rt_no];
	endcase

	case (srcBctrl)
		`rs:  srcB = gr[rs_no];
		`rt:  srcB = gr[rt_no];
		`imm: srcB = imm;
		`shamt: srcB = shamt;
		`signImm: srcB = signImm;
	endcase

	// main ALU
	case (ALUctrl)
		`add: begin
			ALUResult = srcA + srcB;
			overFlag = ((srcA[31] == srcB[31]) && (srcA[31] != ALUResult[31]));
			end
		
		`sub: begin
			ALUResult = srcA - srcB;
			overFlag = ((srcA[31] != srcB[31]) && (srcA[31] != ALUResult[31]));
			end

		/*`mult: begin
			acc = $signed(srcA) * $signed(srcB);
			hiResult = acc[63 : 32];
			loResult = acc[31 : 00];
			end

		`multu: begin
			acc = srcA * srcB;
			hiResult = acc[63 : 32];
			loResult = acc[31 : 00];
			end

		`div: begin
			hiResult = $signed(srcA) % $signed(srcB);
			loResult = $signed(srcA) / $signed(srcB);
			end

		`divu: begin
			hiResult = srcA % srcB;
			loResult = srcA / srcB;
			end*/

		`AND: begin
			ALUResult = srcA & srcB;
			end
		
		`NOR: begin
			ALUResult = srcA ~| srcB;
			end

		`OR: begin
			ALUResult = srcA | srcB;
			end
		
		`XOR: begin
			ALUResult = srcA ^ srcB;
			end

		`slt: begin
			ALUResult = $signed(srcA) < $signed(srcB);
			end

		`sltu: begin
			ALUResult = srcA < (srcB & 32'h1f); // shamt reduced mod 32
			end
		
		`sll: begin
			ALUResult = srcA << (srcB & 32'h1f); // shamt reduced mod 32
			end

		`srl: begin
			ALUResult = srcA >> (srcB & 32'h1f); // shamt reduced mod 32
			end

		`sra: begin
			ALUResult = srcA >>> (srcB & 32'h1f); // shamt reduced mod 32
			end
	endcase

	zeroFlag = !ALUResult;
	negFlag = ALUResult[31];

	PCSrc = (branchEQ & zeroFlag) | (branchNE & !zeroFlag);
	result = memToReg? readData : ALUResult;

	// writeback to register
	if (regWrite) begin
		gr[regDst? rd_no : rt_no] = result;
	end
end

assign writeData = memWrite? gr[rt_no] : 32'hx;
/*assign hi = hiResult;
assign lo = loResult;
assign zero = zeroFlag;
assign neg = negFlag;
assign overflow = overFlag;*/

endmodule