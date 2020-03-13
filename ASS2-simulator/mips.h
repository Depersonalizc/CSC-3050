/*
 * File: mips.h
 * --------------
 * This file exports a set of functions that are involved in
 * translating a MIPS(.asm) file into "executable" machine codes(.txt) file
 *
 * @version 2020/3/2
 */

#ifndef MIPS_H
#define MIPS_H

#include <unordered_map>
#include <stack>
#include <vector>
#include <fstream>

using namespace std;


/*
 * Function: makeR
 * Usage: int R = makeR("add $s1, $s2, $s3")
 * ----------------------------------
 * Return the machine code of an R-type instruction.
 */
int makeR(uint8_t op, uint8_t func, uint8_t rs, uint8_t rt, uint8_t rd,
          uint8_t shamt = 0);

/*
 * Function: makeI
 * Usage: int I = makeI("addi $s1, $s2, 100")
 * ----------------------------------
 * Return the machine code of an I-type instruction.
 */
int makeI(uint8_t op, uint8_t rs, uint8_t rt, uint16_t imm);

/*
 * Function: makeJ
 * Usage: int j = makeJ("j 100")
 * ----------------------------------
 * Return the machine code of a J-type instruction.
 */
int makeJ(uint8_t op, int ln_num);

/*
 * Function: break_instr
 * Usage: stack<string> instr = break_instr("ll  $v0, 	 24($v0)");
 *        // instr == (top) ["ll", "$24($v0)", "$v0"] (bottom)
 * ----------------------------------
 * Break a single-line string instruction into a corresponding stack,
 * with the name of the instruction on top of the stack.
 */
stack<string> break_instr(string instr);

/*
 * Function: make
 * Usage: int m_code = make("j 100");
 * ----------------------------------
 * Returns the translated machine code of a
 * one-line instruction (already shrunk.)
 */
int make(string instruction);

/*
 * Function: get_label
 * Usage: get_label("\tJ:   j 100  ", my_labels);
 *      // "j 100   "
 * ----------------------------------
 * Store info of label (if any) into a hashmap,
 * remove the label from the instruction.
 */
void get_label(string &str, unordered_map<string, int> &labs);

/*
 * Function: no_comment
 * Usage: strip("abc # comments");
 * // "abc "
 * ----------------------------------
 * Deletes the comments starting with '#' from a string.
 */
void no_comment(string &str);

/*
 * Function: strip
 * Usage: strip("\t   abc, 123 ");
 * // "abc, 123"
 * ----------------------------------
 * Strips the whitespaces on the both ends of a string.
 */
void strip(string &str);

/*
 * Function: get_stream
 * Usage: get_stream(ifstream, ofstream);
 * ----------------------------------
 * Prompts user for file path, set i/o fstreams ready for scanning
 */
void get_stream(ifstream &is, ofstream &os);

/*
 * Function: scan
 * Usage: scan(ifstream, instr);
 * ----------------------------------
 * Scans instructions through ifstream,
 * stores them into vector instr.
 */
void scan(ifstream &is, vector<string> &instr);

/*
 * Function: mips_to_machine
 * Usage: vector<int> codes = mips_to_machine(instr, ofstream);
 * ----------------------------------
 * Returns a list of machine codes corresponding to the input mips instructions
 */
void mips_to_machine(vector<string> &instr, ofstream &os);


#endif // MIPS_H
