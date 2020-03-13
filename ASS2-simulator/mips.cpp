// regs save fake 32-bit address!!!!!

#include <QCoreApplication>
#include <iostream>
#include <string>
#include <bitset>
#include <fstream>
#include <limits.h>
#include <cstdlib>
#include <iomanip>
#include "mips.h"
using namespace std;

int ln_idx = 0;
ifstream f;
unordered_map<string, int> labels;

const string TITLE = "MIPS SIMULATOR for CSC3050 Assignment 2";
const string IN_PROMPT = "Please specify the absolute path of input file (e.g. /usr/local/input.asm):";
const string NUMS = "0123456789";
const string WS = " \t";
const vector<string> REG_LIT = {
    "$ze", "$at", "$v0","$v1","$a0","$a1", "$a2","$a3","$t0",  "$t1",
    "$t2","$t3","$t4","$t5", "$t6","$t7", "$s0", "$s1", "$s2","$s3", "$s4","$s5",
    "$s6","$s7","$t8","$t9","$k0","$k1","$gp","$sp","$fp","$ra","$hi","$lo"
};
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
uint32_t last(int n)
{
    return (1 << n) - 1;
}


const unordered_map<string, uint8_t> R_DST = {
    // instr rd, rs, rt
    {"add", 0b100000}, {"addu", 0b100001}, {"and", 0b100100},
    {"nor", 0b100111},  {"or", 0b100101},
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

const unordered_map<string, uint8_t> R_ST = {
    // instr rs, rt
    {"div", 0b011010},   {"divu", 0b011011}, {"mult", 0b011000},
    {"multu", 0b011001},
    {"teq", 0x34},
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
    // {"instr", op}
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

const int SIM_SIZE = 0x600000; // size of simulation (6 MB)
const int TEXT_SIZE = 0x100000; // size of text section (1 MB)
const int RESERVED_SIZE = 0x40000; // size of reserved memory

uint32_t* REG_ptr = (uint32_t*) malloc(SIM_SIZE); // simulated registers in memory
char* BASE = (char*)REG_ptr + REGS.size() * sizeof(int); // base pointer, start of text section
char* DATA = BASE + TEXT_SIZE; // data pointer, start of data section
char* STACK = DATA + (SIM_SIZE - TEXT_SIZE); // stack pointer, highest memory
uint32_t* PC = (uint32_t*)BASE; // program counter

// true address mapped to fake 32-bit
uint32_t addr_32(uint64_t true_addr, uint64_t base = (uintptr_t) BASE, uint32_t reserved = RESERVED_SIZE) {return true_addr - base + reserved;}

// fake 32-bit mapped back to true address
uint64_t addr_64(uint32_t fake, uint64_t base = (uintptr_t) BASE, uint32_t reserved = RESERVED_SIZE) {return (fake + base - reserved);}

const uint32_t REG_DEFAULT = addr_32((intptr_t)(DATA - sizeof(int))); // default value of registers


// returns the VALUE at i-th register, default Type=int
template <class T = int>
T reg_val(int idx, uint32_t* reg_ptr = REG_ptr) {
    if (idx < 32){
    uint64_t true_addr = addr_64(reg_ptr[idx]);
    return *(T*)true_addr;}
    else {
        reg_ptr += idx;
        return *(T*)reg_ptr;
    }
}

// returns cotent (default Type int) stored at pointer
template <class T = int>
T at_pointer(void* pt) {return *(T*)pt;}

// store content (default Type int) at a pointer
template <class T = int>
T* store_at_ptr(char* &p, T content = 0){
    T* pt = (T*) p;
    *pt = content;
    p += sizeof(T);
    return pt;
}

// initialize registers
void init_reg(uint32_t* reg_ptr = REG_ptr)
{
    for (size_t i = 0; i < REGS.size(); i++)
    {
        reg_ptr[i] = REG_DEFAULT;
    }
}

template<class T = int>
void set_high(T val, uint32_t* pt = REG_ptr){
    ((T*)pt)[32] = val;
}

template<class T = int>
void set_low(T val, uint32_t* pt = REG_ptr){
    ((T*)pt)[33] = val;
}

/*
 * Function: pop
 * Usage: pop<T>(my_stack)
 * ----------------------------------
 * Pops out the first element in my_stack,
 * returns a reference to that element.
 */
template<class T = string>
T pop(stack<T> &s){
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
    uint8_t rd;
    if (instr.size() == 1)
      rd = 31; // default rd = 31
    else
      rd = REGS.at(pop(instr)); // user input rd

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
    int c = 0;
    int i = 0;
    string imm_str, rs_str = "";
    string imm_rs = pop(instr); // imm_rs == "imm(rs)"
    uint8_t rt = REGS.at(pop(instr));
    uint8_t op = I_LS.at(name);

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
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
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
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
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
      imm = stoi(lab_ln); // #ln
    else {
      int label = labels[lab_ln];
      imm = label - ln_idx - 1; // label
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
    int addr;
    uint8_t op = J.at(name);
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
void get_stream(ifstream &is){
    string in_path;
    cout << TITLE << "\n\n";
    while (1){
    cout << IN_PROMPT << endl;

    //getline(cin, in_path);
    in_path = "c:/users/chen1/desktop/test.asm";

    is.open(in_path);
    if (is.fail()) {
        cout << "Invalid path! Try again.";
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
void scan(ifstream &is, vector<string> &instr){
    string curr_ln;
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
 * Function: mips_to_machine
 * Usage: vector<int> codes = mips_to_machine(instr, ofstream);
 * ----------------------------------
 * Returns a list of machine codes corresponding to the input mips instructions
 */
vector<int> mips_to_machine(vector<string> &instr)
{
    int mach_code;
    vector<int> mach_codes;
    // Second scanning: reading instructions
    for (ln_idx = 0; ln_idx < int(instr.size()); ln_idx++) {
      mach_code = make(instr[ln_idx]);
      mach_codes.push_back(mach_code);
    }
    return mach_codes;
}

void write_text(char* text_pt = BASE)
{
    int* p = (int*)text_pt;
    vector<string> instructions;
    vector<int> codes;
    get_stream(f);
    scan(f, instructions);
    codes = mips_to_machine(instructions);

    for (size_t i = 0; i < codes.size(); i++){
        p[i] = codes[i];
    }
}

template<class T = int>
void set_reg_val(int idx, T val, uint32_t* pt = REG_ptr)
{
    *(T*)addr_64(pt[idx]) = val;
}

bool add_overflow(int a, int b)
{
    return ((a > 0) && (b > INT_MAX - a)) || ((a < 0) && (b < INT_MIN - a));
}

void addu(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    set_reg_val(rd, reg_val(rs) + reg_val(rt), pt);
}

void add(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    if (add_overflow(reg_val(rs), reg_val(rt))) throw "Trapped: addition overflow!\n";
    addu(rs, rt, rd, pt);
}

void addiu(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr){
    set_reg_val(rt, reg_val(rs) + imm, pt);
}

void addi(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr){
    if (add_overflow(reg_val(rs), imm)) throw "Trapped: addition overflow!\n";
    addiu(rs, rt, imm, pt);
}

void AND(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    set_reg_val(rd, reg_val(rs) & reg_val(rt), pt);
}

void andi(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr) {
    set_reg_val(rt, reg_val(rs) & imm, pt);
}

void OR(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    set_reg_val(rd, reg_val(rs) | reg_val(rt), pt);
}

void ori(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr) {
    set_reg_val(rt, reg_val(rs) | imm, pt);
}

void nor(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    set_reg_val(rd, !(reg_val(rs) | reg_val(rt)), pt);
}

void XOR(int rs, int rt, int rd, uint32_t* pt = REG_ptr) {
    set_reg_val(rd, reg_val(rs) xor reg_val(rt), pt);
}

void xori(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr) {
    set_reg_val(rt, reg_val(rs) xor imm, pt);
}

bool sub_overflow(int a, int b)
{
    return ((b > 0) && (a < INT_MIN + b)) || ((b < 0) && (a > INT_MAX + b));
}

void subu(int rs, int rt, int rd, uint32_t* pt = REG_ptr){
    set_reg_val(rd, reg_val(rs) - reg_val(rt), pt);
}

void sub(int rs, int rt, int rd, uint32_t* pt = REG_ptr){
    if (sub_overflow(reg_val(rs), reg_val(rt))) throw "Trapped: subtraction overflow!\n";
    subu(rs, rt, rd, pt);
}

bool mult_overflow(int a, int b)
{
    if (b >= 0) return (a > LONG_MAX / b) || (a < LONG_MIN / b);
    return (a < LONG_MAX / b) || (a > LONG_MIN / b);
}

bool multu_overflow(uint32_t a, uint32_t b){
    return a > ULONG_MAX / b;
}

void mult(int rs, int rt, uint32_t* pt = REG_ptr){
    if (mult_overflow(reg_val(rs), reg_val(rt))) throw "Trapped: multiplication overflow!\n";
    int64_t ans = (int64_t)reg_val(rs) * (int64_t)reg_val(rt);
    int high = ans >> 32; // high-order word
    int low = ans & 0xFFFFFFFF; // low-order word
    set_high(high, pt);
    set_low(low, pt);
}

void multu(uint32_t rs, uint32_t rt, uint32_t* pt = REG_ptr){
    if (multu_overflow(reg_val<uint32_t>(rs), reg_val<uint32_t>(rt))) throw "Trapped: unsigned multiplication overflow!\n";
    uint64_t ans = (int64_t)reg_val(rs) * (int64_t)reg_val(rt);
    int high = ans >> 32; // high-order word
    int low = ans & 0xFFFFFFFF; // low-order word
    set_high(high, pt);
    set_low(low, pt);
}

bool div_overflow(int p, int q) // p ÷ q
{
    return (q == 0) || (p == INT_MIN && q == -1);
}

void divu(int rs, int rt, uint32_t* pt = REG_ptr){
    int quo = reg_val(rs, pt) / reg_val(rt, pt);
    int rem = reg_val(rs, pt) % reg_val(rt, pt);
    set_high(rem, pt);
    set_low(quo, pt);
}

void div(int rs, int rt, uint32_t* pt = REG_ptr){
    if (div_overflow(rs, rt)) throw "Trapped: division overflow!\n";
    divu(rs, rt, pt);
}

void mfhi(int rd, uint32_t* pt = REG_ptr){
    set_reg_val(rd, reg_val(32, pt), pt);
}

void mflo(int rd, uint32_t* pt = REG_ptr){
    set_reg_val(rd, reg_val(33, pt), pt);
}

void mthi(int rs, uint32_t* pt = REG_ptr){
    set_high(reg_val(rs, pt));
}

void mtlo(int rs, uint32_t* pt = REG_ptr){
    set_low(reg_val(rs, pt));
}

void teq(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) == reg_val(rt, pt)) throw "Trapped!";
}

void teqi(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) == imm) throw "Trapped!";
}

void tne(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) != reg_val(rt, pt)) throw "Trapped!";
}

void tnei(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) != imm) throw "Trapped!";
}

