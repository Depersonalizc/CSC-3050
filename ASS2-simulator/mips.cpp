// regs save fake 32-bit address!!!!!

#include <QCoreApplication>
#include <iostream>
#include <string>
#include <sstream>
#include <cstdlib>
#include <iomanip>
#include <limits.h>
#include <cmath>
#include "mips.h"
using namespace std;

uint32_t curr_code;
uint64_t ln_idx = 0;
ifstream f;
unordered_map<string, uint64_t> labels;

const string TITLE = "\
        __  __ _____ _____   _____ _            \n\
       |  \\/  |_   _|  __ \\ / ____(_)           \n\
       | \\  / | | | | |__) | (___  _ _ __ ___   \n\
       | |\\/| | | | |  ___/ \\___ \\| | '_ ` _ \\  \n\
       | |  | |_| |_| |     ____) | | | | | | |  © Jamie Chen\n\
       |_|  |_|_____|_|    |_____/|_|_| |_| |_|  for CSC3050 Assignment 2";
const string IN_PROMPT =
    "Please enter the absolute path of the input file (e.g. "
    "/usr/local/input.asm):";
const string NUMS = "0123456789";
const string WS = " \t";
const vector<string> REG_LIT = {"$ze", "$at", "$v0", "$v1", "$a0", "$a1", "$a2",
                                "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5",
                                "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4",
                                "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1",
                                "$gp", "$sp", "$fp", "$ra", "$hi", "$lo"};
const unordered_map<string, uint8_t> REGS = {
    {"$zero", 0}, {"$at", 1},  {"$v0", 2},  {"$v1", 3},  {"$a0", 4},
    {"$a1", 5},   {"$a2", 6},  {"$a3", 7},  {"$t0", 8},  {"$t1", 9},
    {"$t2", 10},  {"$t3", 11}, {"$t4", 12}, {"$t5", 13}, {"$t6", 14},
    {"$t7", 15},  {"$s0", 16}, {"$s1", 17}, {"$s2", 18}, {"$s3", 19},
    {"$s4", 20},  {"$s5", 21}, {"$s6", 22}, {"$s7", 23}, {"$t8", 24},
    {"$t9", 25},  {"$k0", 26}, {"$k1", 27}, {"$gp", 28}, {"$sp", 29},
    {"$fp", 30},  {"$ra", 31}, {"$hi", 32}, {"$lo", 33}};

/*
 *  ========================= R-type funcode =========================
 *  opcode = 0
 *  ------------------------------------------------------------------
 *  op       rs      rt      rd      shamt   funct
 *  000000   5       5       5       5       6
 *  ------------------------------------------------------------------
 */

