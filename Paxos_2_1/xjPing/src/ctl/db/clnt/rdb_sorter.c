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
 *	rdb_sorter.c
 *
 *	説明	R_CMD_SORT
 *			 	 テーブルをソートするアルゴリズム
 *
 *	(注)  ＡＰには開放されない
 *
 ******************************************************************************/

#include	"neo_db.h"

static int	db_compare(), db_down(), db_up(), db_sdown(), db_sup(), 
		db_fdown(), db_fup();
static int	db_ldown(), db_lup();
static int	db_udown(), db_uup();
static int	db_uldown(), db_ulup();

/* ソート分析器構造体 */
typedef	struct	r_sorter {
	int	s_offset;	/* フィールドのレコード内のオフセット地値 */
	int	s_length;	/* フィールドの有効バイト長		  */
	int	(*s_comp)();	/* 該当フィールドを対応するソート関数	  */
} r_sorter_t;

static int		sorters;
static r_sorter_t	*sorter;

/*****************************************************************************
 *
 *  関数名	_rdb_sort_open()
 *
 *  機能	ソート条件解析
 * 
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
_rdb_sort_open( td, n, sort )
	r_tbl_t		*td;	 	/* ソート対象テーブル記述子 */
	int		n;		/* ソートキーの数	    */
	r_sort_t	*sort;		/* ソートキー		    */
{
	int		i, j;
	int		n_item;
	r_item_t	*item;
	int		found;
	int		err;
DBG_ENT(M_RDB,"_rdb_sanal");

	/*
	 * 各キー用のソート分析器エリアを用意する
	 */
	sorters	= n;
	if( !(sorter = (r_sorter_t *)neo_malloc(sorters*sizeof(r_sorter_t))) ) {
		err = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * 上記生成した各キーの分析器に内容を設定する
	 */
	n_item	= td->t_scm->s_n;
	for( i = 0; i < n; i++ ) {

		/*
		 * キーと一致するフィールドを捜し出して、該当フィールド
		 * の属性より、キーのソート分析器を設定する
		 */
		found = 0;
		for( j = 0; j < n_item; j++ ) {
			item = &td->t_scm->s_item[j];
			if( !(strncmp( item->i_name, sort[i].s_name,
						sizeof(r_item_name_t)) ) ) {

				/*
				 * 有効バイト長とオフセット値設定
				 */
				sorter[i].s_offset	= item->i_off;
				sorter[i].s_length	= item->i_len;

				/*
				 */
				if ( item->i_attr == R_INT ) { 

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_down;
					else
						sorter[i].s_comp = db_up;

				}

				else if ( item->i_attr == R_HEX 
				  || item->i_attr == R_UINT ) {

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_udown;
					else
						sorter[i].s_comp = db_uup;

				}

				 else if ( item->i_attr == R_LONG ) {

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_ldown;
					else
						sorter[i].s_comp = db_lup;

				} 

				 else if ( item->i_attr == R_ULONG ) {

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_uldown;
					else
						sorter[i].s_comp = db_ulup;

				} 

				/*
				 * 実数の場合には、db_fdown あるいは
				 * db_fup ソート関数を用いる
				 */
				 else if ( item->i_attr == R_FLOAT ) {

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_fdown;
					else
						sorter[i].s_comp = db_fup;

				} 

				/*
				 * 文字列の場合には db_sdown あるいは
				 * db_supソート関数を用いる
				 */
				else if ( item->i_attr == R_STR ) {

					if( sort[i].s_order == R_DESCENDANT )
						sorter[i].s_comp = db_sdown;
					else
						sorter[i].s_comp = db_sup;
				}
				else if ( item->i_attr == R_BYTES ) {
                    //ignore R_BYTES sort
				}

				found = 1;
				goto next;
			}
		}
next:
		
		/*
		 * キーと一致するフィールド無し、ソートキー指定エラー
		 */
		if ( !found ) {
			err = E_RDB_NOITEM;
			goto err2;
		}
	}

DBG_EXIT(0);
	return( 0 );

err2: DBG_T("err2");
	neo_free( sorter );
	sorter = 0;
err1:
DBG_EXIT(err);
	return( err );

}


/*****************************************************************************
 *
 *  関数名	_rdb_sort_close()
 *
 *  機能	ソートの後処理
 * 
 *  関数値
 *     		0
 *
 ******************************************************************************/
int
_rdb_sort_close()
{
DBG_ENT(M_RDB,"_rdb_sort_close");

	/*
	 * キーのソート分析器エリアを解放する
	 */
	neo_free( sorter );
	sorter = 0;

DBG_EXIT(0);
	return( 0 );
}


/*****************************************************************************
 *
 *  関数名	_rdb_sort_start()
 *
 *  機能	ソートを実施する
 * 
 *  関数値	0
 *
 ******************************************************************************/
int
_rdb_sort_start( start, num )
	r_sort_index_t	*start;		/* ソート用 Index エリア */
	int		num;		/* ソート対象レコード数	 */
{
DBG_ENT(M_RDB,"_rdb_sort_start");

	/*
	 * システムライブラリ関数qsort
	 * (クイックソート)にてソートをする
	 */
	qsort( start, num, sizeof(r_sort_index_t), db_compare );
DBG_EXIT(0);
	return( 0 );
}


/*****************************************************************************
 *
 *  関数名	_rdb_arrange_data()
 *
 *  機能	ソート後、実際メモリ上のレコードの位置を調整する
 * 		(ソートは、Index テーブルに対して行ったので)
 *
 *  関数値
 *     		正常	0
 *		異常	エラー番号
 *
 ******************************************************************************/
int
_rdb_arrange_data( abs, rel, n, size )
	r_sort_index_t	*abs,		/* ソート前 Index 情報	*/
			*rel;		/* ソート後 Index 情報	*/
	int		n;		/* レコード数		*/
	int		size;		/* レコードサイズ	*/
{
	int		i, j, k, l;
	int		is;
	r_head_t	*wp;
	int		err;
	int		reln, absn;

DBG_ENT(M_RDB,"_rdb_arrange_data");

	/*
	 * 一レコードサイズ分の臨時エリアを用意する
	 */
	if( !(wp = (r_head_t *)neo_malloc( size )) ) {
		err = E_RDB_NOMEM;
		goto err1;
	}

	/*
	 * 全レコード分をチェックする
	 */
	for( i = 0; i < n; i++ ) {

		/*
		 * ソート後レコードの位置が変更していない、あるいは
		 * 前のループで既に適当な所に移したの場合には、処理しない
		 */
		if( rel[i].i_id == i )
			continue;

		/*
		 * 移動は、各循環リスト内で行って、is を循環リストの
		 * 先頭番号として、先頭レコードを臨時エリアに退避させる
		 * [循環リスト(1,3,6)とは、1を3で書き換え(1,3), (3,6)
		 *  (6,1)という置換リスト]
		 */ 
		is = j = i;

		bcopy( abs[is].i_head, wp, size );

		for( k = 0; k < n; k++ )  {

			/*
			 * レコードを書き換え、但し、レコードの絶対、相対
			 * 番号は元レコードのとする
			 */
			reln	= abs[j].i_head->r_rel;
			absn	= abs[j].i_head->r_abs;

			bcopy( rel[j].i_head, abs[j].i_head, size );

			abs[j].i_head->r_rel	= reln;
			abs[j].i_head->r_abs	= absn;

			/*
			 * 循環リスト内次の番号を洗い出す
			 */
			l	= rel[j].i_id;	/* next */

			rel[j].i_id	= j;

			j	= l;

			/*
			 * 循環リスト内最後のレコードだったら、
			 * 処理がちょっと違う
			 */
			if( rel[j].i_id == is )
				goto term;
		}
DBG_A(("SORTER ERROR\n"));
		err = E_RDB_CONFUSED;
		goto err1;
	term:
		/*
		 * 循環リスト内最後のレコードの場合には、
		 * 前臨時エリアに退避させた循環リスト内先頭レコードで
		 * 書き換える
		 */
		reln	= abs[j].i_head->r_rel;
		absn	= abs[j].i_head->r_abs;

		bcopy( wp, abs[j].i_head, size );

		abs[j].i_head->r_rel	= reln;
		abs[j].i_head->r_abs	= absn;

		rel[j].i_id		= j;
	}

	/*
	 * 臨時エリアを解放する
	 */
	neo_free( wp );
DBG_EXIT(0);
	return( 0 );
err1:
DBG_EXIT(err);
	return( err );
}


/*****************************************************************************
 *
 *  関数名	db_compare()
 *
 *  機能	qsort()からコールされる比較関数
 * 
 *  関数値
 *     		ゼロ未満整数
 *		ゼロ
 *		ゼロ異常整数
 *
 ******************************************************************************/
static int
db_compare( rec1, rec2 )
	r_sort_index_t	*rec1;		/* 比較するレコード１	*/
	r_sort_index_t	*rec2;		/* 比較するレコード２	*/
{
	register int	i, off;
	int	ret;

	/*
	 * 比較原則として、キーの並べ順によって、あるキーで二つ
	 * レコードの大小関係ができるまで、各キーずつ比較する
	 */
	ret	= 0;
	for( i = 0; i< sorters; i++ ) {
		off	= sorter[i].s_offset;
		if( ( ret = (*sorter[i].s_comp)( (char*)rec1->i_head+off, 
			      (char*)rec2->i_head+off, sorter[i].s_length ) ) ){
			break;
		}
	}

	return( ret );
}


/*****************************************************************************
 *
 *  関数名	db_up()
 *
 *  機能	整数項目についての昇順評価関数
 * 
 *  関数値
 *     		負整数
 *		0
 *		正整数
 *
 ******************************************************************************/
static int
db_up( data1, data2, len ) 
	register int	*data1, *data2, len;
{
	return( *data1 - *data2 );
}

static int
db_uup( data1, data2, len ) 
	register unsigned int	*data1, *data2, len;
{
	return( *data1 - *data2 );
}

static int
db_lup( data1, data2, len ) 
	register long long	*data1, *data2;
	register int		len;
{
	if( *data1 == *data2 )		return( 0 );
	else if( *data1 > *data2 )	return( 1 );
	else				return( -1 );
}

static int
db_ulup( data1, data2, len ) 
	register unsigned long long	*data1, *data2;
	register int		len;
{
	if( *data1 == *data2 )		return( 0 );
	else if( *data1 > *data2 )	return( 1 );
	else				return( -1 );
}

/*****************************************************************************
 *
 *  関数名	db_down()
 *
 *  機能	整数項目についての降順評価関数
 * 
 *  関数値
 *     		負整数
 *		0
 *		正整数
 *
 ******************************************************************************/
static int
db_down( data1, data2, len ) 
	register int	*data1, *data2, len;
{
	return( *data2 - *data1 );
}

static int
db_udown( data1, data2, len ) 
	register unsigned int	*data1, *data2, len;
{
	return( *data2 - *data1 );
}


static int
db_ldown( data1, data2, len ) 
	register long long	*data1, *data2;
	register int		len;
{
	if( *data2 == *data1 )		return( 0 );
	else if( *data2 > *data1 )	return( 1 );
	else				return( -1 );
}

static int
db_uldown( data1, data2, len ) 
	register unsigned long long	*data1, *data2;
	register int		len;
{
	if( *data2 == *data1 )		return( 0 );
	else if( *data2 > *data1 )	return( 1 );
	else				return( -1 );
}

/*****************************************************************************
 *
 *  関数名	db_fup()
 *
 *  機能	実数項目についての昇順評価関数
 * 
 *  関数値
 *     		-1
 *		0
 *		1
 *
 ******************************************************************************/
static int
db_fup( data1, data2, len ) 
	register float	*data1, *data2;
	int	len;
{
	if ( *data1 > *data2 ) 
		return( 1 );
	else if ( *data1 == *data2 )
		return( 0 );
	else 
		return( -1 );
}


/*****************************************************************************
 *
 *  関数名	db_fdown()
 *
 *  機能	実数項目についての降順評価関数
 * 
 *  関数値
 *     		-1
 *	 	0
 *	 	1
 *
 ******************************************************************************/
static int
db_fdown( data1, data2, len ) 
	register float	*data1, *data2;
	int	len;
{
	if ( *data1 > *data2 ) 
		return( -1 );
	else if ( *data1 == *data2 )
		return( 0 );
	else 
		return( 1 );
}


/*****************************************************************************
 *
 *  関数名	db_sup()
 *
 *  機能	文字列項目についての昇順評価関数
 * 
 *  関数値
 *     		負整数
 *		0
 *		正整数
 *
 ******************************************************************************/
static int
db_sup( data1, data2, len ) 
	register char	*data1, *data2;
	register int	len;
{
	return( strncmp( data1, data2, len ) );
}


/*****************************************************************************
 *
 *  関数名	db_sdown()
 *
 *  機能	文字列項目についての評価関数
 * 
 *  関数値
 *     		負整数
 *		0
 *		正整数
 *
 ******************************************************************************/
static int
db_sdown( data1, data2, len ) 
	register char	*data1, *data2;
	register int	len;
{
	return( strncmp( data2, data1, len ) );
}