void tge(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) >= reg_val(rt, pt)) throw "Trapped!";
}

void tgeu(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val<uint32_t>(rs, pt) >= reg_val<uint32_t>(rt, pt)) throw "Trapped!";
}

void tgei(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) >= imm) throw "Trapped!";
}

void tgeiu(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val<uint32_t>(rs, pt) >= imm) throw "Trapped!";
}

void tlt(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) < reg_val(rt, pt)) throw "Trapped!";
}

void tltu(int rs, int rt, uint32_t* pt = REG_ptr){
    if (reg_val<uint32_t>(rs, pt) < reg_val<uint32_t>(rt, pt)) throw "Trapped!";
}

void tlti(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val(rs, pt) < imm) throw "Trapped!";
}

void tltiu(int rs, int16_t imm, uint32_t* pt = REG_ptr){
    if (reg_val<uint32_t>(rs, pt) < imm) throw "Trapped!";
}

void slt(int rs, int rt, int rd, uint32_t* pt = REG_ptr)
{
    int ans = reg_val(rs, pt) < reg_val(rt, pt);
    set_reg_val(rd, ans, pt);
}

void sltu(int rs, int rt, int rd, uint32_t* pt = REG_ptr)
{
    int ans = reg_val<uint32_t>(rs, pt) < reg_val<uint32_t>(rt, pt);
    set_reg_val(rd, ans, pt);
}

