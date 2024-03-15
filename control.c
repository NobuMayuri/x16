#include <stdio.h>
#include <stdlib.h>
#include "bits.h"
#include "control.h"
#include "instruction.h"
#include "x16.h"
#include "trap.h"
#include "decode.h"


// Update condition code based on result
void update_cond(x16_t* machine, reg_t reg) {
    uint16_t result = x16_reg(machine, reg);
    if (result == 0) {
        x16_set(machine, R_COND, FL_ZRO);
    } else if (is_negative(result)) {
        x16_set(machine, R_COND, FL_NEG);
    } else {
        x16_set(machine, R_COND, FL_POS);
    }
}



// Execute a single instruction in the given X16 machine. Update
// memory and registers as required. PC is advanced as appropriate.
// Return 0 on success, or -1 if an error or HALT is encountered.
int execute_instruction(x16_t* machine) {
    // Fetch the instruction and advance the program counter
    uint16_t pc = x16_pc(machine);
    uint16_t instruction = x16_memread(machine, pc);
    x16_set(machine, R_PC, pc + 1);

    if (LOG) {
        fprintf(LOGFP, "0x%x: %s\n", pc, decode(instruction));
    }

    // Variables we might need in various instructions
    reg_t dst, src1, src2, base;
    uint16_t result, indirect, offset, imm, cond, jsrflag, op1, op2;

    // Decode the instruction
    uint16_t opcode = getopcode(instruction);
    switch (opcode) {
        case OP_ADD:
            dst = getbits(instruction, 9, 3);
            src1 = (getbits(instruction, 6, 3));
            if (getbit(instruction, 5) == 0){
                src2 = getbits(instruction, 0, 3);
        x16_set(machine, dst, x16_reg(machine, src1) + x16_reg(machine, src2));
            } else {
                imm = getbits(instruction, 0, 5);
                imm = sign_extend(imm, 5);
                x16_set(machine, dst, x16_reg(machine, src1) + imm);
            }
            update_cond(machine, dst);
            break;

        case OP_AND:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            if (getbit(instruction, 5) == 0){
                src2 = getbits(instruction, 0, 3);
        x16_set(machine, dst, x16_reg(machine, src1) & x16_reg(machine, src2));
            } else {
                imm = getbits(instruction, 0, 5);
                imm = sign_extend(imm, 5);
                x16_set(machine, dst, x16_reg(machine, src1) & imm);
            }
            update_cond(machine, dst);
            break;

        case OP_NOT:
            dst = getbits(instruction, 9, 3);
            src1 = getbits(instruction, 6, 3);
            x16_set(machine, dst, ~(x16_reg(machine, src1)));
            update_cond(machine, dst);
            break;

        case OP_BR:
            cond =  x16_cond(machine);
            int n = getbit(instruction, 11);
            int z = getbit(instruction, 10);
            int p = getbit(instruction, 9);
    if ((n&&cond == 4) || (z&&cond == 2) || (p&&cond == 1) || (!n&&!z&&!p)) {
                offset = getbits(instruction, 0 , 9);
                offset = sign_extend(offset, 9);
                x16_set(machine, R_PC, pc + 1 + offset);
            }
            break;

        case OP_JMP:
            base = getbits(instruction, 6, 3);
            x16_set(machine, R_PC, x16_reg(machine, base));
            break;

        case OP_JSR:
            x16_set(machine, R_R7, pc+1);
            if (getbit(instruction, 11) == 0){
                base = getbits(instruction, 6, 3);
                x16_set(machine, R_PC, x16_reg(machine, base));
            } else {
                offset = getbits(instruction, 0, 11);
                offset = sign_extend(offset, 11);
                x16_set(machine, R_PC, pc + 1 + offset);
            }
            break;

        case OP_LD:
            dst = getbits(instruction, 9, 3);
            offset = getbits(instruction, 0, 9);
            offset = sign_extend(offset, 9);
            x16_set(machine, dst, x16_memread(machine, pc+1+offset));
            update_cond(machine, dst);
            break;

        case OP_LDI:
            dst = getbits(instruction, 9, 3);
            offset = getbits(instruction, 0, 9);
            offset = sign_extend(offset, 9);
            indirect = x16_memread(machine, pc+1+offset);
            x16_set(machine, dst, x16_memread(machine, indirect));
            update_cond(machine, dst);
            break;

        case OP_LDR:
            dst = getbits(instruction, 9, 3);
            offset = getbits(instruction, 0, 6);
            offset = sign_extend(offset, 6);
            base = x16_reg(machine, getbits(instruction, 6, 3));
            x16_set(machine, dst, x16_memread(machine, offset+base));
            update_cond(machine, dst);
            break;

        case OP_LEA:
            dst = getbits(instruction, 9, 3);
            offset = getbits(instruction, 0, 9);
            offset = sign_extend(offset, 9);
            x16_set(machine, dst, pc + 1 + offset);
            update_cond(machine, dst);
            break;

        case OP_ST:
            dst = getbits(instruction, 9,  3);
            offset = getbits(instruction, 0, 9);
            offset = sign_extend(offset, 9);
            x16_memwrite(machine, pc+1+offset, x16_reg(machine, dst));
            break;

        case OP_STI:
            dst = getbits(instruction, 9,  3);
            offset = getbits(instruction, 0, 9);
            offset = sign_extend(offset, 9);
            base = x16_memread(machine, pc+1+offset);
            x16_memwrite(machine, base, x16_reg(machine, dst));
            break;

        case OP_STR:
            dst = getbits(instruction, 9,  3);
            offset = getbits(instruction, 0, 6);
            offset = sign_extend(offset, 6);
            base = x16_reg(machine, getbits(instruction, 6, 3));
            x16_memwrite(machine, offset+base, x16_reg(machine, dst));
            break;

        case OP_TRAP:
            // Execute the trap -- do not rewrite
            return trap(machine, instruction);

        case OP_RES:
        case OP_RTI:
        default:
            // Bad codes, never used
            abort();
    }

    return 0;
}
