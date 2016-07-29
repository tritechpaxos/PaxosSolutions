/******************************************************************************
*  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
*
*  See GPL-LICENSE.txt for copyright and license details.
*
*  This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License as published by the Free Software
* Foundation; either version 3 of the License, or (at your option) any later
* version. 
*
*  This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License along with
* this program. If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/******************************************************************************
 *
 *         
 *	rdb_lex.c
 *
 *	説明	検索条件解析用ｙａｃｃ／ｌｅｘ関数群
 * 
 ******************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include "y.tab.h"
#include "neo_system.h"

void	error();
int		more_punct();
int		more_str();
int		cl_literal();
int		read_token();

/*===================================================================
*	character handle table
*==================================================================*/
static int	char_tab[256];		/* character handle table */

#define	A_WHITE		0x00000001		/* space character	*/
#define	A_1CHAR		0x00000002		/* punctuate character	*/
#define	A_STR		0x00000004		/* start symbol character */
#define	A_NUMB		0x00000008		/* 0-9 */
#define	A_QUOTE		0x00000010		/* ' */

#define	A_MORE		0x00000100		/* double punct :=,>= etc */

#define	M_STR		0x00000200		/* symbol charcter	*/
#define	M_NUMB		0x00000400		/* 0-9_ */
#define	M_HEX		0x00000800		/* hexa character */

#define	is_white(c)	( char_tab[c] & A_WHITE )
#define	what_is(c)	( char_tab[c] & 0x00ff )
#define	is_beginname(c)	( char_tab[c] & A_STR )
#define	is_midname(c)	( char_tab[c] & M_STR )

/*===================================================================
*	unget character buffer
*	=================================================
*	|	yysbuf[YYLMAX]				|
*	=================================================
*		^ *yysptr ( next store pointer )
*
*	yytext return buffer
*	=================================================
*	|	text_buffer[YYTEXT_MAX]			 | 
*	=================================================
*		^ *yytext	^ inptr			^ text_buffer_end
*		token start	next store pointer
*	previous_yytext 0 yytext 0
*==================================================================*/
#define YYTEXT_MAX 512
#define YYLMAX 	256

static	char	*text_buffer, *inptr, *text_buffer_end;
static	char	yysbuf[YYLMAX];
static	char	*yysptr = yysbuf;

static	int	yytchar;			/* current character	*/
static	int	yyleng;				/* return buffer length */
static	int	through_flag;			/* not convert Capital	*/

static	char	*yytext;			/* return buffer pointer */
static	int	save_yytext = 1;		/* save yytext flag	*/

/*===================================================================
*	token buffer
*	=================================================
*	|	token_stack[TOKEN_MAX]			|
*	=================================================
*	 ->	^ token_idx 	^ token_max
*		next read	next store
*==================================================================*/

//static	int	yylineno = 1;
static	FILE	*yyin, *yyout;
/***
static	FILE	*yyin =stdin, *yyout =stdout;
***/
static	char	*strptr;

int (*cl_getc)();


/*===================================================================
*	functions
*==================================================================*/
int		(*input)();			/* getchar function */
void	(*unput)();			/* un_getchar function */

int
cl_getstr(in)
{
	register	int	c;
	if( (c = *strptr) )	strptr++;
	return(c);
}

int
cl_getfile(in)
FILE *in;
{
	register	int	c;

	if((c = getc(in)) < 0 )	c = 0;
	return(c);
}

int
input_nl()
/*===================================================================
*	機能
*		読みもどし文字があれば、それを
*		なければ入力ソースから一文字読みこむ
*		セーブ要求があれば（ｓａｖｅ＿ｔｅｘｔ）
*		ｔｅｘｔ＿ｂｕｆにセーブし、ＮＵＬＬで閉じる
*		行番号（ｌｅｘｌｌｉｎｅｎｏ）を管理する
*	関数値
*		読みこんだ文字
*		０　：＝　ＥＯＦ
*==================================================================*/
{
	register	int	c;

	if( yysptr > yysbuf )
		c	= *--yysptr;
	else 
		c = (*cl_getc)( yyin );

	if( save_yytext )
	{
		if( inptr < text_buffer_end )
		{
			*inptr++	= c;
			*inptr	= 0;
		}
		else if( inptr == text_buffer_end )
		{
			error("lex:text_buffer overflow\n");
		}
	}

	return( yytchar = (c & 0xff) );
}

