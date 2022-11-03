#define _POSIX_C_SOURCE 200809L // enable strdup

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include "ast.h"
#include "mips_ast.h"
#include "util.h"

static FILE * out;

// Push frpm register v0
static void push0();

// Push from register v1
static void push1();

// Pop to register v0
static void pop0();

// Pop to register v1
static void pop1();

static void emitInstruction(char * instruction, char * comment, ...);

static void emitLabel(char * label, char * comment, ...);

void mips_astExpr(exp_node * e, S_table global_types, S_table function_rets, frame * f);

static void intrinsic(char * name, list * args, S_table global_types, S_table function_rets, frame * f) { // list of exp_node*


    if (!strcmp(name, "exit")) {
        // Push all args to stack
        exp_node * ret = (exp_node *) args->head;
        mips_astExpr(ret, global_types, function_rets, f);
        // POP and pass to system call
        pop0(); // Pop to $v0
        emitInstruction("move $a0, $v0", "EXIT pass return code");
        emitInstruction("li $v0, 17", "EXIT specify exit2 as the syscall to perform");
        emitInstruction("syscall", "EXIT perform syscall");
        return;
    }

    if (!strcmp(name, "printint")) {
        // Push all args to stack
        exp_node * intToPrint = (exp_node *) args->head;
        mips_astExpr(intToPrint, global_types, function_rets, f);
        // POP and pass to system call
        pop0(); // Pop to $v0
        emitInstruction("move $a0, $v0", "PRINTINT pass int to be printed");
        emitInstruction("li $v0, 1", "PRINTINT specify printint as the syscall to perform");
        emitInstruction("syscall", "PRINTINT perform syscall");
        return;
    }
    assert(0);
}

void mips_astExpr(exp_node * e, S_table global_types, S_table function_rets, frame * f) {
    UNUSED(global_types);
    UNUSED(function_rets);
    UNUSED(f);
    if(!e) return;
    switch(e->kind){
        case int_exp: {
            // Push ival of expression to the stack
            emitInstruction("li $v0, %d", "astExpr - int_exp: load resulting integer value to push to the stack", e->data.ival);
            push0();
            break;
        }
        case string_exp: {
            assert(0);
        }
        case binop_exp: {
            if (e->data.bin_ops.op == plus_op) {
                mips_astExpr(e->data.bin_ops.e1, global_types, function_rets, f);
                // store value on stack from above recursive call into a register;
                mips_astExpr(e->data.bin_ops.e2, global_types, function_rets, f);
                pop0();
                pop1();

                emitInstruction("add $v0, $v0, $v1","BINOP_EXP PLUS_OP - Add the two operands");
                // push result to top of stack
                push0();

                return;
            }
            assert(0);
        }
        case call_exp: {
            assert(0);
        }
        case unop_exp: {
            assert(0);
        }
        case var_exp: {
            assert(0);
        }
        case intrinsic_exp: {
            intrinsic(e->data.intrinsic_ops.name, e->data.intrinsic_ops.args, global_types, function_rets, f);
            break;
        }
        default:
            assert(0);
    }
}

static char * generateFreshLabel() {
    static char buffer[1024];
    static int lastLabel = 0;

    snprintf(buffer, sizeof(buffer), "L%d", lastLabel);
    lastLabel += 1;
    return strdup(buffer);
}


void mips_astStmts(list * l, S_table globals, S_table function_rets, frame * f);

void mips_astStmt(stmt_node * s, S_table global_types, S_table function_rets, frame * f) {
    UNUSED(global_types);
    UNUSED(function_rets);
    UNUSED(f);
    if(!s) return;
    switch(s->kind){
        case assign_stmt: {
            assert(0);
        }
        case if_stmt: {
            assert(0);
        }
        case while_stmt: {
            assert(0);
        }
        case repeat_stmt: {
            assert(0);
        }
        case ret_stmt: {
            assert(0);
        }
        case call_stmt: {
            assert(0);
        }
        case intrinsic_stmt: {
             intrinsic(s->data.intrinsic_ops.name, s->data.intrinsic_ops.args, global_types, function_rets, f);
             break;
         }
        default:
            assert(0);
    }
}

