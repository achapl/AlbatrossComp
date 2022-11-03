/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    STRING = 258,                  /* STRING  */
    NAME = 259,                    /* NAME  */
    TYPE = 260,                    /* TYPE  */
    INT = 261,                     /* INT  */
    RETURN = 262,                  /* RETURN  */
    IF = 263,                      /* IF  */
    ELSE = 264,                    /* ELSE  */
    WHILE = 265,                   /* WHILE  */
    OTHERWISE = 266,               /* OTHERWISE  */
    REPEAT = 267,                  /* REPEAT  */
    SEMICOLON = 268,               /* SEMICOLON  */
    LPAREN = 269,                  /* LPAREN  */
    RPAREN = 270,                  /* RPAREN  */
    LCURL = 271,                   /* LCURL  */
    RCURL = 272,                   /* RCURL  */
    ASSIGN = 273,                  /* ASSIGN  */
    VAR = 274,                     /* VAR  */
    FUN = 275,                     /* FUN  */
    COMMA = 276,                   /* COMMA  */
    OP_OR = 277,                   /* OP_OR  */
    OP_AND = 278,                  /* OP_AND  */
    OP_BOR = 279,                  /* OP_BOR  */
    OP_XOR = 280,                  /* OP_XOR  */
    OP_BAND = 281,                 /* OP_BAND  */
    OP_EQ = 282,                   /* OP_EQ  */
    OP_NE = 283,                   /* OP_NE  */
    OP_LT = 284,                   /* OP_LT  */
    OP_GT = 285,                   /* OP_GT  */
    OP_LE = 286,                   /* OP_LE  */
    OP_GE = 287,                   /* OP_GE  */
    OP_PLUS = 288,                 /* OP_PLUS  */
    OP_MINUS = 289,                /* OP_MINUS  */
    OP_TIMES = 290,                /* OP_TIMES  */
    OP_DIV = 291,                  /* OP_DIV  */
    OP_REM = 292,                  /* OP_REM  */
    OP_NOT = 293                   /* OP_NOT  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif
/* Token kinds.  */
#define YYEMPTY -2
#define YYEOF 0
#define YYerror 256
#define YYUNDEF 257
#define STRING 258
#define NAME 259
#define TYPE 260
#define INT 261
#define RETURN 262
#define IF 263
#define ELSE 264
#define WHILE 265
#define OTHERWISE 266
#define REPEAT 267
#define SEMICOLON 268
#define LPAREN 269
#define RPAREN 270
#define LCURL 271
#define RCURL 272
#define ASSIGN 273
#define VAR 274
#define FUN 275
#define COMMA 276
#define OP_OR 277
#define OP_AND 278
#define OP_BOR 279
#define OP_XOR 280
#define OP_BAND 281
#define OP_EQ 282
#define OP_NE 283
#define OP_LT 284
#define OP_GT 285
#define OP_LE 286
#define OP_GE 287
#define OP_PLUS 288
#define OP_MINUS 289
#define OP_TIMES 290
#define OP_DIV 291
#define OP_REM 292
#define OP_NOT 293

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 22 "parser.y"

	int ival;
    char * sval;
    stmt_node * stmt;
    list * stmts;
    exp_node * exp;
    list * exps;
    list * vars;
    vardec_node * var;
    list * funcs;
    fundec_node * func;
    list * argdecls;
    param * argdecl;
	

#line 159 "y.tab.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;


int yyparse (void);


#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