const unordered_map<string, uint8_t> R_DST = {
    // instr rd, rs, rt
    {"add", 0b100000},  {"addu", 0b100001}, {"and", 0b100100},
    {"nor", 0b100111},  {"or", 0b100101},   {"sub", 0b100010},
    {"subu", 0b100011}, {"xor", 0b100110},  {"slt", 0b101010},
    {"sltu", 0b101011},
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

const unordered_map<string, uint8_t> R_ST = {
    // instr rs, rt
    {"div", 0b011010},   {"divu", 0b011011}, {"mult", 0b011000},
    {"multu", 0b011001}, {"teq", 0x34},      {"tne", 0x36},
    {"tge", 0x30},       {"tgeu", 0x31},     {"tlt", 0x32},
    {"tltu", 0x33},
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
    // {"instr", op}
    {"lb", 0b100000}, {"lbu", 0b100100}, {"lh", 0b100001}, {"lhu", 0b100101},
    {"lw", 0b100011}, {"lwl", 0x22},     {"lwr", 0x26},    {"ll", 0x30},
    {"sb", 0b101000}, {"sh", 0b101001},  {"sw", 0b101011}, {"swl", 0x2a},
    {"swr", 0x2e},    {"sc", 0x38}};

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

const uint32_t SIM_SIZE = 0x600000;      // size of simulation (6 MB)
const uint32_t TEXT_SIZE = 0x100000;     // size of text section (1 MB)
const uint32_t RESERVED_SIZE = 0x400000;  // size of reserved memory
const uint32_t REG_DEFAULT = 0;

uint32_t* REG = (uint32_t*)malloc(SIM_SIZE);          // pointer at start of simulated registers
char* BASE = (char*)REG + REGS.size() * sizeof(int);  // base pointer, start of text section
char* DATA = BASE + TEXT_SIZE;                            // data pointer, start of data section
char* STACK = DATA + (SIM_SIZE - TEXT_SIZE);              // stack pointer, at highest memory addr
uint32_t* PC = (uint32_t*)BASE;                           // program counter

// print formatted title
void print_title(string s, int len, char fill) {
  int fillw,
      halfLen = len / 2,
      halfSlen = s.length() / 2;
  fillw = halfLen - halfSlen;
  cout << setw(s.length() + fillw) << right << setfill(fill) << s;
  cout << setw(fillw - 1) << setfill(fill) << fill << endl;
}

// map real address to fake 32-bit
uint32_t addr_32(uint64_t true_addr, uint64_t base = (uintptr_t)BASE,
                 uint32_t reserved = RESERVED_SIZE) {
  return true_addr - base + reserved;
}

// fake 32-bit mapped back to true address
uint64_t addr_64(uint32_t fake, uint64_t base = (uintptr_t)BASE,
                 uint32_t reserved = RESERVED_SIZE) {
  return (fake + base - reserved);
}

// returns last-n-bits masking
uint32_t last(int n) { return (1 << n) - 1; }

// returns the VALUE in i-th register, default Type=int
template <class T = int>
T reg_val(int idx, uint32_t* pt = REG) {
  if (idx < 32) {
    uint64_t true_addr = addr_64(pt[idx]);
    return *(T*)true_addr;
  } else {
    return *(T*)(pt + idx);
  }
}

// compute the size of a data in blocks of 4-bytes
size_t blockwise(size_t size)
{
    return ((size - 1)/4 + 1) * 4;
}

// display bytes in memory
void print_bytes(int num_of_bytes, char* ptr = BASE) {
  print_title("MEMORY");
  for (int i = 0; i < num_of_bytes; i++) {
    if (i % 32 == 0) {
      uint32_t x = addr_32((intptr_t)ptr);
      cout << "0x" << hex << (x + i) << "\t\t";
    }
    printf("%02hhX ", ptr[i]);
    if (i % 4 == 3) cout << '\t';
    if (i % 32 == 31) cout << '\n';
  }
  cout << '\n';
  print_title();
}

// store content using a pointer, increment pointer block-wise (size = 4 bytes).
// return a new pointer of the corresponding type to the saved content.
template <class T = int> // store a single value
T* store_at_ptr(char*& p, T content = (T)0) {
  T* pt = (T*)p;
  *pt = content;
  p += blockwise(sizeof (T)); // 1,2,3,4 -> 4; 5,6,7,8 -> 8;...
  return pt;
}

// store an array given a vector
template <class T>
T* store_at_ptr(char*& p, vector<T> content){

  T *pt = (T*)p,
    *saving_p = pt;
  size_t size = sizeof (T) * content.size();

  for (T element: content){
      *saving_p = element; ++saving_p;
  }
    p += blockwise(size);
    return pt;
}

// store a string at pointer, from given vector<char>
char* store_string_at_ptr(char*& p, vector<char> content, bool terminator){
  char *pt = p,
       *saving_p = p;
  size_t size = sizeof (char) * (content.size() + terminator);

  for (char c: content){
      *saving_p = c; ++saving_p;
  }
    if (terminator) *saving_p = '\0';
    p += blockwise(size);
    return pt;
}

// initialize register saved address
void init_reg(uint32_t* pt = REG) {
  for (size_t i = 0; i < 32; i++) {
    pt[i] = REG_DEFAULT;
  }
  pt[28] = addr_32((intptr_t)DATA); // $gp
  pt[29] = addr_32((intptr_t)STACK); // $sp
  pt[30] = pt[29]; // $fp
}

/*
 * Function: pop
 * Usage: pop<T>(my_stack)
 * ----------------------------------
 * Pops out the first element in my_stack,
 * returns a reference to that element.
 */
template <class T = string>
T pop(stack<T>& s) {
  T x = s.top();
  s.pop();
  return x;
}

/*
 * Function: makeR
 * Usage: int R = makeR("add $s1, $s2, $s3")
 * ----------------------------------
 * Return the machine code of an R-type instruction.
 */
uint32_t makeR(uint8_t op, uint8_t func, uint8_t rs, uint8_t rt, uint8_t rd,
               uint8_t shamt) {
  uint32_t R = op << 26;
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
uint32_t makeI(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm) {
  uint32_t I = op << 26;
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
uint32_t makeJ(uint8_t op, uint32_t ln_num) {
  uint32_t J = op << 26;
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

  if (instr == "syscall")
    args.push(instr);
  else {
    for (int i = 0; i < int(instr.length()); i++) {
      char curr = instr[i];

      if (!found_name) switch (curr) {
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
  }

  return args;
}

// break a data declaration instruction into name, type, and content,
// store them into the input arguments
void break_data(string data, string& name, string& type, string& content) {
    int colon, type_beg, type_end, cont_beg;
    colon = data.find(':');
    name = data.substr(0,colon);
    type_beg = data.find_first_not_of(WS,colon+1);
    type_end = data.find_first_of(WS, type_beg+1) - 1;
    type = data.substr(type_beg, type_end - type_beg + 1);
    cont_beg = data.find_first_not_of(WS, type_end + 1);
    content = data.substr(cont_beg);
}

/*
 * Function: make
 * Usage: int m_code = make("j 100");
 * ----------------------------------
 * Returns the translated machine code of a
 * one-line instruction (already shrunk.)
 */
uint32_t make(string instruction) {
  // finding name of the instruction
  stack<string> instr = break_instr(instruction);
  string name = pop(instr);

  if (name == "syscall") return 0xc;

  // R-type -------------------------------------
  // jalr rs(, rd = 31)
  else if (name == "jalr") {
    uint8_t rd;
    switch (instr.size()) {
      case 1:
        rd = 31;
        break;  // default rd = 31
      default:
        rd = REGS.at(pop(instr));  // user input rd
    }
    uint8_t rs = REGS.at(pop(instr));
    return makeR(0, 9, rs, 0, rd);
  }

  // R rd, rs, rt
  else if (R_DST.find(name) != R_DST.end()) {
    uint8_t func = R_DST.at(name);
    uint8_t rt = REGS.at(pop(instr));
    uint8_t rs = REGS.at(pop(instr));
    uint8_t rd = REGS.at(pop(instr));
    return makeR(0, func, rs, rt, rd);
  }

  // R rd, rt, rs
  else if (R_SLV.find(name) != R_SLV.end()) {
    uint8_t func = R_SLV.at(name);
    uint8_t rs = REGS.at(pop(instr));
    uint8_t rt = REGS.at(pop(instr));
    uint8_t rd = REGS.at(pop(instr));
    return makeR(0, func, rs, rt, rd);
  }

  // R rd, rt, shamt
  else if (R_DTS.find(name) != R_DTS.end()) {
    uint8_t func = R_DTS.at(name);
    uint8_t shamt = stoi(pop(instr));
    uint8_t rt = REGS.at(pop(instr));
    uint8_t rd = REGS.at(pop(instr));
    return makeR(0, func, 0, rt, rd, shamt);
  }

  // R rs, rt
  else if (R_ST.find(name) != R_ST.end()) {
    uint8_t func = R_ST.at(name);
    uint8_t rt = REGS.at(pop(instr));
    uint8_t rs = REGS.at(pop(instr));
    return makeR(0, func, rs, rt, 0);
  }

  // R rd
  else if (R_D.find(name) != R_D.end()) {
    uint8_t func = R_D.at(name);
    uint8_t rd = REGS.at(pop(instr));
    return makeR(0, func, 0, 0, rd);
  }

  // R rs
  else if (R_S.find(name) != R_S.end()) {
    uint8_t func = R_S.at(name);
    uint8_t rs = REGS.at(pop(instr));
    return makeR(0, func, rs, 0, 0);
  }

  // I-type -------------------------------------

  // lui rt, imm
  else if (name == "lui") {
    uint16_t imm = stoi(pop(instr));
    uint8_t rt = REGS.at(pop(instr));
    return makeI(15, 0, rt, imm);
  }

  // I rt, rs, imm
  else if (I_TSI.find(name) != I_TSI.end()) {
    uint16_t imm = stoi(pop(instr));
    uint8_t op = I_TSI.at(name);
    uint8_t rs = REGS.at(pop(instr));
    uint8_t rt = REGS.at(pop(instr));
    return makeI(op, rs, rt, imm);
  }

  // I rt, imm(rs)
  else if (I_LS.find(name) != I_LS.end()) {
    string imm_str, rs_str;
    string imm_rs = pop(instr);  // imm_rs == "imm($rs)"
    uint8_t rt = REGS.at(pop(instr));
    uint8_t op = I_LS.at(name);

    size_t left = imm_rs.find('(');
    size_t right = imm_rs.find(')', left + 4);

    imm_str = imm_rs.substr(0, left);
    rs_str = imm_rs.substr(left + 1, right - left - 1);
    uint16_t imm = stoi(imm_str);
    uint8_t rs = REGS.at(rs_str);
    return makeI(op, rs, rt, imm);
  }

  // I rs, rt, label/#ln
  else if (I_B_STL.find(name) != I_B_STL.end()) {
    uint16_t imm;
    uint8_t op = I_B_STL.at(name);
    string lab_ln = pop(instr);
    uint8_t rt = REGS.at(pop(instr));
    uint8_t rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln);  // direct branch line number
    else {
      uint64_t label = labels[lab_ln];
      imm = label - ln_idx - 1;  // branch to label
    }

    return makeI(op, rs, rt, imm);
  }

  // I rs, label/#ln (rt = 0)
  else if (I_B_SL.find(name) != I_B_SL.end()) {
    uint16_t imm;
    uint8_t op = I_B_SL.at(name);
    string lab_ln = pop(instr);
    uint8_t rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln);  // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1;  // label
    }

    return makeI(op, rs, 0, imm);
  }

  // I rs, label/#ln
  else if (I_B1.find(name) != I_B1.end()) {
    uint16_t imm;
    uint8_t rt = I_B1.at(name);
    string lab_ln = pop(instr);
    uint8_t rs = REGS.at(pop(instr));

    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      imm = stoi(lab_ln);  // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1;  // label
    }

    return makeI(1, rs, rt, imm);
  }

  // I rs, imm
  else if (I_T.find(name) != I_T.end()) {
    uint8_t rt = I_T.at(name);
    uint16_t imm = stoi(pop(instr));
    uint8_t rs = REGS.at(pop(instr));
    return makeI(1, rs, rt, imm);
  }

  // J-type -------------------------------------

  // J label/#ln*4
  else {
    uint32_t ln_num;
    uint8_t op = J.at(name);
    string lab_ln = pop(instr);  // label or line number
    if (lab_ln.find_first_not_of(NUMS) == lab_ln.npos)
      ln_num = stoi(lab_ln) / 4;  // #ln
    else
      ln_num = labels[lab_ln];  // label
    return makeJ(op, ln_num);
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
void get_label(string& str, unordered_map<string, uint64_t>& labs) {
  string label;
  size_t colon = str.find(':');
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
void no_comment(string& str) {
  size_t comm = str.find('#');
  if (comm != str.npos)  // found comment
    str = str.substr(0, comm);
}

/*
 * Function: strip
 * Usage: strip("\t   abc, 123 ");
 * // "abc, 123"
 * ----------------------------------
 * Strips the whitespaces on the both ends of a string.
 */
void strip(string& str) {
  size_t start = str.find_first_not_of(WS);
  if (start == str.npos)
    str = "";
  else {
    size_t end = str.find_last_not_of(WS);
    str = str.substr(start, end - start + 1);
  }
}

/*
 * Function: get_stream
 * Usage: get_stream(my_ifstream, my_ofstream);
 * ----------------------------------
 * Prompts user for file path, set i/o fstreams ready for scanning
 */
void get_stream(ifstream& is) {
  string in_path;
  cout << TITLE << "\n\n";
  while (1) {
    cout << IN_PROMPT << endl;

    //getline(cin, in_path);
    in_path = "c:/users/chen1/desktop/fib.asm";

    is.open(in_path);
    if (is.fail()) {
      cout << "File not found! Please try again.\n";
      continue;
    }
    break;
  }
}

/*
 * Function: scan
 * Usage: scan(ifstream, instr);
 * ----------------------------------
 * Scans instructions through ifstream,
 * stores them into vector instr.
 */
void scan(ifstream& is, vector<string>& instr, vector<string>& data) {
  string curr_ln;
  bool is_data = 1;
  // First scanning: read labels and store instructions
  while (getline(is, curr_ln)) {
    no_comment(curr_ln);
    if (curr_ln.find_first_not_of(WS) == curr_ln.npos) continue;
    if (curr_ln.find(".data") != curr_ln.npos) continue;
    if (curr_ln.find(".text") != curr_ln.npos) {is_data = 0; continue;}

    if (is_data) // .data
    {
        strip(curr_ln);
        data.push_back(curr_ln);
    }

    else // .text
    {
        get_label(curr_ln, labels);  // gets label info and delete the labels
        strip(curr_ln);              // strips WS to obtain the raw instructions
        if (curr_ln != "") {         // skips label line
          instr.push_back(curr_ln);
          ++ln_idx;
    }
  }
}
    is.close();
}

// set the value (memory pointed by the address) of a register
template <class T = int>
void set_reg_val(int idx, T val, uint32_t* pt = REG) {
  *(T*)addr_64(pt[idx]) = val;
}

// display the contents (addresses) stored in a register
void print_reg_saved_addr(int idx, uint32_t*& ptr = REG) {
  cout << REG_LIT[idx] << ": 0x" << hex << ptr[idx];
}

// display the contents (addresses) stored in all registers
void print_regs_saved_addr(uint32_t*& ptr = REG) {
  print_title("REGISTERS");
  for (size_t i = 0; i < REGS.size(); i++) {
    print_reg_saved_addr(i, ptr);
    cout << '\t';
    if (i % 9 == 8) cout << '\n';
  }
  cout << '\n';
  print_title();
}

void link(uint32_t* pt = REG) {
  pt[31] = addr_32((intptr_t)(PC + 1));
}

// convert a array declaration string into a vector
template<class T>
vector<T> parse_array(string str)
{
   int element;
   T elem;
   vector<T> vec;
   stringstream ss(str);
   bool flag = 0; // char literal

   while (1)
  {
      auto p = ss.peek();
      if (p == ',' || p == ' ' || p == '\t') ss.ignore();
      else {
          if (p == '\''){
              flag = 1 - flag;
              ss.ignore();
          }
          else{
              if (flag){
                  if (ss >> elem) vec.push_back(elem);
                  else break;
              }
              else {
                  if (ss >> element) vec.push_back((T)element);
                  else break;
              }
          }
      }
}
   return vec;}

// write static data and machine codes
void write_data_and_text(char* p = BASE) {
  uint32_t mach_code;
  uint32_t* text_p = (uint32_t*)p;
  string name, type, content;
  vector<string> instructions, data;

  get_stream(f);
  scan(f, instructions, data);

  // write static data
  for (string dat: data){
      break_data(dat, name, type, content);

      if (type == ".ascii"){
          content = content.substr(1, content.length()-2);
          vector<char> cvect(content.begin(), content.end());
          store_string_at_ptr(DATA, cvect, 0); // without terminator
      }

      else if (type == ".asciiz"){
          content = content.substr(1, content.length()-2);
          vector<char> cvect(content.begin(), content.end());
          store_string_at_ptr(DATA, cvect, 1); // with null terminator
      }

      else if (type == ".byte"){
          store_at_ptr<char>(DATA, parse_array<char>(content));
      }

      else if (type == ".half"){
          store_at_ptr<int16_t>(DATA, parse_array<int16_t>(content));
      }

      else  // ".word"
          store_at_ptr<int32_t>(DATA, parse_array<int32_t>(content));

      //print_bytes(10, (char*)addr_64(0x00500134));
  }

  // loop thru all instructions, store assembled machine codes
  for (ln_idx = 0; ln_idx < instructions.size(); ln_idx++) {
      mach_code = make(instructions[ln_idx]);
      text_p[ln_idx] = mach_code;
  }
}

void throw_trap(string info = "trapped")
{
    cout << '\n' << info << hex << " @ 0x" << addr_32((intptr_t)PC) << '\n';
    throw 0;
}

bool add_overflow(int a, int b) {
  return ((a > 0) && (b > INT_MAX - a)) || ((a < 0) && (b < INT_MIN - a));
}

void addu(int rs, int rt, int rd, int32_t* pt = (int32_t*)REG) {
  pt[rd] = pt[rs] + pt[rt];
}

void add(int rs, int rt, int rd, int32_t* pt = (int32_t*)REG) {
  if (add_overflow(pt[rs], pt[rt])) throw_trap();
  addu(rs, rt, rd, pt);
}

void addiu(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  pt[rt] = pt[rs] + imm;
}

void addi(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (add_overflow(pt[rs], imm)) throw_trap();
  addiu(rs, rt, imm, pt);
}

void AND(int rs, int rt, int rd, uint32_t* pt = REG) {
  pt[rd] = pt[rs] & pt[rt];
}

void andi(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
  pt[rt] = pt[rs] & imm;
}

void OR(int rs, int rt, int rd, uint32_t* pt = REG) {
    pt[rd] = pt[rs] | pt[rt];
}

void ori(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    pt[rt] = pt[rs] | imm;
}

void nor(int rs, int rt, int rd, uint32_t* pt = REG) {
    pt[rd] = ~(pt[rs] | pt[rt]);
}

void XOR(int rs, int rt, int rd, uint32_t* pt = REG) {
    pt[rd] = pt[rs] ^ pt[rt];
}

void xori(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    pt[rt] = pt[rs] ^ imm;
}

bool sub_overflow(int a, int b) {
  return ((b > 0) && (a < INT_MIN + b)) || ((b < 0) && (a > INT_MAX + b));
}

void subu(int rs, int rt, int rd, int32_t* pt = (int32_t*)REG) {
    pt[rd] = pt[rs] - pt[rt];
}

void sub(int rs, int rt, int rd, int32_t* pt = (int32_t*)REG) {
  if (sub_overflow(pt[rs], pt[rt])) throw_trap();
  subu(rs, rt, rd, pt);
}

void mult(int rs, int rt, int32_t* pt = (int32_t*)REG) {
  uint64_t ans = pt[rs] * pt[rt];
  int32_t high = ans >> 32;        // high-order word
  int32_t low = ans & last(32);  // low-order word
  pt[32] = high;
  pt[33] = low;
}

void multu(uint32_t rs, uint32_t rt, uint32_t* pt = REG) {
  uint64_t ans = pt[rs] * pt[rt];
  uint32_t high = ans >> 32;        // high-order word
  uint32_t low = ans & 0xFFFFFFFF;  // low-order word
  pt[32] = high;
  pt[33] = low;
}

void DIV(int rs, int rt, int32_t* pt = (int32_t*)REG) {
  int32_t quo = pt[rs] / pt[rt];
  int32_t rem = pt[rs] % pt[rt];
  pt[32] = rem;
  pt[33] = quo;
}

void divu(int rs, int rt, uint32_t* pt = REG) {
    uint32_t quo = pt[rs] / pt[rt];
    uint32_t rem = pt[rs] % pt[rt];
    pt[32] = rem;
    pt[33] = quo;
}

void mfhi(int rd, uint32_t* pt = REG) {
  pt[rd] = pt[32];
}

void mflo(int rd, uint32_t* pt = REG) {
    pt[rd] = pt[33];
}

void mthi(int rs, uint32_t* pt = REG) {   pt[32] = pt[rs]; }

void mtlo(int rs, uint32_t* pt = REG) { pt[33] = pt[rs];}

void teq(int rs, int rt, uint32_t* pt = REG) {
  if (pt[rs] == pt[rt]) throw_trap();
}

void teqi(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] == imm) throw_trap();
}

void tne(int rs, int rt, uint32_t* pt = REG) {
  if (pt[rs] != pt[rt]) throw_trap();
}

void tnei(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] != imm) throw_trap();
}

