#include <QCoreApplication>
#include <iostream>
#include <string>
#include <bitset>
#include <fstream>
#include "mips.h"
using namespace std;

int ln_idx = 0;
int mach_code, imm;
uint8_t op, rd, rs, rt, shamt, func;
ifstream f; ofstream o;
string curr_ln, in_path, out_path;
unordered_map<string, int> labels;
vector<string> instructions;

const string TITLE = "MIPS ASSEMBLER for CSC3050 Assignment 1";
const string IN_PROMPT = "Please specify the absolute path of input file (e.g. /usr/local/input.asm):";
const string QUESTION = "Save output file where input file is? [Y/N]";
const string OUT_PROMPT = "Please specify the absolute path of output file (e.g. /usr/local/output.txt):";
const string NUMS = "0123456789";
const string WS = " \t";
const unordered_map<string, uint8_t> REGS = {
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

const unordered_map<string, uint8_t> R_DST = {
    // instr rd, rs, rt
    {"add", 0b100000}, {"addu", 0b100001}, {"and", 0b100100},
    {"mul", 0b000010}, {"nor", 0b100111},  {"or", 0b100101},
    {"sub", 0b100010}, {"subu", 0b100011}, {"xor", 0b100110},
    {"slt", 0b101010}, {"sltu", 0b101011},
};

const unordered_map<string, uint8_t> R_SLV = {
    // instr rd, rt, rs
    {"sllv", 0b000100},
    {"srav", 0b000111},
    {"srlv", 0b000110},
};

const unordered_map<string, uint8_t> R_DTS = {
    // instr rd, rt, shamt
    {"sll", 0b000000},
    {"sra", 0b000011},
    {"srl", 0b000010},
};

const unordered_map<string, uint8_t> R_DS = {
    // instr rd, rs
    {"clo", 0x21},
    {"clz", 0x20}};

const unordered_map<string, uint8_t> R_ST = {
    // instr rs, rt
    {"div", 0b011010},   {"divu", 0b011011}, {"mult", 0b011000},
    {"multu", 0b011001}, {"madd", 0},        {"maddu", 1},
    {"msub", 4},         {"msubu", 5},       {"teq", 0x34},
    {"tne", 0x36},       {"tge", 0x30},      {"tgeu", 0x31},
    {"tlt", 0x32},       {"tltu", 0x33},
};

const unordered_map<string, uint8_t> R_D = {
    // instr rd
    {"mfhi", 0b010000},
    {"mflo", 0b010010}};

const unordered_map<string, uint8_t> R_S = {
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

const unordered_map<string, uint8_t> I_TSI = {
    // instr rt, rs, imm
    // {"instr", op}
    {"addi", 0b001000},  {"addiu", 0b001001}, {"andi", 0b001100},
    {"ori", 0b001101},   {"xori", 0b001110},  {"slti", 0b001010},
    {"sltiu", 0b001011},
};

const unordered_map<string, uint8_t> I_LS = {
    // load and save
    // instr rt, imm(rs)
    // format: {"instr", op}
    {"lb", 0b100000}, {"lbu", 0b100100},  {"lh", 0b100001}, {"lhu", 0b100101},
    {"lw", 0b100011}, {"lwc1", 0b110001}, {"lwl", 0x22},    {"lwr", 0x26},
    {"ll", 0x30},     {"sb", 0b101000},   {"sh", 0b101001}, {"sw", 0b101011},
    {"swl", 0x2a},    {"swr", 0x2e},      {"sc", 0x38}};

const unordered_map<string, uint8_t> I_B_STL = {
    // instr rs, rt, label
    // {"branch", op}
    {"beq", 0b000100},
    {"bne", 0b000101}};

const unordered_map<string, uint8_t> I_B_SL = {
    // instr rs, label
    // {"branch", op}
    {"bgtz", 0b000111},
    {"blez", 0b000110}};

const unordered_map<string, uint8_t> I_B1 = {
    // instr rs, label
    // opcode = 1
    // {"branch", rt}
    {"bltz", 0b00000},
    {"bgez", 0b00001},
    {"bltzal", 0b10000},
    {"bgezal", 0b10001}};

const unordered_map<string, uint8_t> I_T = {
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

const unordered_map<string, uint8_t> J = {{"j", 2}, {"jal", 3}};

string pop(stack<string> &s) {
  string x = s.top();
  s.pop();
  return x;
}

/*
 * Function: makeR
 * Usage: int R = makeR("add $s1, $s2, $s3")
 * ----------------------------------
 * Return the machine code of an R-type instruction.
 */
int makeR(uint8_t op, uint8_t func, uint8_t rs, uint8_t rt, uint8_t rd,
          uint8_t shamt) {
  int R = op << 26;
  R |= rs << 21;
  R |= rt << 16;
  R |= rd << 11;
  R |= shamt << 6;
  R |= func;
  return R;
}

/*
 * Function: makeI
 * Usage: int I = makeI("addi $s1, $s2, 100")
 * ----------------------------------
 * Return the machine code of an I-type instruction.
 */
int makeI(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm) {
  int I = op << 26;
  I |= rs << 21;
  I |= rt << 16;
  I |= imm;
  return I;
}

/*
 * Function: makeJ
 * Usage: int j = makeJ("j 100")
 * ----------------------------------
 * Return the machine code of a J-type instruction.
 */
int makeJ(uint8_t op, int ln_num) {
  int J = op << 26;
  J |= ln_num;
  return J;
}

/*
 * Function: break_instr
 * Usage: stack<string> instr = break_instr("ll  $v0, 	 24($v0)");
 *        // instr == (top) ["ll", "$24($v0)", "$v0"] (bottom)
 * ----------------------------------
 * Break a single-line string instruction into a corresponding stack,
 * with the name of the instruction on top of the stack.
 */
stack<string> break_instr(string instr) {
  string arg = "";
  string name;
  bool found_name = 0;
  stack<string> args;

  for (int i = 0; i < int(instr.length()); i++) {
    char curr = instr[i];

    if (!found_name)
      switch (curr) {
      case ' ':
      case '\t':
        found_name = 1;
        name = arg;
        arg = "";
        break;
      default:
        arg += curr;
      }

    else {
      switch (curr) {
      case ' ':
      case '\t':
        break;
      case ',':
        args.push(arg);
        arg = "";
        break;
      default:
        arg += curr;
      }
    }
  }
  args.push(arg);
  args.push(name);
  return args;
}

/*
 * Function: make
 * Usage: int m_code = make("j 100");
 * ----------------------------------
 * Returns the translated machine code of a
 * one-line instruction (already shrunk.)
 */
int make(string instruction) {

  // finding name of the instruction
  stack<string> instr = break_instr(instruction);
  string name = pop(instr);

  // R-type -------------------------------------

  // jalr rs(, rd = 31)
  if (name == "jalr") {
    if (instr.size() == 1)
      rd = 31; // default rd = 31
    else
      rd = REGS.at(pop(instr)); // user input rd

    rs = REGS.at(pop(instr));

    return makeR(0, 9, rs, 0, rd);
  }

  // R rd, rs, rt
  else if (R_DST.find(name) != R_DST.end()) {
    func = R_DST.at(name);
    rt = REGS.at(pop(instr));
    rs = REGS.at(pop(instr));
    rd = REGS.at(pop(instr));

    if (func == 2)
      op = 0x1c; // mul
    else
      op = 0;

    return makeR(op, func, rs, rt, rd);
  }

  // R rd, rt, rs
  else if (R_SLV.find(name) != R_SLV.end()) {
    func = R_SLV.at(name);
    rs = REGS.at(pop(instr));
    rt = REGS.at(pop(instr));
    rd = REGS.at(pop(instr));

    return makeR(0, func, rs, rt, rd);
  }

  // R rd, rt, shamt
  else if (R_DTS.find(name) != R_DTS.end()) {
    func = R_DTS.at(name);
    shamt = stoi(pop(instr));
    rt = REGS.at(pop(instr));
    rd = REGS.at(pop(instr));

    return makeR(0, func, 0, rt, rd, shamt);
  }

  // R rd, rs
  else if (R_DS.find(name) != R_DS.end()) {
    func = R_DS.at(name);
    rs = REGS.at(pop(instr));
    rd = REGS.at(pop(instr));

    return makeR(0x1c, func, rs, 0, rd);
  }

  // R rs, rt
  else if (R_ST.find(name) != R_ST.end()) {
    func = R_ST.at(name);
    rt = REGS.at(pop(instr));
    rs = REGS.at(pop(instr));

    if (func == 0 || func == 1 || func == 4 ||
        func == 5) // "madd", "maddu", "msub", "msubu"
      op = 0x1c;
    else
      op = 0;

    return makeR(op, func, rs, rt, 0);
  }

  // R rd
  else if (R_D.find(name) != R_D.end()) {
    func = R_D.at(name);
    rd = REGS.at(pop(instr));

    return makeR(0, func, 0, 0, rd);
  }

  // R rs
  else if (R_S.find(name) != R_S.end()) {
    func = R_S.at(name);
    rs = REGS.at(pop(instr));

    return makeR(0, func, rs, 0, 0);
  }

  // I-type -------------------------------------

  // lui rt, imm
  else if (name == "lui") {
    imm = stoi(pop(instr));
    rt = REGS.at(pop(instr));

    return makeI(15, 0, rt, imm);
  }

  // I rt, rs, imm
  else if (I_TSI.find(name) != I_TSI.end()) {
    imm = stoi(pop(instr));
    op = I_TSI.at(name);
    rs = REGS.at(pop(instr));
    rt = REGS.at(pop(instr));
    return makeI(op, rs, rt, imm);
  }

  // I rt, imm(rs)
  else if (I_LS.find(name) != I_LS.end()) {
    int c = 0;
    int i = 0;
    string imm_str, rs_str = "";
    string imm_rs = pop(instr); // imm_rs == "imm(rs)"
    rt = REGS.at(pop(instr));
    op = I_LS.at(name);

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
    rs = REGS.at(rs_str);

    return makeI(op, rs, rt, imm);
  }

  // I rs, rt, label/#ln
  else if (I_B_STL.find(name) != I_B_STL.end()) {
    op = I_B_STL.at(name);

    string lab_ln = pop(instr);
    rt = REGS.at(pop(instr));
    rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }

    return makeI(op, rs, rt, imm);
  }

  // I rs, label/#ln (rt = 0)
  else if (I_B_SL.find(name) != I_B_SL.end()) {
    op = I_B_SL.at(name);
    string lab_ln = pop(instr);
    rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }

    return makeI(op, rs, 0, imm);
  }

  // I rs, label/#ln
  else if (I_B1.find(name) != I_B1.end()) {
    rt = I_B1.at(name);
    string lab_ln = pop(instr);
    rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
    }
    return makeI(1, rs, rt, imm);
  }

  // I rs, imm
  else if (I_T.find(name) != I_T.end()) {
    rt = I_T.at(name);
    uint16_t imm = stoi(pop(instr));
    rs = REGS.at(pop(instr));
    return makeI(1, rs, rt, imm);
  }

  // J-type -------------------------------------

  // J label/#ln*4
  else {
    int addr;
    op = J.at(name);
    string lab_ln = pop(instr); // label or line number
    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      addr = stoi(lab_ln) / 4; // #ln
    else
      addr = labels[lab_ln]; // label
    return makeJ(op, addr);
  }
}