void slti(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr)
{
    int ans = reg_val(rs, pt) < imm;
    set_reg_val(rt, ans, pt);
}

void sltiu(int rs, int rt, int16_t imm, uint32_t* pt = REG_ptr)
{
    int ans = reg_val<uint32_t>(rs, pt) < imm;
    set_reg_val(rt, ans, pt);
}

void jr(int rs, uint32_t* pt = REG_ptr)
{   // unconditionally jump to addr in rs
    // however PC += 1 each time, hence -1
    PC = (uint32_t*)addr_64(pt[rs]) - 1;
}

void jalr(int rs, int rd, uint32_t* pt = REG_ptr)
{   // unconditionally jump to addr in rs, store next addr in rd
    // however PC += 1 each time, hence -1
    uint32_t* addr = (uint32_t*)addr_64(pt[rs]);
    PC = addr - 1;
    uint32_t fake_addr = addr_32((intptr_t)(addr + 1));
    set_reg_val(rd, fake_addr, pt);
}

void j(uint32_t ln_num)
{
    uint32_t curr_ln_num = 1 + ((intptr_t)PC - (intptr_t)BASE) / 4;
    int diff = ln_num - curr_ln_num;
    PC += diff; PC -= 1; // PC += 1 each time, hence -1
}

void jal(uint32_t ln_num)
{
    uint32_t curr_ln_num = 1 + ((intptr_t)PC - (intptr_t)BASE) / 4;
    int diff = ln_num - curr_ln_num;
    PC += diff;
    set_reg_val<uint32_t>(31, addr_32((intptr_t)(PC + 1)));
    PC -= 1; // PC += 1 each time, hence -1
}