void tge(int rs, int rt, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] >= pt[rt]) throw_trap();
}

void tgeu(int rs, int rt, uint32_t* pt = REG) {
    if (pt[rs] >= pt[rt]) throw_trap();
}

void tgei(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] >= imm) throw_trap();
}

void tgeiu(int rs, uint16_t imm, uint32_t* pt = REG) {
    if (pt[rs] >= imm) throw_trap();
}

void tlt(int rs, int rt, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] < pt[rt]) throw_trap();
}

void tltu(int rs, int rt, uint32_t* pt = REG) {
    if (pt[rs] < pt[rt]) throw_trap();
}

void tlti(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] < imm) throw_trap();
}

void tltiu(int rs, uint16_t imm, uint32_t* pt = REG) {
    if (pt[rs] < imm) throw_trap();
}

void slt(int rs, int rt, int rd, int32_t* pt = (int32_t*)REG) {
  pt[rd] = pt[rs] < pt[rt];
}

void sltu(int rs, int rt, int rd, uint32_t* pt = REG) {
  pt[rd] = pt[rs] < pt[rt];
}

void slti(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  pt[rt] = pt[rs] < imm;
}

void sltiu(int rs, int rt, uint16_t imm, uint32_t* pt = REG) {
    pt[rt] = pt[rs] < imm;
}