/*
 * Function: get_label
 * Usage: get_label("\tJ:   j 100  ", my_labels);
 *      // "j 100   "
 * ----------------------------------
 * Store info of label (if any) into a hashmap,
 * remove the label from the instruction.
 */
void get_label(string &str, unordered_map<string, int> &labs) {
  string label;
  auto colon = str.find(':');
  if (colon != str.npos) {
    label = str.substr(0, colon);

    labs.insert(pair<string, int>(label, ln_idx));
    str = str.substr(colon + 1);
  }
}

/*
 * Function: no_comment
 * Usage: strip("abc # comments");
 * // "abc "
 * ----------------------------------
 * Deletes the comments starting with '#' from a string.
 */
void no_comment(string &str) {
  auto comm = str.find('#');
  if (comm != str.npos) // found comment
  {
    str = str.substr(0, comm);
  }
}

/*
 * Function: strip
 * Usage: strip("\t   abc, 123 ");
 * // "abc, 123"
 * ----------------------------------
 * Strips the whitespaces on the both ends of a string.
 */
void strip(string &str) {
  auto start = str.find_first_not_of(WS);
  if (start == str.npos)
    str = "";
  else {
    auto end = str.find_last_not_of(WS);
    str = str.substr(start, end - start + 1);
  }
}