void sll(int rt, int rd, uint8_t shamt, uint32_t* pt = REG_ptr){
    pt[rd] = pt[rt] << shamt;
}

void sllv(int rs, int rt, int rd, uint32_t* pt = REG_ptr){
    uint32_t shift = reg_val<uint32_t>(rs, pt);
    pt[rd] = pt[rt] << shift;
}

void srl(int rt, int rd, uint8_t shamt, uint32_t* pt = REG_ptr){
    pt[rd] = pt[rt] >> shamt;
}

void srlv(int rs, int rt, int rd, uint32_t* pt = REG_ptr){
    uint32_t shamt = reg_val<uint32_t>(rs, pt);
    pt[rd] = pt[rt] >> shamt;
}

void sra(int rt, int rd, uint8_t shamt, uint32_t* pt = REG_ptr){
    pt[rd] = (uint32_t)((int32_t)(pt[rt]) >> shamt);
}

void srav(int rs, int rt, int rd, uint32_t* pt = REG_ptr){
    uint32_t shamt = reg_val<uint32_t>(rs, pt);
    pt[rd] = (uint32_t)((int32_t)(pt[rt]) >> shamt);
}

void execute(uint32_t mach_code, uint32_t* pt = REG_ptr)
{
    cout << "Executing: " << hex << mach_code << endl;

    uint8_t op = mach_code >> 26,
            funct = mach_code & last(6),
            shamt = (mach_code >> 6) & last(5),
            rd = (mach_code >> 11) & last(5),
            rt = (mach_code >> 16) & last(5),
            rs = (mach_code >> 21) & last(5);
    int16_t imm = (int16_t)(mach_code & last(16));
    uint32_t ln_num = mach_code & last(26);

    switch (op) {
    case 0: // R-type
        switch (funct) {
        // rst
        case 0b100000:
            add(rs, rt, rd, pt); break;
        case 0b100001:
            addu(rs,rt,rd, pt); break;
        case 0b100100:
            AND(rs,rt,rd,pt); break;
        case 0b100111:
            nor(rs,rt,rd,pt); break;
        case 0b100101:
            OR(rs,rt,rd,pt); break;
        case 0b100010:
            sub(rs,rt,rd,pt); break;
        case 0b100011:
            subu(rs,rt,rd,pt); break;
        case 0b100110:
            XOR(rs,rt,rd,pt); break;
        case 0b101010:
            slt(rs,rt,rd, pt); break;
        case 0b101011:
            sltu(rs,rt,rd,pt); break;
        // slv
        case 0b000100:
            sllv(rs,rt,rd,pt); break;
        case 0b000111:
            srav(rs,rt,rd,pt); break;
        case 0b000110:
            srlv(rs,rt,rd,pt); break;
        // dts
        case 0b000000:
            sll(rt,rd,shamt,pt); break;
        case 0b000011:
            sra(rt,rd,shamt,pt); break;
        case 0b000010:
            srl(rt,rd,shamt,pt); break;
        // st
        case 0b011010:
            div(rs,rt,pt); break;
        case 0b011011:
            divu(rs,rt,pt); break;
        case 0b011000:
            mult(rs,rt,pt); break;
        case 0b011001:
            multu(rs,rt,pt); break;
        case 0x34:
            teq(rs,rt,pt); break;
        case 0x36:
            tne(rs,rt,pt); break;
        case 0x30:
            tge(rs,rt,pt); break;
        case 0x31:
            tgeu(rs,rt,pt); break;
        case 0x32:
            tlt(rs,rt,pt); break;
        case 0x33:
            tltu(rs,rt,pt); break;
        // d
        case 0b010000:
            mfhi(rd,pt); break;
        case 0b010010:
            mflo(rd,pt); break;
        case 0b010001:
            mthi(rs,pt); break;
        case 0b010011:
            mtlo(rs,pt); break;
        default: // case 0b001000
            jr(rs,pt);
        }
        break;

    case 1: // some branch and trap
        switch (rt) {
        case 0b00000: bltz(rs, imm, pt); break;
        case 0b00001: bgez(rs, imm, pt); break;
        case 0b10000: bltzal(rs, imm, pt); break;
        case 0b10001: bgezal(rs, imm, pt); break;
        case 0b01000: tgei(rs, imm, pt); break;
        case 9: tgeiu(rs, imm, pt); break;
        case 0b01010: tlti(rs, imm, pt); break;
        case 0b01011: tltiu(rs, imm, pt); break;
        case 0b01100: teqi(rs, imm, pt); break;
        default: tnei(rs, imm, pt); //case 0b01110
        }
        break;

    case 2:  // j
        j(ln_num);
        break;

    case 3: // jal
        jal(ln_num);
        break;

    case 0b001000: addi(rs, rt, imm, pt); break;
    case 0b001001: addiu(rs, rt, imm, pt); break;
    case 0b001100: andi(rs, rt, imm, pt); break;
    case 0b001101: ori(rs, rt, imm, pt); break;
    case 0b001110: xori(rs, rt, imm, pt); break;
    case 0b001010: slti(rs, rt, imm, pt); break;
    case 0b001011: sltiu(rs, rt, imm, pt); break;

    case 0b100000: lb(rt, imm, pt); break;
    case 0b100100: lbu(rt, imm, pt); break;
    case 0b100001: lh(rt, imm, pt); break;
    case 0b100101: lhu(rt, imm, pt); break;
    case 0b100011: lw(rt, imm, pt); break;
    case 0b110001: lwc1(rt, imm, pt); break;
    case 0x22: lwl(rt, imm, pt); break;
    case 0x26: lwr(rt, imm, pt); break;
    case 0x30: ll(rt, imm, pt); break;
    case 0b101000: sb(rt, imm, pt); break;
    case 0b101001: sh(rt, imm, pt); break;
    case 0b101011: sw(rt, imm, pt); break;
    case 0x2a: swl(rt, imm, pt); break;
    case 0x2e: swr(rt, imm, pt); break;
    case 0x38: sc(rt, imm, pt); break;

    case 0b000100: beq(rs, rt, imm, pt); break;
    case 0b000101: bne(rs, rt, imm, pt); break;
    case 0b000111: bgtz(rs, imm, pt); break;
    default: blez(rs, imm, pt); // case 0b000110
    }
}

