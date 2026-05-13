/**************************************************************************
 * C S 429 system emulator
 *
 * instr_Execute.c - Execute stage of instruction processing pipeline.
 **************************************************************************/

#include "hw_elts.h"
#include "instr.h"
#include "instr_pipeline.h"
#include "machine.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

extern machine_t guest;

extern comb_logic_t copy_m_ctl_sigs(m_ctl_sigs_t *, m_ctl_sigs_t *);
extern comb_logic_t copy_w_ctl_sigs(w_ctl_sigs_t *, w_ctl_sigs_t *);

extern uint8_t X_nzcvval;
extern bool X_set_flags;

/*
 * Execute stage logic.
 * STUDENT TO-DO:
 * Implement the execute stage.
 *
 * Use in as the input pipeline register,
 * and update the out pipeline register as output.
 *
 * You will need the following helper functions:
 * copy_m_ctl_signals, copy_w_ctl_signals, and alu.
 */
comb_logic_t execute_instr(x_instr_impl_t *in, m_instr_impl_t *out) {
    uint64_t alu_vala;
    uint64_t alu_valb;

    if(in->X_sigs.vala_sel) {
        if (in->op == OP_ADRP) {
            alu_vala = in->multipurpose_val.seq_succ_PC & ~0xFFFULL;
        } else {
            alu_vala = in->multipurpose_val.seq_succ_PC;
        }
    } else {
        alu_vala = in->val_a;
    }

    if(in->X_sigs.valb_sel) {
        alu_valb = in->val_b;
    } else {
        alu_valb = in->val_imm;
    }

    X_set_flags = in->X_sigs.set_flags;
    alu(alu_vala, alu_valb, in->val_hw, X_nzcvval, in->ALU_op, X_set_flags, in->cond, &out->val_ex, &out->cond_holds, &X_nzcvval);

#ifdef EC
    if (in->op == OP_BLR) {
        // `PASS_A_OP` produces the branch target in val_ex, but later stages
        // (M/W) treat val_ex as the value to retire for register writes. For
        // BLR, that must be the link address (PC+4), carried in val_imm.
        out->val_ex = (uint64_t) in->val_imm;
    }
#endif

    out->op = in->op;
    out->print_op = in->print_op;
    out->multipurpose_val.seq_succ_PC = in->multipurpose_val.seq_succ_PC;
    out->dst = in->dst;
    out->val_b = in->val_b;
    out->status = in->status;
    

    copy_m_ctl_sigs(&out->M_sigs, &in->M_sigs);
    copy_w_ctl_sigs(&out->W_sigs, &in->W_sigs);

    return;
}