// unconditionally jump to addr in rs
void jr(int rs, uint32_t* pt = REG) {
  // however PC += 1 each time, hence -1
  PC = (uint32_t*)addr_64(pt[rs]) - 1;
}

// unconditionally jump to addr in rs, store next addr in rd
void jalr(int rs, int rd, uint32_t* pt = REG) {
  // however PC += 1 each time, hence -1
  pt[rd] = addr_32((intptr_t)(PC + 1)); // link to rd
  uint32_t* j_addr = (uint32_t*)addr_64(pt[rs]);
  PC = j_addr - 1;
}

void j(uint32_t ln_num) {
  uint32_t curr_ln_num = ((intptr_t)PC - (intptr_t)BASE) / 4;
  int diff = ln_num - curr_ln_num;
  PC += diff - 1;  // PC += 1 each time, hence -1
}

void jal(uint32_t ln_num, uint32_t* pt = REG) {
  link(pt);
  uint32_t curr_ln_num = ((intptr_t)PC - (intptr_t)BASE) / 4;
  int diff = ln_num - curr_ln_num;
  PC += diff - 1;  // PC += 1 each time, hence -1
}

void sll(int rt, int rd, uint8_t shamt, uint32_t* pt = REG) {
  pt[rd] = pt[rt] << shamt;
}

