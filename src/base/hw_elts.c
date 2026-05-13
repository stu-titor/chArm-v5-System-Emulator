/**************************************************************************
 * C S 429 system emulator
 *
 * hw_elts.c - Module for emulating hardware elements.
 *
 * Copyright (c) 2022, 2023, 2024, 2025.
 * Authors: S. Chatterjee, Z. Leeper., P. Jamadagni, W. Borden.
 * All rights reserved.
 * May not be used, modified, or copied without permission.
 **************************************************************************/

#include "hw_elts.h"
#include "err_handler.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include "mem.h"
#include <assert.h>
#include <stdint.h>

extern machine_t guest;

fa_t rca[64];

/*
 * Read from instruction memory. Students, do not modify.
 */
comb_logic_t imem(uint64_t imem_addr, uint32_t *imem_rval, bool *imem_err) {
    // imem_addr must be in "instruction memory" and a multiple of 4
    *imem_err = (!addr_in_imem(imem_addr) || (imem_addr & 0x3U));
    *imem_rval = (uint32_t) mem_read_I(imem_addr);
}

/*
 * Sets up the inputs to the ripple carry adder.
 * STUDENT TO-DO:
 */
comb_logic_t init_rca(uint64_t val_a, uint64_t val_b, bool c_in) {
    rca[0].c_in = c_in;
    for(int i = 0; i < 64; i++) {
        rca[i].bit_a = (val_a >> i) & 1;
        rca[i].bit_b = (val_b >> i) & 1;
    }
    return;
}

/*
 * Performs the ripple carry add.
 * STUDENT TO-DO:
 */
comb_logic_t ripple_carry_add(uint64_t *sum) {
    rca[0].s = (rca[0].bit_a ^ rca[0].bit_b) ^ rca[0].c_in;
    rca[0].c_out = (rca[0].bit_a & rca[0].bit_b) | (rca[0].bit_a & rca[0].c_in) | (rca[0].bit_b & rca[0].c_in);
    
    for(int i = 1; i < 64; i++) {
        rca[i].c_in = rca[i - 1].c_out;
        rca[i].s = (rca[i].bit_a ^ rca[i].bit_b) ^ rca[i].c_in;
        rca[i].c_out = (rca[i].bit_a & rca[i].bit_b) | (rca[i].bit_a & rca[i].c_in) | (rca[i].bit_b & rca[i].c_in);
    }

    // assemble sum bits into the output
    *sum = 0;
    for (int i = 0; i < 64; i++) {
        *sum |= ((uint64_t)rca[i].s << i);
    }
    
    return;
}

/*
 * Read from register file.
 * STUDENT TO-DO:
 * Read from src1 and src2 registers. Take extra care for SP/XZR.
 */
comb_logic_t regfile_read(uint8_t src1, uint8_t src2, uint64_t *val_a,
                          uint64_t *val_b) {
    if(src1 == XZR_NUM){
        *val_a = 0;
    } else if(src1 == SP_NUM){
        *val_a = guest.proc->SP;
    } else {
        *val_a = guest.proc->GPR[src1];
    }
    if(src2 == XZR_NUM){
        *val_b = 0;
    } else if(src2 == SP_NUM){
        *val_b = guest.proc->SP;
    } else {
        *val_b = guest.proc->GPR[src2];
    }
    return;
}


/*
 * Write to register file.
 * STUDENT TO-DO:
 * Write to dst register if enabled. Take extra care for SP/XZR.
 */
comb_logic_t regfile_write(uint8_t dst, uint64_t val_w, bool w_enable) {
    if(w_enable){
        if(dst < SP_NUM){
            guest.proc->GPR[dst] = val_w;
        } else if(dst == SP_NUM){
            guest.proc->SP = val_w;
        }
    }
    return;
}

/*
 * Check whether a condition is satisfied given the NCZV status flags.
 * STUDENT TO-DO:
 */
static bool cond_holds(cond_t cond, uint8_t flags) {
    bool N = (flags >> 3) & 0x1;
    bool Z = (flags >> 2) & 0x1;
    bool C = (flags >> 1) & 0x1;
    bool V = flags & 0x1;
    switch(cond) {
        case C_EQ:
            return Z;
        case C_NE:
            return !Z;
        case C_CS:
            return C;
        case C_CC:
            return !C;
        case C_MI:
            return N;
        case C_PL:
            return !N;
        case C_VS:
            return V;
        case C_VC:
            return !V;
        case C_HI:
            return C && !Z;
        case C_LS:
            return !(C && !Z);
        case C_GE:
            return N == V;
        case C_LT:
            return !(N == V);
        case C_GT:
            return !Z && (N == V);
        case C_LE: 
            return !(!Z && (N == V));
        case C_AL:
            return true;
        case C_NV:
            return true;
        default:
            return false;
    }
    return false;
}