void
unput_nl(c)
register int	c;
/*==============================================================
*	機能
*		ｙｓｂｕｆに１文字戻す
*		ｙｙｔｅｘｔも１文字消す
*		行番号（ｌｅｘｌｌｉｎｅｎｏ）を管理する
*	関数値
*		なし
*==============================================================*/
{
	*yysptr++ = yytchar = c;

	if( save_yytext ) {
		if( inptr <= text_buffer )
			error("lex:text_buffer underflow\n");
		else
			*--inptr	= 0;
	}
}

void
lex_start(str, bufp, size)
char *str, *bufp;
int	size;
{
	yyin =stdin ;
	yyout =stdout;
	strptr = str;
	cl_getc = cl_getstr;
	save_yytext = 1;
	yysptr = yysbuf;
	text_buffer = bufp;
	text_buffer_end = bufp + size;
	inptr = text_buffer;
}

void
lex_init()
/*===================================================================
*	機能
*		ＬＥＸのキャラクタテーブルを初期化する
*		キイワードハッシュテーブルをつくる
===================================================================*/
/*-------------------------------------------------------------------
*	function
*		initialize lex table
*	return value
*		NONE
*------------------------------------------------------------------*/
{
	register char	*p;
	static	char	*sabc	=  "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	static	char	*s123	=  "0123456789";
	static 	char	*shex	=  "XABCDEF";
	

	char_tab[' ']	|= A_WHITE;
	char_tab['\t']	|= A_WHITE;
	char_tab['\n']	|= A_WHITE;

	char_tab['\'']	|= A_QUOTE;

	p	= "=-()<>!|&~";
	while( *p )	char_tab[(int)*p++]	|= A_1CHAR;

	p	= "<>=!|&";
	while( *p )	char_tab[(int)*p++]	|= A_MORE;

	p	= s123;
	while( *p )
		char_tab[(int)*p++]	|= A_NUMB | M_STR | M_NUMB | M_HEX;
	p	= sabc;
	while( *p ){	
		char_tab[*p + 'a' - 'A']	|= A_STR | M_STR ;
		char_tab[(int)*p++]	|= A_STR | M_STR ;
	}
	p	= shex;
	while( *p ) {
		char_tab[*p + 'a' - 'A']	|= M_NUMB|M_HEX;
		char_tab[(int)*p++]	|= M_NUMB|M_HEX;
	}

	char_tab['_']	|= M_STR ;
	int	i;
	for( i = 128; i < 256; i++ ) {
		char_tab[i]	|= A_STR|M_STR;
	}

	input = input_nl;
	unput = unput_nl;
}

int
yylex()
{
	register int	c;
	c = read_token();
	return(c);
}

int
read_token()
/*==============================================================-
*	機能
*		一文字読み込み、字句解析を行い、トークン情報をつくる
*	関数値
*		トークン番号
*		0 : EOF
*==============================================================*/
/*-------------------------------------------------------------------
*	function
*		read character & make up token
*	return value
*		token
*		0 : EOF
*------------------------------------------------------------------*/
{
	register int	c,c1,lval;

	yytext	= ++inptr;


	for( ; (lval = c = (*input)()); inptr = yytext )
	{
		if( char_tab[c] & A_MORE )
		/*==========================
		* 最初の文字が複合特殊文字か、２，１６進文字（Ｂ，Ｈ）
		*=========================*/
		{
			c1	= (*input)();

			switch( what_is( c ) )
			{
				case A_1CHAR	:
			/*==========================
			* 最初の文字が複合特殊文字
			*=========================*/
					lval =  more_punct( c,c1 );
					yylval.i = lval;
					goto gotone;

				default	:
					(*unput)( c1 );
			}
		}
		switch( what_is( c ) )
		{
			case	A_WHITE	:	lval = 0;	break;
				
			case	A_QUOTE	:	lval = cl_literal();
						goto gotone;

			case	A_1CHAR	:	yylval.i = c;
						goto gotone;
				
			case	A_STR	:	lval = more_str( M_STR );
						goto gotone;
				
			case	A_NUMB	:	(*unput)( c );
						lval = more_str( M_NUMB );
						goto gotone;

			default		:
				error("lex:Unknown character %c\n",c);
				lval = 0;
				return( 0 );
		}

gotone:
		if( lval )	break;
	}
	/*==========================
	* lexval に値をセットする
	*=========================*/

	yyleng	= inptr - yytext;

	return( lval );
}