void sllv(int rs, int rt, int rd, uint32_t* pt = REG) {
  pt[rd] = pt[rt] << pt[rs];
}

void srl(int rt, int rd, uint8_t shamt, uint32_t* pt = REG) {
  pt[rd] = pt[rt] >> shamt;
}

void srlv(int rs, int rt, int rd, uint32_t* pt = REG) {
  pt[rd] = pt[rt] >> pt[rs];
}

void sra(int rt, int rd, uint8_t shamt, uint32_t* pt = REG) {
  pt[rd] = (uint32_t)((int32_t)(pt[rt]) >> shamt);
}

void srav(int rs, int rt, int rd, uint32_t* pt = REG) {
  pt[rd] = (uint32_t)((int32_t)(pt[rt]) >> pt[rs]);
}

void lb(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  char* pchar = (char*)addr_64(pt[rs]) + imm;
  pt[rt] = (int32_t)*pchar;
}

void lbu(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
  uint8_t* pchar = (uint8_t*)addr_64(pt[rs]) + imm;
  pt[rt] = (uint32_t)*pchar;
}

void lh(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  int16_t* phalfw = (int16_t*)((char*)addr_64(pt[rs]) + imm);
  pt[rt] = (int32_t)*phalfw;
}

void lhu(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
  uint16_t* phalfw = (uint16_t*)((char*)addr_64(pt[rs]) + imm);
  pt[rt] = (uint32_t)*phalfw;
}