void mips_astStmts(list * l, S_table globals_types, S_table function_rets, frame * f) {
    if (l == NULL) return;
    mips_astStmt(l->head, globals_types, function_rets, f);
    mips_astStmts(l->next, globals_types, function_rets, f);
}

void mips_astVariable(vardec_node * node, S_table globals_types, S_table function_rets, frame * f) {
    mips_astExpr(node->init, globals_types, function_rets, f);
}

void mips_astVariables(list * l, S_table global_types, S_table function_rets, frame * f) {
    if (l == NULL) return;
    fprintf(out, ".data\n");
    mips_astVariable((vardec_node *)l->head, global_types, function_rets, f);
    mips_astVariables(l->next, global_types, function_rets, f);
}

void mips_astFunction(fundec_node * fundec, S_table globals, S_table functions_rets, frame * f) {
    UNUSED(fundec);
    mips_astStmts(fundec->stmts, globals, functions_rets, f);
}

void mips_astFunctions(list * l, S_table global_types, S_table function_ret_types, S_table frames) {
    if (l == NULL) return;
    fundec_node * fundec = (fundec_node*) l->head;
    frame * f = S_look(frames, S_Symbol(fundec->name));
    mips_astFunction(fundec, global_types, function_ret_types, f);
    mips_astFunctions(l->next, global_types, function_ret_types, frames);
}

static void emitLabel(char * label, char * comment, ...) {
    static char buffer[1024];
    va_list argp;

    assert(comment != NULL && strcmp("",comment)); // Label must have comment
    assert(label != NULL);

    va_start(argp, comment);
    vsnprintf(buffer, sizeof(buffer), label, argp);
    va_end(argp);
    fprintf(out, "%s: # %s\n", buffer, comment);
    fflush(out);
}

static void emitInstruction(char * instruction, char * comment, ...) {
    static char buffer[1024];

    va_list argp;
    assert(comment != NULL && strcmp("",comment)); // Never have empty comment, even for labels
    assert(instruction != NULL);

    va_start(argp, comment);
    vsnprintf(buffer, sizeof(buffer), instruction, argp);
    va_end(argp);
    fprintf(out, "\t\t%s\t\t\t# %s\n", buffer, comment);
    fflush(out);
}

static void push0() {
    // Push register v0
    // $sp always points to top of stack (next unused position at top of stack)
    // Move $v0 to top of stack and adjust $sp to next unused position

    //        Rest of stack (high mem address)
    //        ------
    // $sp -> unused        (low  mem address), $v0 contents go here

    emitInstruction("sw $v0, ($sp) ", "PUSH0 - store $v0 on top of stack");
    // Substitution for : "subi $sp, $sp, 4"
    emitInstruction("addi $at, $zero, 4", "PUSH0 - adjust stack pointer by register size to pt to next unused position");
    emitInstruction("sub $sp, $sp, $at", "PUSH0 - adjust stack pointer by register size to pt to next unused position");
}

static void push1() {
    // Push register v1
    assert(0);
}

static void pop0() {
    // Pop to register v0
    emitInstruction("addi $sp, $sp, 4", "POP0 - adjust stack pointer by register size to pt to top stack value");
    emitInstruction("lw $v0, ($sp)", "POP0 - store $v0 on top of stack");
    emitInstruction("sw $zero, ($sp)", "POP0 - clear free space atop stack");
}

static void pop1() {
    // Pop to register v1
    emitInstruction("addi $sp, $sp, 4", "POP1 - adjust stack pointer by register size to pt to top stack value");
    emitInstruction("lw $v1, ($sp)", "POP1 - store $v0 on top of stack");
    emitInstruction("sw $zero, ($sp)", "POP1 - clear free space atop stack");
}

void mips_ast(program * p, S_table global_types, S_table function_rets, S_table frames, FILE * o) {
    out = o;
    UNUSED(generateFreshLabel);
    UNUSED(emitInstruction);
    UNUSED(emitLabel);
    UNUSED(push0);
    UNUSED(push1);
    UNUSED(pop0);
    UNUSED(pop1);

    mips_astVariables(p->variables, global_types, function_rets, NULL);
    mips_astFunctions(p->functions, global_types, function_rets, frames);
    mips_astStmts(p->statements, global_types, function_rets, NULL);
}