/*
 * Function: get_stream
 * Usage: get_stream(my_ifstream, my_ofstream);
 * ----------------------------------
 * Prompts user for file path, set i/o fstreams ready for scanning
 */
void get_stream(ifstream &is, ofstream &os){
    char ans;
    cout << TITLE << "\n\n" << IN_PROMPT;
    getline(cin, in_path);

    while (1) {
    cout << QUESTION;
    ans = toupper(cin.get());

    if (ans == 'Y') {
        auto slash = in_path.find_last_of('/');
        out_path = in_path.substr(0, slash + 1) + "output.txt";
        break;
    }
    else if (ans == 'N') {
        cout << OUT_PROMPT;
        getline(cin, out_path);
        break;
    }
    }
    is.open(in_path);
    os.open(out_path);
}

/*
 * Function: scan
 * Usage: scan(ifstream, instr);
 * ----------------------------------
 * Scans instructions through ifstream,
 * stores them into vector instr.
 */
void scan(ifstream &is, vector<string> &instr){
    // First scanning: read labels and store instructions
    while (getline(is, curr_ln)) {
      no_comment(curr_ln);
      if (curr_ln.find_first_not_of(WS) == curr_ln.npos)
        continue;

      get_label(curr_ln, labels);  // gets label info and delete the labels
      strip(curr_ln);      // strips WS to obtain the raw instructions
      if (curr_ln != "") { // skips label line
        instr.push_back(curr_ln);
        ln_idx += 1;
      }
    }
    is.close();
}

/*
 * Function: read
 * Usage: scan(instr, ofstream);
 * ----------------------------------
 * Reads instructions from vector instr in to machine codes,
 * stores those codes through ofstream
 */
void read(vector<string> &instr, ofstream &os)
{
    // Second scanning: reading instructions
    for (ln_idx = 0; ln_idx < int(instr.size()); ln_idx++) {
      mach_code = make(instr[ln_idx]);
      bitset<32> x(mach_code);
      os << x.to_string() << endl;
    }
    cout << "Successfully assembled!" << endl
         << "File path: " << out_path << endl;
    os.close();
}

int main() {
  get_stream(f, o);
  scan(f, instructions);
  read(instructions, o);
}
