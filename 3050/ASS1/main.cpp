#include <QCoreApplication>
#include <iostream>
#include <string>
#include <unordered_map>
#include <bitset>
#include <stack>
#include <fstream>
#include <vector>
using namespace std;

int ln_idx = 0;
int mach_code, imm;
uint8_t op, rd, rs, rt, shamt, func;
string in_path = "C:\\Users\\chen1\\Desktop\\testfile.asm";
string out_path = "C:\\Users\\chen1\\Desktop\\testfile.txt";
string curr_ln;
string nums = "0123456789";
ifstream f(in_path);
ofstream o(out_path);
unordered_map<string, int> labels;
vector<string> instructions;

unordered_map<string, uint8_t> regs = {
    {"$zero", 0}, {"$at", 1},  {"$v0", 2},  {"$v1", 3},  {"$a0", 4},
    {"$a1", 5},   {"$a2", 6},  {"$a3", 7},  {"$t0", 8},  {"$t1", 9},
    {"$t2", 10},  {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14},
    {"$t7", 15},  {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19},
    {"$s4", 20},  {"$s5", 21}, {"$s6", 22}, {"$s7", 23}, {"$t8", 24},
    {"$t9", 25},  {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29},
    {"$fp", 30},  {"$ra", 31}};

/*
 *  ========================= R-type funcode =========================
 *  opcode = 0 or 0x1c
 *  ------------------------------------------------------------------
 *  op       rs      rt      rd      shamt   funct
 *  000000   5       5       5       5       6
 *  ------------------------------------------------------------------
 */

// The 7 special R-type's with opcode 0x1c
// string R_spc[7] = {"mul", "clo", "clz", "madd", "maddu", "msub", "msubu"};

unordered_map<string, uint8_t> R_dst = {
    // instr rd, rs, rt
    {"add", 0b100000},  {"addu", 0b100001}, {"and", 0b100100},
    {"mul", 2},         {"nor", 0b100111},  {"or", 0b100101},
    {"sllv", 0b000100}, {"srav", 0b000111}, {"srlv", 0b000110},
    {"sub", 0b100010},  {"subu", 0b100011}, {"xor", 0b100110},
    {"slt", 0b101010},  {"sltu", 0b101011},
};

unordered_map<string, uint8_t> R_dts = {
    // instr rd, rt, shamt
    {"sll", 0b000000},
    {"sra", 0b000011},
    {"srl", 0b000010},
};

unordered_map<string, uint8_t> R_ds = {
    // instr rd, rs
    {"clo", 0x21},
    {"clz", 0x20}};

unordered_map<string, uint8_t> R_st = {
    // instr rs, rt
    {"div", 0b011010},   {"divu", 0b011011}, {"mult", 0b011000},
    {"multu", 0b011001}, {"madd", 0},        {"maddu", 1},
    {"msub", 4},         {"msubu", 5},       {"teq", 0x34},
    {"tneq", 0x36},      {"tge", 0x30},      {"tgeu", 0x31},
    {"tlt", 0x32},       {"tltu", 0x33},
};

unordered_map<string, uint8_t> R_d = {
    // instr rd
    {"mfhi", 0b010000},
    {"mflo", 0b010010}};

unordered_map<string, uint8_t> R_s = {
    // instr rs
    {"mthi", 0b010001},
    {"mtlo", 0b010011},
    {"jr", 0b001000},
};

/*
 *  ======================= I-type opcode/rt ========================
 *  one-to-one opcode or opcode = 1
 *  -----------------------------------------------------------------
 *  op      rs      rt      imm
 *  6       5       5       16
 *  -----------------------------------------------------------------
 */

unordered_map<string, uint8_t> I_tsi = {
    // instr rt, rs, imm
    // {"instr", op}
    {"addi", 0b001000},  {"addiu", 0b001001}, {"andi", 0b001100},
    {"ori", 0b001101},   {"xori", 0b001110},  {"slti", 0b001010},
    {"sltiu", 0b001011},
};

unordered_map<string, uint8_t> I_ls = {
    // instr rt, imm(rs)
    // format: {"instr", op}
    {"lb", 0b100000}, {"lbu", 0b100100},  {"lh", 0b100001}, {"lhu", 0b100101},
    {"lw", 0b100011}, {"lwc1", 0b110001}, {"lwl", 0x22},    {"lwr", 0x26},
    {"ll", 0x30},     {"sb", 0b101000},   {"sh", 0b101001}, {"sw", 0b101011},
    {"swl", 0x2a},    {"swr", 0x2e},      {"sc", 0x38}};

unordered_map<string, uint8_t> I_b_stl = {
    // instr rs, rt, label
    // {"branch", op}
    {"beq", 0b000100},
    {"bne", 0b000101}};

