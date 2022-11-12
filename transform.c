#define _POSIX_C_SOURCE 200809L // enable strdup
#include <string.h>

#include <stdio.h>
#include <assert.h>
#include "ast.h"
#include "transform.h"
#include "util.h"

//Temp include
#include <stdlib.h>

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
    switch(e->kind) {
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
    assert(s != NULL); // Same as commented text beneath. Is this Necessary?
    //if (s == NULL) {printf("S == NULL!\n"); exit(0);}
    if(s) {
        switch(s->kind){
            case assign_stmt: {
                break;
            }
            case if_stmt: {
                transformStmts(s->data.if_ops.then_stmts, globals_types, function_rets, f);
                transformStmts(s->data.if_ops.else_stmts, globals_types, function_rets, f);
                break;
            }
            case while_stmt: {
                if (s->data.while_ops.otherwise != NULL) {
                    list *next = l->next;
                    s->kind = if_stmt;
                    l->next = ListAddLast(IfNode(s->data.while_ops.cond, ListAddLast(
                                                         WhileNode(s->data.while_ops.cond, s->data.while_ops.body, NULL), s->data.while_ops.body),
                                                 s->data.while_ops.otherwise), NULL);// new ast node
                    l->next->next = next;
                }
                transformStmts(s->data.while_ops.body, globals_types, function_rets, f);
                break;
            }
            case repeat_stmt: {
                break;
            }
            case ret_stmt: {
                // if top level, (not in func), replace with intrinsic call "exit"
                if (f == NULL) {
                    exp_node * ret = s->data.ret_exp;
                    s->kind = intrinsic_stmt;
                    s->data.intrinsic_ops.name = "exit";
                    s->data.intrinsic_ops.args = ListAddFirst(ret, NULL);
                }
                break;
            }
            case call_stmt: {
                if (!strcmp("exit", s->data.call_ops.name)) {
                    list * args = s->data.call_ops.args;
                    char * name = s->data.call_ops.name; // name = "exit"
                    s->kind = intrinsic_stmt;
                    s->data.intrinsic_ops.name = name;
                    s->data.intrinsic_ops.args = args;
                }

                if (!strcmp("printint", s->data.call_ops.name)) {
                    list * args = s->data.call_ops.args;
                    char * name = s->data.call_ops.name; // name = "printint"
                    s->kind = intrinsic_stmt;
                    s->data.intrinsic_ops.name = name;
                    s->data.intrinsic_ops.args = args;
                }

                if (!strcmp("printstring", s->data.call_ops.name)) {
                    list * args = s->data.call_ops.args;
                    char * name = s->data.call_ops.name; // name = "printstring"
                    s->kind = intrinsic_stmt;
                    s->data.intrinsic_ops.name = name;
                    s->data.intrinsic_ops.args = args;
                }
                break;
            }
            default:
                printf("Stype: %d",s->kind);
                assert(0);
        }
    }

    transformStmts(l->next, globals_types, function_rets, f);
}

void transformVariable(vardec_node * node, S_table globals_types, S_table function_rets, frame * f, list ** glob_var_inits) {
    UNUSED(node); UNUSED(globals_types); UNUSED(function_rets); UNUSED(f);
    stmt_node * newNode = AssignNode(node->name, node->init);
    *glob_var_inits = ListAddLast(newNode, *glob_var_inits);
    //stmt_node * head = (*glob_var_inits)->head;
//    transformExpr(node->init, globals_types, function_rets, f);
}

void transformVariables(list * l, S_table global_types, S_table function_rets, frame * f, list ** glob_var_inits) {
    if (l == NULL) return;
    transformVariable((vardec_node *)l->head, global_types, function_rets, f, glob_var_inits);
    transformVariables(l->next, global_types, function_rets, f, glob_var_inits);
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

list * getLastElement(list * l) {
    while (l->next != NULL) {
        l = l->next;
    }
    return l;
}

void transform(program * p, S_table global_types, S_table function_rets, S_table frames) {

    // Store initialization data for global variables to be put first in .text section
    list * glob_var_inits = NULL; //ListAddLast(AssignNode("NULL", IntNode(0)), NULL); // List of stmt_node*
    stmt_node * last_glb_v_int = NULL;
    transformVariables(p->variables, global_types, function_rets, NULL, &glob_var_inits);
    UNUSED(last_glb_v_int);


    //p->statements = ListAddLast();
    if (glob_var_inits != NULL) {
        list * last = getLastElement(glob_var_inits);
        if (last != NULL) {
            last->next = p->statements;
            p->statements = glob_var_inits;
        }
    }

    transformFunctions(p->functions, global_types, function_rets, frames);
    transformStmts(p->statements, global_types, function_rets, NULL);
}