void lw(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  int32_t* pword = (int32_t*)((char*)addr_64(pt[rs]) + imm);
  pt[rt] = *pword;
}

void lwl(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  int32_t* pword = (int32_t*)((char*)addr_64(pt[rs]) + imm);
  pt[rt] = *pword;
}

void lwr(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  int32_t* pword = (int32_t*)((char*)addr_64(pt[rs]) + imm);
  pt[rt] = *pword;
}

void ll(int rs, int rt, int16_t imm, int32_t* pt = (int32_t*)REG) {
  lw(rs, rt, imm, pt);
}

void lui(int rt, uint16_t imm, uint32_t* pt = REG) {
  pt[rt] = imm << 16;
}

void sb(int rs, int rt, uint16_t imm, uint32_t* pt = REG) {
  uint8_t byte = pt[rt] & last(8);
  uint8_t* addr = (uint8_t*)addr_64(pt[rs]) + imm;
  *addr = byte;
}

void sh(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    uint16_t half = pt[rt] & last(16);
    uint16_t* addr = (uint16_t*)(addr_64(pt[rs]) + imm);
    *addr = half;
}

void sw(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    uint32_t* addr = (uint32_t*)(addr_64(pt[rs]) + imm);
    *addr = pt[rt];
}

void swl(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    uint32_t* addr = (uint32_t*)(addr_64(pt[rs]) + imm);
    *addr = pt[rt];
}

void swr(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    uint32_t* addr = (uint32_t*)(addr_64(pt[rs]) + imm);
    *addr = pt[rt];
}

void sc(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
    sw(rs, rt, imm, pt);
}

void beq(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
  if (pt[rs] == pt[rt])
    PC += imm;  // imm already considered PC++
}

void bne(int rs, int rt, int16_t imm, uint32_t* pt = REG) {
  if (pt[rs] != pt[rt])
    PC += imm;  // imm already considered PC++
}

void bgtz(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] > 0) PC += imm;  // imm already considered PC++
}