unordered_map<string, uint8_t> I_b_sl = {
    // instr rs, label
    // {"branch", op}
    {"bgtz", 0b000111},
    {"blez", 0b000110}};

unordered_map<string, uint8_t> I_b1 = {
    // instr rs, label
    // opcode = 1
    // {"branch", rt}
    {"bltz", 0b00000},
    {"bgez", 0b00001},
    {"bltzal", 0b10000},
    {"bgezal", 0b10001}};

unordered_map<string, uint8_t> I_t = {
    // instr rs, imm
    // opcode = 1
    // {"trap", rt}
    {"tgei", 0b01000},  {"tgeiu", 9},      {"tlti", 0b01010},
    {"tltiu", 0b01011}, {"teqi", 0b01100}, {"tnei", 0b01110}};

/*
 *  ======================== J-type opcode =========================
 *  op      addr
 *  6       26
 *  ----------------------------------------------------------------
 */

unordered_map<string, uint8_t> J = {{"j", 2}, {"jal", 3}};

string pop(stack<string> &s) {
  string x = s.top();
  s.pop();
  return x;
}

bool is_num_str(string str) {
  return (str.find_first_not_of(nums) == str.npos);
}

bool is_empty_str(string str) {
  for (auto c = str.begin(); c < str.end(); c++) {
    if (*c != '\t' && *c != ' ')
      return false;
  }
  return true;
}

int makeR(uint8_t op, uint8_t func, uint8_t rs, uint8_t rt, uint8_t rd,
          uint8_t shamt = 0) {
  int R = op << 26;
  R |= rs << 21;
  R |= rt << 16;
  R |= rd << 11;
  R |= shamt << 6;
  R |= func;
  return R;
}

int makeI(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm) {
  int I = op << 26;
  I |= rs << 21;
  I |= rt << 16;
  I |= imm;
  return I;
}

int makeJ(uint8_t op, int ln_idx) {
  int J = op << 26;
  J |= ln_idx;
  return J;
}

/*
 * Break a single-line string instruction into corresponding stack
 * break_instr("add $s1, $s2, $s3") == <"add", "$s1", "$s2", "$s3">
 * break_instr("sw $s1, $s2(100)") == <"sw", "$s1", "$s2(100)">
 */
stack<string> break_instr(string instr) {
  string arg = "";
  string name;
  stack<string> args;

  for (int i = 0; i < int(instr.length()); i++) {
    char curr = instr[i];

    switch (curr) {
    case ' ':
      name = arg;
      arg = "";
      break;
    case ',':
      args.push(arg);
      arg = "";
      i += 1;
      break;
    default:
      arg += curr;
    }
  }
  args.push(arg);
  args.push(name);
  return args;
}

/*
 * Given a single-line string instruction (already shrunk)
 * return the corresponding 4-byte machine code
 */
