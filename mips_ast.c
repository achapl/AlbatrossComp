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

static int currJump = 0;

static FILE * out;

// Push from register v0
static void push0();

// Push from register v1
static void push1();

// Pop to register v0
static void pop0();

// Pop to register v1
static void pop1();

char * kindToTypeExp(exp_node* s);

static void emitInstruction(char * instruction, char * comment, ...);

static void emitLabel(char * label, char * comment, ...);

void mips_astExpr(exp_node * e, S_table global_types, S_table function_rets, frame * f);

static void intrinsic(char * name, list * args, S_table global_types, S_table function_rets, frame * f) { // list of exp_node*

/*
    if (args != NULL) {
        list *l = args;

        while (l != NULL) {
            mips_astExpr(l->head, global_types, function_rets, f);
            l = l->next;
        }
    }*/

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

    if (!strcmp(name, "printstring")) {

        // Eval and push each arg
        mips_astExpr(args->head, global_types, function_rets, f);
        pop0();
        // TODO: Already MIPS'd the arg, but trying to pass the argument (a function) as a string
        // TODO: MIP ARGS BEFORE CALLING FUNC, PASS VALS INTO FUNCTION?
            // That is for non-intrinsic, implement for intrinsic as is done in non-intrinsic?
        exp_node * strToPrint = (exp_node *) args->head;
        //strToPrint->data.sval;
        //emitInstruction("","PRINTSTRING Move string address to stack");
        // POP and pass to system call
        //pop0(); // Pop to $v0
        // TODO: INVESTIGATE: ARG PASSED IN IS 'F', NAME OF FUNCTION, NOT NAME OF THE TRANSFORMED STRING
        emitInstruction("move $a0, $v0", "PRINTSTRING pass address of string to be printed", strToPrint->data.sval);
        emitInstruction("li $v0, 4", "PRINTSTRING specify printstring as the syscall to perform");
        emitInstruction("syscall", "PRINTSTRING perform syscall");
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
            //emitInstruction("lw $v0, %s", "astExpr - string_exp: Load string address into v0", e->data.sval);
            //push0();
            break;
        }
        case binop_exp: {
            // Recursively evaluate each side of binary operation
            // Place result on the stack
            if (e->data.bin_ops.op != and_op && e->data.bin_ops.op != or_op) {
                mips_astExpr(e->data.bin_ops.e1, global_types, function_rets, f);

                mips_astExpr(e->data.bin_ops.e2, global_types, function_rets, f);
                // store values on stack from recursive call into a register;
                pop1();
                pop0();
            }

            if (e->data.bin_ops.op == plus_op) {
                // Add the registers that correspond to each operand
                emitInstruction("add $v0, $v0, $v1","BINOP_EXP PLUS_OP - Add the two operands");
            } else if (e->data.bin_ops.op == minus_op) {
                emitInstruction("sub $v0, $v0, $v1","BINOP_EXP MINUS_OP - Subtract the two operands");
            } else if (e->data.bin_ops.op == times_op) {
                emitInstruction("mul $v0, $v0, $v1","BINOP_EXP TIMES_OP - Multiply the two operands");
            } else if (e->data.bin_ops.op == div_op) {
                emitInstruction("div $v0, $v0, $v1","BINOP_EXP DIV_OP - Divide the two operands");
            } else if (e->data.bin_ops.op == rem_op) {
                emitInstruction("div $v0, $v0, $v1","BINOP_EXP REM_OP - Divide the two operands");
                emitInstruction("mfhi $v0","BINOP_EXP REM_OP - Move remainder from register 'hi' to $v0");
            } else if (e->data.bin_ops.op == bor_op) {
                emitInstruction("or $v0, $v0, $v1","BINOP_EXP BOR_OP - Or the two operands");
            } else if (e->data.bin_ops.op == ban_op) {
                emitInstruction("and $v0, $v0, $v1","BINOP_EXP BAN_OP - And the two operands");
            } else if (e->data.bin_ops.op == xor_op) {
                emitInstruction("xor $v0, $v0, $v1",    "BINOP_EXP XOR_OP - And the two operands");
            }

            if (e->data.bin_ops.op == or_op) {
                int myCurrJump = currJump;
                currJump += 2;
                mips_astExpr(e->data.bin_ops.e1, global_types, function_rets, f);
                pop0();
                // Only go into both branches if equal to 0, since only 0 AND 0 will result in a 0
                emitInstruction("bne $v0, $zero, OR_TRUE%d",    "BINOP_EXP OR_OP - Check if $v1 is zero", myCurrJump);
                mips_astExpr(e->data.bin_ops.e2, global_types, function_rets, f);
                // store values on stack from recursive call into a register;
                pop1();
                emitInstruction("bne $v1, $zero, OR_TRUE%d",    "BINOP_EXP OR_OP - Check if $v1 is zero", myCurrJump);
                emitInstruction("li $v0, 0", "BINOP_EXP OR_OP - Or is false, both 0's");
                emitInstruction("j OR_END%d", "BINOP_EXP OR_OP - Done with or op", myCurrJump + 1);
                emitLabel("OR_TRUE%d", "BINOP_EXP OR_OP - Jump here if OR op is true", myCurrJump);
                emitInstruction("li $v0, 1", "BINOP_EXP OR_OP - Or is true, neither is a 0");
                emitLabel("OR_END%d", "BINOP_EXP OR_OP - Jump here if OR op is true", myCurrJump + 1);


            } else if (e->data.bin_ops.op == and_op) {
                int myCurrJump = currJump;
                currJump += 2;
                mips_astExpr(e->data.bin_ops.e1, global_types, function_rets, f);
                pop0();
                // Only go into both branches if equal to 1, since only 1 AND 1 will result in a 1
                emitInstruction("beq $v0, $zero, AND_FALSE%d",    "BINOP_EXP AND_OP - Check if $v1 is zero", myCurrJump);
                mips_astExpr(e->data.bin_ops.e2, global_types, function_rets, f);
                // store values on stack from recursive call into a register;
                pop1();
                emitInstruction("beq $v1, $zero, AND_FALSE%d",    "BINOP_EXP AND_OP - Check if $v1 is zero", myCurrJump);
                emitInstruction("li $v0, 1", "BINOP_EXP AND_OP - And is true, both 1's");
                emitInstruction("j AND_END%d", "BINOP_EXP AND_OP - Done with or op", myCurrJump + 1);
                emitLabel(          "AND_FALSE%d", "BINOP_EXP AND_OP - Jump here if OR op is true", myCurrJump);
                emitInstruction("li $v0, 0", "BINOP_EXP AND_OP - And is false, neither is a 1");
                emitLabel(          "AND_END%d", "BINOP_EXP AND_OP - Jump here if OR op is true", myCurrJump + 1);

            }
            else if (e->data.bin_ops.op == eq_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If eq_op evals to false, continue, and store 0 to $v0
                // if true jump to EQ_OP_BNE and store 0 to $v0
                emitInstruction("beq $v0, $v1, EQ_OP_BE%d","BINOP_EXP EQ_OP - Branch if equal", myCurrJump);
                emitInstruction("li $v0, 0",                "BINOP_EXP EQ_OP - Store 0 for false comparison");
                // Once done wit this, jump over the "false" branch to EQ_OP_END
                emitInstruction("j EQ_OP_END%d",              "BINOP_EXP EQ_OP - Done with equal expression if it is false. Go to end", myCurrJump+1);

                // If false, jump here, and store 0 to $v0
                emitLabel(          "EQ_OP_BE%d",              "BINOP_EXP EQ_OP - Branch to here if not equal", myCurrJump);
                emitInstruction("li $v0, 1",            "BINOP_EXP EQ_OP - Store one for true comparison");
                emitLabel(                "EQ_OP_END%d",          "BINOP_EXP EQ_OP - End from all branches here", myCurrJump+1);


            }
            else if (e->data.bin_ops.op == ne_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If ne_op evals to false, continue, and store 0 to $v0
                // if true jump to EQ_OP_BNE and store 1 to $v0
                emitInstruction("bne $v0, $v1, NE_OP_TRUE%d","BINOP_EXP NE_OP - Branch if equal", myCurrJump);
                emitInstruction("li $v0, 0",                "BINOP_EXP NE_OP - Store 0 for false comparison");
                // Once done wit this, jump over the "false" branch to EQ_OP_END
                emitInstruction("j NE_OP_END%d",              "BINOP_EXP NE_OP - Done with equal expression if it is false. Go to end", myCurrJump+1);

                // If false, jump here, and store 0 to $v0
                emitLabel(          "NE_OP_TRUE%d",              "BINOP_EXP NE_OP - Branch to here if not equal", myCurrJump);
                emitInstruction("li $v0, 1",            "BINOP_EXP NE_OP - Store one for true comparison");
                emitLabel(                "NE_OP_END%d",          "BINOP_EXP NE_OP - End from all branches here", myCurrJump+1);


            }
            else if (e->data.bin_ops.op == lt_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If lt_op evals to false, continue, and store 0 to $v0
                // if true branch to LT_OP_TRUE and store 1 to $v0
                emitInstruction("blt $v0, $v1, LT_OP_TRUE%d","BINOP_EXP LT_OP - Branch if LT", myCurrJump);
                emitInstruction("li $v0, 0",                 "BINOP_EXP LT_OP - Store 0 for false comparison");
                // Once done with this, jump over the "true" branch to LT_OP_END
                emitInstruction("j LT_OP_END%d",              "BINOP_EXP LT_OP - Done with LT expression if it is true, go to end", myCurrJump+1);

                // If true, jump here, and store 1 to $v0
                emitLabel(          "LT_OP_TRUE%d",              "BINOP_EXP LT_OP - Branch to here if LT", myCurrJump);
                emitInstruction("li $v0, 1",                 "BINOP_EXP LT_OP - Store 1 for true comparison");

                emitLabel(          "LT_OP_END%d",               "BINOP_EXP LT_OP - End from all branches here", myCurrJump+1);


            }
            else if (e->data.bin_ops.op == gt_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If gt_op evals to false, continue, and store 0 to $v0
                // if true branch to GT_OP_TRUE and store 1 to $v0
                emitInstruction("bgt $v0, $v1, GT_OP_TRUE%d","BINOP_EXP GT_OP - Branch if GT", myCurrJump);
                emitInstruction("li $v0, 0",                 "BINOP_EXP GT_OP - Store 0 for false comparison");
                // Once done with this, jump over the "true" branch to GT_OP_END
                emitInstruction("j GT_OP_END%d",              "BINOP_EXP GT_OP - Done with GT expression if it is true, go to end", myCurrJump+1);

                // If true, jump here, and store 1 to $v0
                emitLabel(          "GT_OP_TRUE%d",              "BINOP_EXP GT_OP - Branch to here if GT", myCurrJump);
                emitInstruction("li $v0, 1",                 "BINOP_EXP GT_OP - Store 1 for true comparison");

                emitLabel(          "GT_OP_END%d",               "BINOP_EXP GT_OP - End from all branches here", myCurrJump+1);


            }
            else if (e->data.bin_ops.op == le_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If le_op evals to false, continue, and store 0 to $v0
                // if true branch to LE_OP_TRUE and store 1 to $v0
                emitInstruction("ble $v0, $v1, LE_OP_TRUE%d","BINOP_EXP LE_OP - Branch if LE", myCurrJump);
                emitInstruction("li $v0, 0",                 "BINOP_EXP LE_OP - Store 0 for false comparison");
                // Once done with this, jump over the "true" branch to LE_OP_END
                emitInstruction("j LE_OP_END%d",              "BINOP_EXP LE_OP - Done with LE expression if it is true, go to end", myCurrJump+1);

                // If true, jump here, and store 1 to $v0
                emitLabel(          "LE_OP_TRUE%d",              "BINOP_EXP LE_OP - Branch to here if LE", myCurrJump);
                emitInstruction("li $v0, 1",                 "BINOP_EXP LE_OP - Store 1 for true comparison");

                emitLabel(          "LE_OP_END%d",               "BINOP_EXP LE_OP - End from all branches here", myCurrJump+1);

            }
            else if (e->data.bin_ops.op == ge_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // If lt_op evals to false, continue, and store 0 to $v0
                // if true branch to GE_OP_TRUE and store 1 to $v0
                emitInstruction("bge $v0, $v1, GE_OP_TRUE%d","BINOP_EXP GE_OP - Branch if GE", myCurrJump);
                emitInstruction("li $v0, 0",                 "BINOP_EXP GE_OP - Store 0 for false comparison");
                // Once done with this, jump over the "true" branch to GE_OP_END
                emitInstruction("j GE_OP_END%d",              "BINOP_EXP GE_OP - Done with GE expression if it is true, go to end", myCurrJump+1);

                // If true, jump here, and store 1 to $v0
                emitLabel(          "GE_OP_TRUE%d",              "BINOP_EXP GE_OP - Branch to here if GE", myCurrJump);
                emitInstruction("li $v0, 1",                 "BINOP_EXP GE_OP - Store 1 for true comparison");

                emitLabel(          "GE_OP_END%d",               "BINOP_EXP GE_OP - End from all branches here", myCurrJump+1);

            }

            // push result to top of stack
            push0();
            break;
        }
        case call_exp: {
            // Push curr frame pointer
            emitInstruction("move $v0, $fp", "Func Call - Make frame, load $fp to push curr frame pointer", e->data.call_ops.name);
            push0();
            // Eval and push each arg
            int numArgs = 0;
            list * l = e->data.call_ops.args;
            while(l != NULL) {
                mips_astExpr(l->head, global_types, function_rets, f);
                // Note: leaving these on the stack automatically builds arguments
                numArgs+=1;
                l = l->next;
            }

            // Move frame pointer to stack pointer
            emitInstruction("move $fp, $sp", "Func Call - Make frame, move frame pointer to stack pointer");

            // adjust $fp based on current stack pointer and # args on the stack
            emitInstruction("add $fp, $fp, %d", "Func Call - Make frame, Adjust frame ptr to end of args", 4*(numArgs));
            //push0();

            // jal to function
            emitInstruction("jal %s", "Jump and link to callee function", e->data.call_ops.name);

            break;
        }
        case unop_exp: {
            // Recursively evaluate each side of unnary operation
            // Place result on the stack
            mips_astExpr(e->data.un_ops.e, global_types, function_rets, f);
            // store values on stack from recursive call into a register;
            pop0();

            if (e->data.un_ops.op == not_op) {
                int myCurrJump = currJump;
                currJump += 2;
                // Substitute A NOR $0 for NOT A
                // Branch if NOT exp is true (i.e !0 = 1) and then set $v0 to 1 (true)
                // Else, if not jumping, set $v0 to 0 (false)
                emitInstruction("beq $v0, $zero, NOTEXP_TRUE%d","UNOP_EXP NOT_OP - Branch if NOT exp is true (i.e !0 = 1)", myCurrJump);
                emitInstruction("li $v0, 0",                 "UNOP_EXP NOT_OP - Store 0 for false comparison");
                // Once done with this, jump over the "true" branch to NOT_OP_END
                emitInstruction("j NOT_OP_END%d",              "UNOP_EXP NOT_OP - Done with GE expression if it is true, go to end", myCurrJump+1);

                // If true, jump here, and store 1 to $v0
                emitLabel(          "NOTEXP_TRUE%d",              "UNOP_EXP NOT_OP - Branch to here if GE", myCurrJump);
                emitInstruction("li $v0, 1",                 "UNOP_EXP NOT_OP - Store 1 for true comparison");

                emitLabel(          "NOT_OP_END%d",               "UNOP_EXP NOT_OP - End from all branches here", myCurrJump+1);

            }

            push0();
            break;
        }
        case var_exp: {

            char * varName = e->data.var_ops.name;
            S_symbol varSymb = S_Symbol(varName);
            S_table locArgs;
            if (f != NULL) {
                locArgs = f->args_locs_types;
            }
            // Check if var is not global (Note, will never have a global scope with local vars, f != NULL stops segfaults later)
            if (f != NULL && S_look(global_types, varSymb) == NULL) {
                // Check var is local
                if (S_look(locArgs, varSymb) != NULL) {
                    long index = (long) S_look(f->indexes, S_Symbol(varName));

                    // Check if var is an int
                    if (!strcmp("int", typeToStr(S_look(locArgs, varSymb)))) {
                        emitInstruction("lw $v0, %d($fp)", "VAR_EXP - Load variable value", -4*(index));
                    }
                        // Check if var is a string
                    else if (!strcmp("string",typeToStr(S_look(locArgs, varSymb)))) {
                        emitInstruction("lw $v0, %d($fp)", "VAR_EXP - Load variable value", -4*(index));
                    }
                    else {assert(0);}
                    push0();
                    break;

                }
                break;
            }
            // Check if var is an int
            if (!strcmp("int", typeToStr(S_look(global_types, S_Symbol(e->data.var_ops.name))))) {
                emitInstruction("lw $v0, %s", "VAR_EXP - Load variable value", e->data.var_ops.name);
            }
            // Check if var is a string
            else if (!strcmp("string",typeToStr(S_look(global_types, S_Symbol(e->data.var_ops.name))))) {
                emitInstruction("la $v0, %s", "VAR_EXP - Load variable value", e->data.var_ops.name);
            }
            else {assert(0);}
            push0();
            break;
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

char * kindToTypeStmt(stmt_node* s) {
    switch(s->kind) {
        case assign_stmt: return "assign_stmt";
        case if_stmt: return "if_stmt";
        case while_stmt: return"while_stmt";
        case repeat_stmt: return "repeat_stmt";
        case call_stmt: return "call_stmt";
        case intrinsic_stmt: return "intrinsic_stmt";
        default: return "None";
    }
}
char * kindToTypeExp(exp_node* s) {
    switch(s->kind) {
        case int_exp: return "int_exp";
        case string_exp: return "string_exp";
        case binop_exp: return"binop_exp";
        case call_exp: return "call_exp";
        case unop_exp: return "unop_exp";
        case var_exp: return "var_exp";
        case intrinsic_exp: return "intrinsic_exp";
        default: return "None";
    }
}

void mips_astStmt(stmt_node * s, S_table global_types, S_table function_rets, frame * f) {
    UNUSED(global_types);
    UNUSED(function_rets);
    UNUSED(f);

    if(!s) return;
    switch(s->kind){
        case assign_stmt: {
            if (s->data.assign_ops.rhs->kind != string_exp) {
                mips_astExpr(s->data.assign_ops.rhs, global_types, function_rets, f);
                pop0();
                emitInstruction("sw $v0, %s", "ASSIGN_STMT - store $v0 on top of stack", s->data.assign_ops.lhs);
            }
            break;
        }
        case if_stmt: {
            int jumpLabelNum = currJump;
            currJump += 2;
            // Do mips on condition
            mips_astExpr(s->data.if_ops.cond, global_types, function_rets, f);
            // Get result of condition
            pop0();
            emitInstruction("beq $v0, $zero, JFALSE%d", "IF_STMT - Jump to to JFALSE if 'if' is false", jumpLabelNum);
            //TODO: Deal with label and jumping/branching

            // Do mips on statements inside of if
            mips_astStmts(s->data.if_ops.then_stmts, global_types, function_rets, f);
            // Label for skipping last if
            emitInstruction("j JEND%d", "Jump to end when done with true branch", jumpLabelNum+1);
            emitLabel("JFALSE%d", "Jump to here if 'if' is false", jumpLabelNum);

            // "Else" part
            if (s->data.if_ops.else_stmts != NULL) {
                mips_astStmts(s->data.if_ops.else_stmts, global_types, function_rets, f);
                emitInstruction("j JEND%d", "Jump to end when done with true branch", jumpLabelNum+1);
            }
            emitLabel("JEND%d", "Jump to here if 'if' is false", jumpLabelNum+1);
            break;
        }
        case while_stmt: {
            int whileJump = currJump;
            currJump += 2;
            emitLabel("WHILE_START%d", "WHILE - Start of while loop", whileJump);
            mips_astExpr(s->data.while_ops.cond, global_types, function_rets, f);
            pop0();
            emitInstruction("li $v1, 0", "WHILE_STMT - Get 1 (true) to compare to");
            emitInstruction("beq $v0, $v1, WHILE_END%d", "WHILE_STMT - Test condition", whileJump+1);
            mips_astStmts(s->data.while_ops.body, global_types, function_rets, f);
            emitInstruction("j WHILE_START%d", "WHILE_STMT - Loop back to top of while loop", whileJump);
            emitLabel("WHILE_END%d", " WHILE_STMT - End of loop", whileJump+1);

            break;
        }
        case repeat_stmt: {
            assert(0);
        }
        case ret_stmt: {
            /* Returning - Pop ret val into register v0 (v1 for temp?)
             *      Pop return address to $ra
             *      Set stack pointer to frame pointer
             *      Set frame pointer to the value loading 4 after frame pointer (address of prev frame pointer) lw $fp, 4($fp)
             *      push return value (v0/v1)
             *      Jump back to caller
             *      pop $v0
             *      pop $ra
             *      move $sp, $fp
             *      lw $fp, 4($fp)
             *      jr $ra
             */
            mips_astExpr(s->data.ret_exp, global_types, function_rets, f);
            // Check if void function
            //if (f != NULL) {
                if (f != NULL && strcmp(typeToStr(f->ret), "void")) {
                    // Pop ret val to $v1
                    pop1();
                }

                // Pop return address to $ra
                pop0();
                emitInstruction("move $ra, $v0", "Function - Return, pop return address to $ra");
                // Set stack pointer to frame pointer
                emitInstruction("move $sp, $fp", "Function - Return, set stack pointer to frame ptr");

                // Set frame pointer to the value loading 4 after frame pointer (address of prev frame pointer) lw $fp, 4($fp)
                pop0(); // Get rid of callee frame pointer
                // emitInstruction("lw $v0, -4($fp)", "Function - Return, Set $fp to prev frame ptr 4 after curr fp");
                emitInstruction("move $fp, $v0", "Function - Return, Set $fp to prev frame ptr 4 after curr fp");
                if (f != NULL && strcmp(typeToStr(f->ret), "void")) {
                    // push return value (v0/v1)
                    push1(); // put back onto stack now that frame is destroyed
                }
                // Jump back to caller
                emitInstruction("jr $ra", "Function - Return, final jump");
            //}
            // Else, it is a return from global scope
            //else {
                // Pop evaluation of return expression
            //    pop0();
            //}
            break;
        }
        case call_stmt: {
            // Lec 18: Stack Frames - 1:01:45
            /* Stack pointer - End of current frame
             * Frame pointer - End of last frame
             * Creating - Caller-Side
             *      Push curr frame-pointer
             *      Eval and push each arg
             *      adjust $fp based on current stack pointer and # args on the stack
             *      jal to function
             * Creating - Callee
             *      Make room for local vars (right after args, but before $ra)
             *      Initialize local vars
             *      Push $ra (points to return in the source MIPS code)
             * Returning - Pop ret val into register v0 (v1 for temp?)
             *      Pop return address to $ra
             *      Set stack pointer to frame pointer
             *      Set frame pointer to the value loading 4 after frame pointer (address of prev frame pointer) lw $fp, 4($fp)
             *      push return value (v0/v1)
             *      Jump back to caller
             *      pop $v0
             *      pop $ra
             *      move $sp, $fp
             *      lw $fp, 4($fp)
             *      jr $ra
            */

            /* ----------  Stack Frame Start
             * - Prev Frame Pointer value/address-pointed-to
             * - Args
             * (step) Set new fp rel to stack pointer (n)*4 + $sp
             * -$ra (pushed directly after jal instruction), return at end of function by popping stack pointer
             * - Return val
             * ----------  Stack Frame Start
             * -$fp next push old frame pointer
             */

            // Push curr frame pointer
            emitInstruction("move $v0, $fp", "Func Call - Make frame, load $fp to push curr frame pointer", s->data.call_ops.name);
            push0();
            // Eval and push each arg
            int numArgs = 0;
            list * l = s->data.call_ops.args;
            while(l != NULL) {
                mips_astExpr(l->head, global_types, function_rets, f);
                // Note: leaving these on the stack automatically builds arguments
                numArgs+=1;
                l = l->next;
            }

            // Move frame pointer to stack pointer
            emitInstruction("move $fp, $sp", "Func Call - Make frame, move frame pointer to stack pointer");

            // adjust $fp based on current stack pointer and # args on the stack
            emitInstruction("add $fp, $fp, %d", "Func Call - Make frame, Adjust frame ptr to end of args", 4*(numArgs));
            //push0();

            // jal to function
            emitInstruction("jal %s", "Jump and link to callee function", s->data.call_ops.name);
            break;
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
    UNUSED(globals_types);
    UNUSED(function_rets);
    // If scope is global
    if (f == NULL) {
        // Check that it is already in global scope
        char *ty = typeToStr(node->type);
        char *size;
        assert(ty != NULL);
        if (strcmp(ty, "int") == 0) {
            size = "word";
        } else if (strcmp(ty, "string") == 0) {
            size = "asciiz";
        }

        if (strcmp(ty, "int") == 0) {
            // mips_astExpr(node->init, globals_types, function_rets, f);
            // pop0();
            emitInstruction("%s:    .%s     %d", "Variable declaration for var %s", node->name, size, node->init->data.ival);
        } else if (strcmp(ty, "string") == 0) {
            emitInstruction("%s:    .%s     %s", "Variable declaration for var %s", node->name, size, node->init->data.sval);
        }
    }


}

void mips_astVariables(list * l, S_table global_types, S_table function_rets, frame * f) {
    if (l == NULL) return;

    mips_astVariable((vardec_node *)l->head, global_types, function_rets, f);
    mips_astVariables(l->next, global_types, function_rets, f);
}

void mips_astFunction(fundec_node * fundec, S_table globals, S_table functions_rets, frame * f) {
    UNUSED(fundec);
    // Lec 18: Stack Frames - 1:01:45
    /* Stack pointer - End of current frame
     * Frame pointer - End of last frame
     * Creating - Caller-Side
     *      Push curr frame-pointer
     *      Eval and push each arg
     *      adjust $fp based on current stack pointer and # args on the stack
     *      jal to function
     * Creating - Callee
     *      Make room for local vars (right after args, but before $ra)
     *      Initialize local vars
     *      Push $ra (points to return in the source MIPS code)
     * Returning - Pop ret val into register v0 (v1 for temp?)
     *      Pop return address to $ra
     *      Set stack pointer to frame pointer
     *      Set frame pointer to the value loading 4 after frame pointer (address of prev frame pointer) lw $fp, 4($fp)
     *      push return value (v0/v1)
     *      Jump back to caller
     *      pop $v0
     *      pop $ra
     *      move $sp, $fp
     *      lw $fp, 4($fp)
     *      jr $ra
    */

    // Make room for local vars (right after args, but before $ra)

    emitLabel("%s", "Func Name", fundec->name);
    list * l = fundec->locs;

    while(l != NULL) {
        vardec_node * loc = l->head;
        // Initialize local vars
        mips_astExpr(loc->init,globals, functions_rets, f);
        l = l->next;
    }

    // Push $ra (points to return in the source MIPS code)
    emitInstruction("move $v0, $ra", "Function Callee - make frame, push ret address to stack");
    push0();

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
    emitInstruction("sw $v1, ($sp) ", "PUSH0 - store $v0 on top of stack");
    // Substitution for : "subi $sp, $sp, 4"
    emitInstruction("addi $at, $zero, 4", "PUSH0 - adjust stack pointer by register size to pt to next unused position");
    emitInstruction("sub $sp, $sp, $at", "PUSH0 - adjust stack pointer by register size to pt to next unused position");
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

    if (p->variables != NULL) {
        fprintf(out, ".data\n");
    }
    list * t = p->variables;
    while(t != NULL) {
        vardec_node * statementNode = t->head;
        UNUSED(statementNode);
        t = t->next;
    }
    mips_astVariables(p->variables, global_types, function_rets, NULL);

    if (p->statements != NULL) {
        fprintf(out, ".text\n");
    }

    mips_astStmts(p->statements, global_types, function_rets, NULL);

    if (p->functions != NULL) {
        mips_astFunctions(p->functions, global_types, function_rets, frames);
    }
}