void blez(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] <= 0) PC += imm;  // imm already considered PC++
}

void bltz(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] < 0) PC += imm;  // imm already considered PC++
}

void bltzal(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] < 0) {
    link((uint32_t*)pt);
    PC += imm;  // imm already considered PC++
  };
}

void bgez(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] >= 0) PC += imm;  // imm already considered PC++
}

void bgezal(int rs, int16_t imm, int32_t* pt = (int32_t*)REG) {
  if (pt[rs] >= 0) {
    link((uint32_t*)pt);
    PC += imm;  // imm already considered PC++
  }
}

void read_int(int32_t* pt = (int32_t*)REG) {
  string s;
  getline(cin, s);
  try {
      pt[2] = stoi(s);
  } catch (...) {
      throw_trap("syscall exception (not an int)");
  }
}

void read_string(uint32_t* pt = REG) {
  string s;
  size_t len = pt[5];  // $a1 = length, including '\n' and null byte
  char* pchar = (char*)addr_64(pt[4]); // $a0 = buffer address
  getline(cin, s);

  if (len < 1) return;
  if (s.length() > len - 1) throw_trap("syscall exception (string too long)");

  vector<char> cvect(s.begin(), s.end());
  store_at_ptr(pchar, cvect);

  if (s.length() < len - 1) {
      *pchar = '\n';
      ++pchar;
  }
  *pchar = '\0'; // terminates with a null.
}

void print_string(uint32_t* pt = REG) {
  char c;
  char* pchar = (char*)addr_64(pt[4]);

  while ((c = *pchar)) {
    cout << c;
    ++pchar;
  }
  cout << '\n';
}

void read_char(uint32_t* pt = REG) {
  char c;
  cin >> c;
  pt[2] = (uint32_t)c;
}

vector<FILE*> streams{stdout, stdin, stderr};

void open(uint32_t* pt = REG) {
    FILE* fp;
    char* path = (char*)addr_64(pt[4]);
    string mode;

    switch (pt[5]) {
    case 0: mode = "r"; break;
    case 1: mode = "w"; break;
    case 2: mode = "r+"; break;
    default: throw_trap("invalid mode");
    };

    try {
        fp = fopen(path, mode.c_str());
    } catch (...) {
        throw_trap("file to open could not be found");
    }

    streams.push_back(fp);
    pt[4] = streams.size() - 1; // save fd
}

void close(uint32_t* pt = REG) {
    uint32_t fd = pt[4]; // $a0 = fd
    FILE* fp = streams[fd];
    fclose(fp);
}

void read(uint32_t* pt = REG){
    FILE* fp;
    void* buffer = (void*)addr_64(pt[5]); // $a1 = buffer
    uint32_t fd = pt[4],  // $a0 = fd
             len = pt[6]; // $a2 = length

    fp = streams[fd];
    pt[4] = fread(buffer, sizeof(char), len, fp); // num of chars read
}

void write(uint32_t* pt = REG){
    FILE* fp;
    void* buffer = (void*)addr_64(pt[5]); // $a1 = buffer
    uint32_t fd = pt[4], // $a0 = fd
             len = pt[6];   // $a2 = length

    fp = streams[fd];
    pt[4] = fwrite(buffer, sizeof(char), len, fp); // num of chars written
}

void sbrk(uint32_t* pt = REG) {
  uint32_t addr, size;
  size = pt[4];
  addr = addr_32((intptr_t)DATA);
  pt[2] = addr;
  DATA += size;
}

void exit2(uint32_t* pt = REG) {
  cout << dec << *((int*)(pt + 4)) << endl; // print $a0
  throw_trap("exit");
}

void syscall(uint32_t* pt = REG) {
  switch (pt[2]) {  // $v0
    case 1:
      //cout << "(print_int)\n";
      cout << dec << *((int*)(pt + 4)) << '\n'; // $a0
      break;
    case 4:
      //cout << "(print_string)\n";
      print_string();
      break;
    case 5:
      //cout << "(read_int)\n";
      read_int();
      break;
    case 8:
      //cout << "(read_string)\n";
      read_string();
      break;
    case 9:
      //cout << "(sbrk)\n";
      sbrk();
      break;
    case 10:
      throw_trap("exit");
      break;
    case 11:
      //cout << "(print_char)\n" << endl;
      cout << *((char*)(pt + 4)) << endl;
      break;
    case 12:
      //cout << "(read_char)\n";
      read_char();
      break;
    case 13:
      //cout << "(open)\n";
      open();
      break;
    case 14:
      //cout << "(read)\n";
      read();
      break;
    case 15:
      //cout << "(write)\n";
      write();
      break;
    case 16: close(); break;
    case 17: exit2(pt); break;
    default: throw_trap("invalid syscall");
  }
}