int make(string instruction, unordered_map<string, int> labels) {

  // finding name of the instruction
  stack<string> instr = break_instr(instruction);
  string name = pop(instr);

  // R-type

  // jalr rs(, rd = 31)
  if (name == "jalr") {
    if (instr.size() == 2)
      rd = 31; // default rd = 31
    else
      rd = regs[pop(instr)]; // user input rd

    rs = regs[pop(instr)];

    return makeR(0, 9, rs, 0, rd);
  }

  // R rd, rs, rt
  else if (R_dst.find(name) != R_dst.end()) {
    func = R_dst[name];
    rt = regs[pop(instr)];
    rs = regs[pop(instr)];
    rd = regs[pop(instr)];

    if (func == 2)
      op = 0x1c; // mul
    else
      op = 0;

    return makeR(op, func, rs, rt, rd);
  }

  // R rd, rt, shamt
  else if (R_dts.find(name) != R_dts.end()) {
    func = R_dts[name];
    shamt = stoi(pop(instr));
    rt = regs[pop(instr)];
    rd = regs[pop(instr)];

    return makeR(0, func, 0, rt, rd, shamt);
  }

  // R rd, rs
  else if (R_ds.find(name) != R_ds.end()) {
    func = R_ds[name];
    rs = regs[pop(instr)];
    rd = regs[pop(instr)];

    return makeR(0x1c, func, rs, 0, rd);
  }

  // R rs, rt
  else if (R_st.find(name) != R_st.end()) {
    func = R_st[name];
    rt = regs[pop(instr)];
    rs = regs[pop(instr)];

    if (func == 0 || func == 1 || func == 4 ||
        func == 5) // "madd", "maddu", "msub", "msubu"
      op = 0x1c;
    else
      op = 0;

    return makeR(op, func, rs, rt, 0);
  }

  // R rd
  else if (R_d.find(name) != R_d.end()) {
    func = R_d[name];
    rd = regs[pop(instr)];

    return makeR(0, func, 0, 0, rd);
  }

  // R rs
  else if (R_s.find(name) != R_s.end()) {
    func = R_s[name];
    rs = regs[pop(instr)];

    return makeR(0, func, rs, 0, 0);
  }

  // I-type

  // lui rt, imm
  else if (name == "lui") {
    imm = stoi(pop(instr));
    rt = regs[pop(instr)];

    return makeI(15, 0, rt, imm);
  }

  // I rt, rs, imm
  else if (I_tsi.find(name) != I_tsi.end()) {
    imm = stoi(pop(instr));
    op = I_tsi[name];
    rs = regs[pop(instr)];
    rt = regs[pop(instr)];
    return makeI(op, rs, rt, imm);
  }

  // I rt, imm(rs)
  else if (I_ls.find(name) != I_ls.end()) {
    int c = 0;
    int i = 0;
    string imm_str, rs_str = "";
    string imm_rs = pop(instr); // imm_rs == "imm(rs)"
    rt = regs[pop(instr)];
    op = I_ls[name];


    while (1) {
      char curr = imm_rs[i];
      if (curr == '(')
        c += 1;
      else if (curr == ')')
        break;
      else {
        if (c == 0)
          imm_str += curr;
        else
          rs_str += curr;
      }
      i += 1;
    }

    imm = stoi(imm_str);
    rs = regs[rs_str];

    return makeI(op, rs, rt, imm);
  }

  // I rs, rt, label/#ln
  else if (I_b_stl.find(name) != I_b_stl.end()) {
    op = I_b_stl[name];

    string lab_ln = pop(instr);
    rt = regs[pop(instr)];
    rs = regs[pop(instr)];

    if (is_num_str(lab_ln))
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }

    return makeI(op, rs, rt, imm);
  }

  // I rs, label/#ln (rt = 0)
  else if (I_b_sl.find(name) != I_b_sl.end()) {
    op = I_b_sl[name];
    string lab_ln = pop(instr);
    rs = regs[pop(instr)];

    if (is_num_str(lab_ln))
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }

    return makeI(op, rs, 0, imm);
  }

  // I rs, label/#ln
  else if (I_b1.find(name) != I_b1.end()) {
    rt = I_b1[name];
    string lab_ln = pop(instr);
    rs = regs[pop(instr)];

    if (is_num_str(lab_ln))
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }
    return makeI(1, rs, rt, imm);
  }

  // I rs, imm
  else if (I_t.find(name) != I_t.end()) {
    rt = I_t[name];
    uint16_t imm = regs[pop(instr)];
    rs = regs[pop(instr)];
    return makeI(1, rs, rt, imm);
  }

  // J-type

  // J label/#ln*4
  else {
    int addr;
    op = J[name];
    string lab_ln = pop(instr); // label or line number
    if (is_num_str(lab_ln))
      addr = stoi(lab_ln) / 4; // #ln
    else
      addr = labels[lab_ln]; // label
    return makeJ(op, addr);
  }
}

/*
 * Store info of label into labels and remove label from the string
 */
void get_label(int ln_idx, string &str, unordered_map<string, int> &labels) {
  string label;
  char curr;
  int i = 0;

  while (1) {
    curr = str[i];
    // cout << "current char: [" << curr << "]: " << int(curr) << endl;
    if (curr == '\t' or curr == ' ')
      return;
    else if (curr == ':') {
      str = str.substr(i + 1);
      labels.insert(pair<string, int>(label, ln_idx));
      return;
    } else {
      label += curr;
      i += 1;
    }
  }
}

/*
 * Strips the whitespaces and comments off a line of instruction
 */
void shrink(string &str) {
  // find beginning of instruction
  int beg = str.find_first_not_of(" \t");

  // find start of comment
  int comm = str.find('#', beg);

  // find end of instruction
  int end = str.find_last_not_of(" \t", comm - 1);
  int n = end - beg + 1;

  str = str.substr(beg, n);

  return;
}

int main() {

  // First scanning: read labels, process and store instructions
  while (getline(f, curr_ln)) {
    if (is_empty_str(curr_ln))
      continue;

    get_label(ln_idx, curr_ln, labels); // get label info and strip the labels
    shrink(curr_ln); // strip ws and comments to obtain the raw instructions
    instructions.push_back(curr_ln);
    ln_idx += 1;
  }

  // Second scanning: reading instructions
  for (ln_idx = 0; ln_idx < int(instructions.size()); ln_idx++) {
    mach_code = make(instructions[ln_idx], labels);
    bitset<32> x(mach_code);
    o << x.to_string() << endl;
  }

  f.close();
  o.close();
  cout << "Successfully assembled!" << endl
       << "File path: " << out_path << endl;
}
