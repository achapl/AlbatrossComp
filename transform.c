#define _POSIX_C_SOURCE 200809L // enable strdup
#include <string.h>

#include <stdio.h>
#include <assert.h>
#include "ast.h"
#include "transform.h"
#include "util.h"

extern program p;

static char * generateFreshGlobalVar() {
    static int lastVar = 0;
    static char buffer[1024];

    snprintf(buffer, sizeof(buffer), "_stringConstant%d", lastVar);
    lastVar += 1;
    return strdup(buffer);
}


void transformExpr(exp_node * e, S_table global_types, S_table function_rets, frame * f) {
    UNUSED(global_types);
    UNUSED(function_rets);
    UNUSED(f);
    if(!e) return;
    switch(e->kind){
        case int_exp: {
            break;
        }
        case string_exp: {
            // Create a fresh global variable of type string
            char * name = generateFreshGlobalVar();
            // Initialize that global variable with the contents of the string
            p.variables = ListAddFirst(
                    VarDecNode(name, StringTyNode(), StringNode(e->data.sval)),
                    p.variables);
            // Register the new global variable in the right table
            S_enter(global_types, S_Symbol(name), StringTyNode());
            // Replace the string_exp with a var_exp for that fresh global variable
            e->kind = var_exp;
            e->data.var_ops.name = name;
            break;
        }
        case binop_exp: {
            break;
        }
        case call_exp: {
            break;
        }
        case unop_exp: {
            break;
        }
        case var_exp: {
            break;
        }
        default:
            assert(0);
    }
}

void transformStmts(list * l, S_table globals_types, S_table function_rets, frame * f) {
    UNUSED(globals_types);
    UNUSED(function_rets);
    UNUSED(f);
    if (l == NULL) return;
    stmt_node * s = l->head;
    if(s) {
        switch(s->kind){
            case assign_stmt: {
                break;
            }
            case if_stmt: {
                break;
            }
            case while_stmt: {
                break;
            }
            case repeat_stmt: {
                break;
            }
            case ret_stmt: {
                break;
            }
            case call_stmt: {
                break;
            }
            default:
                assert(0);
        }
    }

    transformStmts(l->next, globals_types, function_rets, f);
}

void transformVariable(vardec_node * node, S_table globals_types, S_table function_rets, frame * f) {
    UNUSED(node); UNUSED(globals_types); UNUSED(function_rets); UNUSED(f);
//    transformExpr(node->init, globals_types, function_rets, f);
}

void transformVariables(list * l, S_table global_types, S_table function_rets, frame * f) {
    if (l == NULL) return;
    transformVariable((vardec_node *)l->head, global_types, function_rets, f);
    transformVariables(l->next, global_types, function_rets, f);
}

void transformFunction(fundec_node * fundec, S_table globals, S_table functions_rets, frame * f) {
    UNUSED(fundec);
    transformStmts(fundec->stmts, globals, functions_rets, f);
}

void transformFunctions(list * l, S_table global_types, S_table function_ret_types, S_table frames) {
    if (l == NULL) return;
    fundec_node * fundec = (fundec_node*) l->head;
    frame * f = S_look(frames, S_Symbol(fundec->name));
    transformFunction(fundec, global_types, function_ret_types, f);
    transformFunctions(l->next, global_types, function_ret_types, frames);
}

void transform(program * p, S_table global_types, S_table function_rets, S_table frames) {

    transformVariables(p->variables, global_types, function_rets, NULL);
    transformFunctions(p->functions, global_types, function_rets, frames);
    transformStmts(p->statements, global_types, function_rets, NULL);
}
