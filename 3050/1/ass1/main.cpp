#include <QCoreApplication>
#include <iostream>
#include <string>
#include <unordered_map>
#include <bitset>
using namespace std;

unordered_map<string, int> regs = {
    {"zero", 0}, {"at", 1},  {"v0", 2},  {"v1", 3},  {"a0", 4},  {"a1", 5},
    {"a2", 6},   {"a3", 7},  {"t0", 8},  {"t1", 9},  {"t2", 10}, {"t3", 11},
    {"t4", 12},  {"t5", 13}, {"t6", 14}, {"t7", 15}, {"s0", 16}, {"s1", 17},
    {"s2", 18},  {"s3", 19}, {"s4", 20}, {"s5", 21}, {"s6", 22}, {"s7", 23},
    {"t8", 24},  {"t9", 25}, {"k0", 26}, {"k1", 27}, {"gp", 28}, {"sp", 29},
    {"fp", 30},  {"ra", 31}};

/*
 *  R-type funcode
 *  opcode = 0
 */

// shamt = 0
unordered_map<string, int> R_s = {};

// rt = shamt = 0
unordered_map<string, int> R_rt_s = {};

// rd = shamt = 0
unordered_map<string, int> R_rd_s = {};

// rs = rt = shamt = 0
unordered_map<string, int> R_rs_rt_s = {
    {"mfhi", 0x10}, {"mflo", 0x12},
};

// rt = rd = shamt = 0
unordered_map<string, int> R_rt_rd_s = {
    {"mthi", 0x11}, {"mtlo", 0x13},
};

// all non-zero
unordered_map<string, int> R_func = {
    {"add", 0b100000},   {"addu", 0b100001},    {"and", 0b100100},
    {"break", 0b001101}, {"clo", 0x21},         {"clz", 0x20},
    {"div", 0b011010},   {"divu", 0b011011},    {"jalr", 0b001001},
    {"jr", 0b001000},    {"mfhi", 0b010000},    {"mflo", 0b010010},
    {"mthi", 0b010001},  {"mtlo", 0b010011},    {"mult", 0b011000},
    {"multu", 0b011001}, {"nor", 0b100111},     {"or", 0b100101},
    {"sll", 0b000000},   {"sllv", 0b000100},    {"slt", 0b101010},
    {"sltu", 0b101011},  {"sra", 0b000011},     {"srav", 0b000111},
    {"srl", 0b000010},   {"srlv", 0b000110},    {"sub", 0b100010},
    {"subu", 0b100011},  {"syscall", 0b001100}, {"xor", 0b100110}};

// I-type opcode
unordered_map<string, int> I_op = {
    {"addi", 0b001000}, {"addiu", 0b001001}, {"andi", 0b001100},
    {"beq", 0b000100},  {"bgez", 0b000001},  {"bgtz", 0b000111},
    {"blez", 0b000110}, {"bltz", 0b000001},  {"bne", 0b000101},
    {"lb", 0b100000},   {"lbu", 0b100100},   {"lh", 0b100001},
    {"lhu", 0b100101},  {"lui", 0b001111},   {"lw", 0b100011},
    {"lwc1", 0b110001}, {"ori", 0b001101},   {"sb", 0b101000},
    {"slti", 0b001010}, {"sltiu", 0b001011}, {"sh", 0b101001},
    {"sw", 0b101011},   {"swc1", 0b111001},  {"xori", 0b001110}};

// RI-type rt
// opcode = 1
unordered_map<string, int> RI_rt = {
    {"bltz", 0b00000},    {"bgez", 0b00001},   {"bltzl", 0b00010},
    {"bgezl", 0b00011},   {"tgei", 0b01000},   {"tlti", 0b01010},
    {"tltiu", 0b01011},   {"teqi", 0b01100},   {"tnei", 0b01110},
    {"bltzal", 0b10000},  {"bgezal", 0b10001}, {"bltzall", 0b10010},
    {"bgezall", 0b10011}, {"synci", 0b11111}};

/* --------------------R-type--------------------
 * op       rs      rt      rd      shamt   funct
 * 000000   5       5       5       5       6
 */
int makeR(int rs, int rt, int rd, int shamt, int func) {
  int R = rs << 21;

  R |= rt << 16;
  R |= rd << 11;
  R |= shamt << 6;
  R |= func;
  return R;
}

/* --------------------I-type--------------------
 * op      rs      rt      imm
 * 6       5       5       16
 */
int makeI(int op, int rs, int rt, int imm) {
  int I = op << 26;

  I |= rs << 21;
  I |= rt << 16;
  I |= imm;
  return I;
}

/* --------------------J-type--------------------
 * op      addr
 * 6       26
 */
int makeJ(int op, int addr) {
  int J = op << 26;

  J |= addr;
  return J;
}

/*
 * Given a single-line string instruction
 * return the corresponding machine code
 */
int make(string instr) {
  string name = "";
  int space_pos = instr.find(" ");

  for (int i = 0; i < space_pos; i++) {
    name += instr[i];
  }

  // R-type
  // add $s1, $s2, $s3
  if (R_func.find(name) != R_func.end()) {
    int func = R_func[name];
    int rd_beg = space_pos + 2;
    string rd_str, rs_str, rt_str = "";
    int reg = 0;

    for (int i = rd_beg; i < int(instr.length()); i++) {
      char curr = instr[i];

      if (curr == ',') {
        i += 2;
        reg += 1;
      }

      else {
        switch (reg) {
        case 0:
          rd_str += curr;
          break;

        case 1:
          rs_str += curr;
          break;

        default:
          rt_str += curr;
        }
      }
    }

    int rd = regs[rd_str];
    int rs = regs[rs_str];
    int rt = regs[rt_str];

    cout << "rd: " << rd_str << "\nrs: " << rs_str << "\nrt: " << rt_str
         << endl;
  }

  // I-type
  else if (I_op.find(name) != I_op.end()) {
    int op = I_op[name];
  }

  // RI-type
  else if (RI_rt.find(name) != I_op.end()) {
    int rt = RI_rt[name];
  }

  // J-type
  else {
  }
}

int main() {
  /*int rs = 0b11111;
     int rt = 0b10000;
     int rd = 0b00001;
     int shamt = 0b00000;
     int func = 0b001101;
     int instr = makeR(rs, rt, rd, shamt, func);
     bitset<32> x(instr);

     cout << (x);
   */
  string instr = "add $zero, $s2, $s3";

  make(instr);
}
