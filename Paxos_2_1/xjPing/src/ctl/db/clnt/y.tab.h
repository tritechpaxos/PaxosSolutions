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

#ifndef YY_YY_Y_TAB_H_INCLUDED
# define YY_YY_Y_TAB_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int yydebug;
#endif

/* Token type.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    TLEAF = 258,
    VITEM = 259,
    VSTRING = 260,
    VBIN = 261,
    VBOOL = 262,
    SNAME = 263,
    SSTRING = 264,
    SNUM = 265,
    SOR = 266,
    SAND = 267,
    SLT = 268,
    SGT = 269,
    SLE = 270,
    SGE = 271,
    SEQ = 272,
    SNE = 273,
    STR_EQ = 274,
    STR_NE = 275,
    STR_GT = 276,
    STR_LT = 277,
    STR_GE = 278,
    STR_LE = 279
  };
#endif
/* Tokens.  */
#define TLEAF 258
#define VITEM 259
#define VSTRING 260
#define VBIN 261
#define VBOOL 262
#define SNAME 263
#define SSTRING 264
#define SNUM 265
#define SOR 266
#define SAND 267
#define SLT 268
#define SGT 269
#define SLE 270
#define SGE 271
#define SEQ 272
#define SNE 273
#define STR_EQ 274
#define STR_NE 275
#define STR_GT 276
#define STR_LT 277
#define STR_GE 278
#define STR_LE 279

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED

union YYSTYPE
{
#line 34 "rdb_gram.expr" /* yacc.c:1909  */

	char	*s;
	long long	i;
	

#line 108 "y.tab.h" /* yacc.c:1909  */
};

typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif


extern YYSTYPE yylval;

int yyparse (void);

#endif /* !YY_YY_Y_TAB_H_INCLUDED  */