/*
 * Perform the appropriate ALU operation, setting NZCV flags if needed.
 * STUDENT TO-DO:
 */
comb_logic_t alu(uint64_t alu_vala, uint64_t alu_valb, uint8_t alu_valhw,
                 uint8_t nzcv, alu_op_t ALUop, bool set_flags, cond_t cond,
                 uint64_t *val_e, bool *cond_val, uint8_t *nzcv_dst) {
    *cond_val = cond_holds(cond, nzcv);
    switch(ALUop) {
        case PLUS_OP: 
            init_rca(alu_vala, alu_valb, false);
            ripple_carry_add(val_e);
            break;
        case MINUS_OP:
            init_rca(alu_vala, ~alu_valb, true);
            ripple_carry_add(val_e);
            break;
        case INV_OP:
            *val_e = alu_vala | (~alu_valb);
            break;
        case OR_OP:
            *val_e = alu_vala | alu_valb;
            break;
        case EOR_OP:
            *val_e = alu_vala ^ alu_valb;
            break;
        case AND_OP: 
            *val_e = alu_vala & alu_valb;
            break; 
        case MOV_OP:
            *val_e = alu_vala | (alu_valb << alu_valhw);
            break;
        case MOVK_OP:
            *val_e = (alu_vala & (~(0xFFFFUL << alu_valhw))) | (alu_valb << alu_valhw);
            break;
        case LSL_OP:
            *val_e = alu_vala << (alu_valb & 0x3FUL);
            break;
        case LSR_OP:
            *val_e = alu_vala >> (alu_valb & 0x3FUL);
            break;
        case ASR_OP:
            *val_e = (int64_t)alu_vala >> (alu_valb & 0x3FUL);
            break;
        case PASS_A_OP:
            *val_e = alu_vala;
            break;
#ifdef EC
        case CSEL_OP:
            *val_e = (*cond_val) ? alu_vala : alu_valb;
            break;
        case CSINV_OP:
            *val_e = (*cond_val) ? alu_vala : ~alu_valb;
            break;
        case CSINC_OP:
            if (*cond_val) {
                *val_e = alu_vala;
            } else {
                init_rca(alu_valb, 1, false);
                ripple_carry_add(val_e);
            }
            break;
        case CSNEG_OP:
            if (*cond_val) {
                *val_e = alu_vala;
            } else {
                init_rca(0, ~alu_valb, true);
                ripple_carry_add(val_e);
            }
            break;
        case CBZ_OP:
            *val_e = alu_vala;
            *cond_val = (alu_vala == 0);
            break;
        case CBNZ_OP:
            *val_e = alu_vala;
            *cond_val = (alu_vala != 0);
            break;
#endif
        default:
            break;
    }
    
    if(set_flags) {
        *nzcv_dst = 0;
        if(*val_e >> 63 == 1) *nzcv_dst |= 0x8;
        if(*val_e == 0) *nzcv_dst |= 0x4;
        if(ALUop == PLUS_OP || ALUop == MINUS_OP) {
            if(rca[63].c_out == 1) *nzcv_dst |= 0x2;
            if((rca[63].c_out ^ rca[63].c_in) == 1) *nzcv_dst |= 0x1; 
        }
    }

    return;
}

/*
 * Read from data memory, Students do not modify.
 */
comb_logic_t dmem(uint64_t dmem_addr, uint64_t dmem_wval, bool dmem_read,
                  bool dmem_write, uint64_t *dmem_rval, bool *dmem_err) {
    if (!dmem_read && !dmem_write) {
        return;
    }
    // dmem_addr must be in "data memory" and a multiple of 8
    *dmem_err = (!addr_in_dmem(dmem_addr) || (dmem_addr & 0x7U));
    if (is_special_addr(dmem_addr))
        *dmem_err = false;
    if (dmem_read)
        *dmem_rval = (uint64_t) mem_read_L(dmem_addr);
    if (dmem_write)
        mem_write_L(dmem_addr, dmem_wval);
}
