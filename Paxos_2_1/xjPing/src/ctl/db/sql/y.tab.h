/* A Bison parser, made by GNU Bison 3.0.4.  */

/* Bison interface for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

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

#ifndef YY_SQL_Y_TAB_H_INCLUDED
# define YY_SQL_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 1
#endif
#if YYDEBUG
extern int sqldebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TLEAF = 258,
    VITEM = 259,
    VITEM_ALLOC = 260,
    VSTRING = 261,
    VSTRING_ALLOC = 262,
    VBIN = 263,
    VINT = 264,
    VUINT = 265,
    VLONG = 266,
    VULONG = 267,
    VFLOAT = 268,
    VBOOL = 269,
    VPARAM = 270,
    SNAME = 271,
    SSTRING = 272,
    SNUM = 273,
    SFLOAT = 274,
    SOR = 275,
    SAND = 276,
    SLT = 277,
    SGT = 278,
    SLE = 279,
    SGE = 280,
    SEQ = 281,
    SNE = 282,
    STR_EQ = 283,
    STR_NE = 284,
    STR_GT = 285,
    STR_LT = 286,
    STR_GE = 287,
    STR_LE = 288,
    SCREATE = 289,
    SDROP = 290,
    STABLE = 291,
    SVIEW = 292,
    SAS = 293,
    SSELECT = 294,
    SFROM = 295,
    SWHERE = 296,
    SDELETE = 297,
    SINSERT = 298,
    SINTO = 299,
    SVALUES = 300,
    SUPDATE = 301,
    SSET = 302,
    SNODE = 303,
    SDROP_TABLE = 304,
    SDROP_VIEW = 305,
    SCREATE_TABLE = 306,
    SCREATE_VIEW = 307,
    SSTATEMENT = 308,
    SBEGIN = 309,
    SCOMMIT = 310,
    SROLLBACK = 311,
    SORDER = 312,
    SBY = 313,
    SASC = 314,
    SDESC = 315,
    SPRIMARY = 316,
    SKEY = 317,
    SUNION = 318,
    SALL = 319,
    SLIKE = 320,
    SNOT = 321,
    SNOTLIKE = 322,
    SESCAPE = 323,
    START_program = 324,
    START_pstatement = 325
  };
#endif
/* Tokens.  */
#define TLEAF 258
#define VITEM 259
#define VITEM_ALLOC 260
#define VSTRING 261
#define VSTRING_ALLOC 262
#define VBIN 263
#define VINT 264
#define VUINT 265
#define VLONG 266
#define VULONG 267
#define VFLOAT 268
#define VBOOL 269
#define VPARAM 270
#define SNAME 271
#define SSTRING 272
#define SNUM 273
#define SFLOAT 274
#define SOR 275
#define SAND 276
#define SLT 277
#define SGT 278
#define SLE 279
#define SGE 280
#define SEQ 281
#define SNE 282
#define STR_EQ 283
#define STR_NE 284
#define STR_GT 285
#define STR_LT 286
#define STR_GE 287
#define STR_LE 288
#define SCREATE 289
#define SDROP 290
#define STABLE 291
#define SVIEW 292
#define SAS 293
#define SSELECT 294
#define SFROM 295
#define SWHERE 296
#define SDELETE 297
#define SINSERT 298
#define SINTO 299
#define SVALUES 300
#define SUPDATE 301
#define SSET 302
#define SNODE 303
#define SDROP_TABLE 304
#define SDROP_VIEW 305
#define SCREATE_TABLE 306
#define SCREATE_VIEW 307
#define SSTATEMENT 308
#define SBEGIN 309
#define SCOMMIT 310
#define SROLLBACK 311
#define SORDER 312
#define SBY 313
#define SASC 314
#define SDESC 315
#define SPRIMARY 316
#define SKEY 317
#define SUNION 318
#define SALL 319
#define SLIKE 320
#define SNOT 321
#define SNOTLIKE 322
#define SESCAPE 323
#define START_program 324
#define START_pstatement 325

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 99 "sql_gram.expr" /* yacc.c:1909  */

    char        *s;
    int     i;
    long long   l;
    double      f;
    r_expr_t    *p;
    

#line 203 "y.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE sqllval;

int sqlparse (void);

#endif /* !YY_SQL_Y_TAB_H_INCLUDED  */