int
more_punct( c , c2 )
register int	c;	/* first character */
register int	c2;	/* second character */
/*===================================================================
*	機能
*		複合特殊文字トークンをつくる
*		コメントを読みとばす
*		複合文字でないとき二番目の文字ｃ２を読み戻す
*	関数値
*		トークン番号
===================================================================*/
/*-------------------------------------------------------------------
*	function
*		make up double punctuate token
*	return value
*		0 : COMMENT
*------------------------------------------------------------------*/
{
	register int	val;

	val	= 0;
	switch(c2)
	{
	case	'='	:
		switch( c )
		{
		case	'='	: val = SEQ;	break;
		case	'>'	: val = SGE;	break;
		case	'<'	: val = SLE;	break;
		case	'!'	: val = SNE;	break;
		default		: break;
		}
		break;
	case	'|'	:	if(c == '|')	val = SOR;
				break;
	case	'&'	:	if(c == '&')	val = SAND;
				break;

	default		:
		switch( c )
		{
		case	'>'	: val = SGT;	(*unput)(c2);	break;
		case	'<'	: val = SLT;	(*unput)(c2);	break;
		case	'='	: val = SEQ;	(*unput)(c2);	break;
		}
	}

	if( !val ){
		(*unput)( c2 );
		val	= c;
	}

	return( val );
}

int
more_str( mask )
int	mask;	/* B,H,M_NUMB,M_STR */
/*==============================================================-
*	機能
*		ｍａｓｋの値により継続すべき文字列を読みこみ
*		キーワード、シンボル、数値をつくる
*		数値定数のとき十進値をｌｅｘｖａｌのｖａｌにセットする
*		数字＋英字はエラーにし、英字を読み捨てる
*	関数値
*		トクン番号
*==============================================================*/
/*-------------------------------------------------------------------
*	function
*		make up KEYWORD SYMBOL INT_CONST BIT_CONST
*		ignore	bad continuous
*	return value
*		token
*------------------------------------------------------------------*/
{
	register	int	n,c;
	char		*p;
DBG_ENT( M_RDB, "read_token" );

	for(n = 0; char_tab[ c = (*input)()] & mask ; )
	{
		if(mask == M_STR || c == '_')	continue;

		if( c == 'X' || c == 'x' ) {
			mask = M_HEX;
			continue;
		}
		if( mask == M_NUMB )
			n *= 10;
		else if( mask == M_HEX )
		{
			n <<= 4;
			if( char_tab[c] & A_STR )
			{
				if( c < 'a' )
					n	+= 10 + c - 'A';
				else
					n	+= 10 + c - 'a';
				continue;
			}
		}

		n	+= c - '0';
	}
	(*unput)( c );

	/*========================================
	* 数字＋英字はエラーにし、英字を読み捨てる
	*=======================================*/
	if( char_tab[c] & M_STR )
	{
		p = ++inptr;
		for(; char_tab[ c = (*input)()] & M_STR ; );
		(*unput)( c );
		error("lex:bad constant %s\n",p);
	}

	/*========================================
	* シンボルか、キイワードか調べる
	*=======================================*/
	if( mask == M_NUMB || mask == M_HEX ){
		mask	= SNUM;
		yylval.i = n;
	}
	else if( mask == M_STR )	/* mask == M_STR */
	{
		mask = SNAME;
		yylval.s = yytext;
	}

	return( mask );
}

int
cl_literal()
/*===================================================================
*	機能
*		’でくくられた文字列を一切加工しないで文字定数とする
*	関数値
*		ＣＹ＿ＳＴＲ＿ＣＯＮＳＴ
===================================================================*/
/*-------------------------------------------------------------------
*	function
*		make up LITERAL
*	return value
*		CY_STR_CONST
*------------------------------------------------------------------*/
{
	register	int	c;
	

	through_flag	= 1;

	while( (c = (*input)()))
	{
		if(c == '\'' )
		{
			if((c = (*input)()) != '\'' ){
				break;
			}
		}
	}

	through_flag	= 0;

	if( islower( c ) )
		c	= toupper( c );
	(*unput)(c);
	*(inptr -1) = 0;

	yylval.s = yytext+1;
	return(SSTRING);

}

void
yyerror(s)
char	*s;
{
DBG_ENT( M_RDB, "yyerror" );
	DBG_A(("db_lex-1:yacc:[%s] yytext=[%s]\n",s,yytext));
}

void
error(s,a,b,c,d)
char	*s;
{
DBG_ENT( M_RDB, "error" );
	DBG_A((s,a,b,c,d));
}
