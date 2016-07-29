#!/bin/sh

#########################################################################
#									#
#									#
#	neo_class.c							#
#									#
#	説明	ＮＥＯのクラスに対応するシンボルテーブル作成用の	#
#		ソースファイルを生成する。				#
#									#
#									#
#########################################################################

##############
# 各種　定義 #
##############

CREATE_FILE="neo_class.c"

#########################################################################
#									#
#　関数名	make_head_of_file					#
#									#
#　機能		出力ファイルのヘッダー部を生成する。			#
#									#
#　関数値	なし							#
#									#
#########################################################################

make_head_of_file()
{
	cat >> $CREATE_FILE << END_OF_LINE

/******************************************************************************
 *
 *	NEO -  Node integrated Engineering Operation system
 *         
 *	neo_class.c
 *
 *	説明	ＮＥＯのクラスに対応するシンボルテーブル
 * 
 ******************************************************************************/
#ifndef lint
static char     sccsid[] = "@(#) neo_class.c       1.0     93/10/18";
#endif

#include "neo_class.h"

/************/
/* テーブル */
/************/

neo_class_sym_t	_neo_class_tbl[] = {

END_OF_LINE
}

#########################################################################
#									#
#　関数名	make_table						#
#									#
#　機能		クラステーブルを作成する。				#
#									#
#　引数		＄１　：　クラスヘッダー				#
#									#
#　関数値	なし							#
#									#
#########################################################################

make_table()
{
	grep "^#define" $1 | grep -v "_NEO_CLASS_" | awk '{ printf "\t{  %s,\t\"%s\"  },\t%s %s %s\n", $2, $2, $4, $5, $6 }' >> $CREATE_FILE
}

#########################################################################
#									#
#　関数名	make_tail_of_file					#
#									#
#　機能		出力ファイルのテイル部を生成する。			#
#									#
#　関数値	なし							#
#									#
#########################################################################

make_tail_of_file()
{
	cat >> $CREATE_FILE << END_OF_LINE
	{  0,		(char*)0  }
};
END_OF_LINE
}

#########################################################################
#									#
#　関数名	main							#
#									#
#　機能		メイン　ルーチン					#
#									#
#　関数値	０　：　正常終了					#
#		１　：　異常終了					#
#									#
#########################################################################

main()
{
	rm $CREATE_FILE 2> /dev/null
	make_head_of_file
	make_table $1
	make_tail_of_file
}

main $1