// executes one line of machine code
void execute(uint32_t mach_code) {
  //cout << "Executing: 0x" << hex << mach_code << endl;
  uint8_t  op = mach_code >> 26, funct = mach_code & last(6),
           shamt = (mach_code >> 6) & last(5), rd = (mach_code >> 11) & last(5),
           rt = (mach_code >> 16) & last(5), rs = (mach_code >> 21) & last(5);
  int16_t  imm = (int16_t)(mach_code & last(16));
  uint32_t ln_num = mach_code & last(26);

  switch (op) {
    case 0:  // R-type or syscall
      switch (funct) {
        case 0xc:
          syscall();
          break;
        // rst
        case 0b100000:
          add(rs, rt, rd);
          break;
        case 0b100001:
          addu(rs, rt, rd);
          break;
        case 0b100100:
          AND(rs, rt, rd);
          break;
        case 0b100111:
          nor(rs, rt, rd);
          break;
        case 0b100101:
          OR(rs, rt, rd);
          break;
        case 0b100010:
          sub(rs, rt, rd);
          break;
        case 0b100011:
          subu(rs, rt, rd);
          break;
        case 0b100110:
          XOR(rs, rt, rd);
          break;
        case 0b101010:
          slt(rs, rt, rd);
          break;
        case 0b101011:
          sltu(rs, rt, rd);
          break;
        // slv
        case 0b000100:
          sllv(rs, rt, rd);
          break;
        case 0b000111:
          srav(rs, rt, rd);
          break;
        case 0b000110:
          srlv(rs, rt, rd);
          break;
        // dts
        case 0b000000:
          sll(rt, rd, shamt);
          break;
        case 0b000011:
          sra(rt, rd, shamt);
          break;
        case 0b000010:
          srl(rt, rd, shamt);
          break;
        // st
        case 0b011010:
          DIV(rs, rt);
          break;
        case 0b011011:
          divu(rs, rt);
          break;
        case 0b011000:
          mult(rs, rt);
          break;
        case 0b011001:
          multu(rs, rt);
          break;
        case 0x34:
          teq(rs, rt);
          break;
        case 0x36:
          tne(rs, rt);
          break;
        case 0x30:
          tge(rs, rt);
          break;
        case 0x31:
          tgeu(rs, rt);
          break;
        case 0x32:
          tlt(rs, rt);
          break;
        case 0x33:
          tltu(rs, rt);
          break;
        // d
        case 0b010000:
          mfhi(rd);
          break;
        case 0b010010:
          mflo(rd);
          break;
        case 0b010001:
          mthi(rs);
          break;
        case 0b010011:
          mtlo(rs);
          break;
        default:  // case 0b001000
          jr(rs);
      }
      break;

    case 1:  // some branch and trap
      switch (rt) {
        case 0b00000:
          bltz(rs, imm);
          break;
        case 0b00001:
          bgez(rs, imm);
          break;
        case 0b10000:
          bltzal(rs, imm);
          break;
        case 0b10001:
          bgezal(rs, imm);
          break;
        case 0b01000:
          tgei(rs, imm);
          break;
        case 9:
          tgeiu(rs, imm);
          break;
        case 0b01010:
          tlti(rs, imm);
          break;
        case 0b01011:
          tltiu(rs, imm);
          break;
        case 0b01100:
          teqi(rs, imm);
          break;
        default:
          tnei(rs, imm);  // case 0b01110
      }
      break;

    case 2:  // j
      j(ln_num);
      break;

    case 3:  // jal
      jal(ln_num);
      break;

    case 0b001000:
      addi(rs, rt, imm);
      break;
    case 0b001001:
      addiu(rs, rt, imm);
      break;
    case 0b001100:
      andi(rs, rt, imm);
      break;
    case 0b001101:
      ori(rs, rt, imm);
      break;
    case 0b001110:
      xori(rs, rt, imm);
      break;
    case 0b001010:
      slti(rs, rt, imm);
      break;
    case 0b001011:
      sltiu(rs, rt, imm);
      break;

    case 0b100000:
      lb(rs, rt, imm);
      break;
    case 0b100100:
      lbu(rs, rt, imm);
      break;
    case 0b100001:
      lh(rs, rt, imm);
      break;
    case 0b100101:
      lhu(rs, rt, imm);
      break;
    case 0b100011:
      lw(rs, rt, imm);
      break;
    case 0x22:
      lwl(rs, rt, imm);
      break;
    case 0x26:
      lwr(rs, rt, imm);
      break;
    case 0x30:
      ll(rs, rt, imm);
      break;
    case 0xf:
      lui(rt, imm);
      break;
    case 0b101000:
      sb(rs, rt, imm);
      break;
    case 0b101001:
      sh(rs, rt, imm);
      break;
    case 0b101011:
      sw(rs, rt, imm);
      break;
    case 0x2a:
      swl(rs, rt, imm);
      break;
    case 0x2e:
      swr(rs, rt, imm);
      break;
    case 0x38:
      sc(rs, rt, imm);
      break;

    case 0b000100:
      beq(rs, rt, imm);
      break;
    case 0b000101:
      bne(rs, rt, imm);
      break;
    case 0b000111:
      bgtz(rs, imm);
      break;
    default:
      blez(rs, imm);  // case 0b000110
  }
}

int main() {
  init_reg();
  write_data_and_text();

  while ((curr_code = *PC)) {
    try {
      execute(curr_code);
      PC += 1;  // end of cycle, PC updates
    } catch (...) {
      break;
    }
    //print_regs_saved_addr();
    //system("pause");
  }

  free(REG);
  return 0;
}