void print_title(string s = "-", int len = 141, char fill = '-')
{
    int fillw;
    int halfLen = len / 2;
    int halfSlen = s.length() / 2;
    fillw = halfLen - halfSlen;
    cout << setw(s.length() + fillw) << right << setfill(fill) << s;
    cout << setw(fillw - 1) << setfill(fill) << fill << endl;
}

void print_bytes(int num_of_bytes, char* ptr = BASE)
{
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

// print INTEGER values at the address stored in register idx
template<class T = int>
void print_reg_val(int idx, uint32_t* reg_ptr = REG_ptr){
    if (idx < 32)
    cout << dec << REG_LIT[idx] << ": " << reg_val<T>(idx, reg_ptr);
    else cout << dec << REG_LIT[idx] << ": " << *(T*)(reg_ptr + idx);
}

// print INTEGER values at the address stored in registers
void print_regs_val(){
    print_title("REG VALUES");
    for (size_t i = 0; i < REGS.size(); i++){
        print_reg_val(i);   // assuming integer
        cout << '\t';
        if (i % (REGS.size() / 2) == REGS.size() / 2 - 1) cout << '\n';
    }
    cout << '\n';
    print_title("-");
}

void print_reg_saved_addr(int idx, uint32_t* &ptr = REG_ptr){
    cout << REG_LIT[idx] << ": 0x" << hex << ptr[idx];
}

// regs saved fake (32-bit) address!!!!!
void print_regs_saved_addr(uint32_t* &ptr = REG_ptr)
{
    print_title("REGISTERS");
    for (size_t i = 0; i < REGS.size(); i++) {
        print_reg_saved_addr(i, ptr);
        cout << '\t';
        if (i % 9 == 8) cout << '\n';
    } cout << '\n'; print_title();
}

int main() {

    init_reg();
    write_text();

    ((int*)BASE)[0] = 0XFFFFFFFF;
    ((int*)BASE)[1] = 0XFFFFFFFF;

    int rs = 0, rt = 1, rd = 2;
    REG_ptr[rs] = addr_32((intptr_t)BASE);
    REG_ptr[rt] = REG_ptr[rs] + sizeof(int);
    REG_ptr[rd] = REG_ptr[rt] + sizeof(int);
    REG_ptr[32] = REG_ptr[rt] + sizeof(int);
    REG_ptr[33] = REG_ptr[32] + sizeof(int);

    print_bytes(500);

    try {

        add(rs, rt, rd);
        //print_regs_val();

    } catch (const char* e) {
        cerr << e << endl;
        return 0;
    }

    print_regs_saved_addr();
    print_bytes(500);

    free(REG_ptr);
    return 0;

}
