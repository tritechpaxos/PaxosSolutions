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
 *	skl_arg.c
 *
 *	説明	引数構造文字列操作
 * 
 ******************************************************************************/

#include	"skl_prcs.h"


/*******************************************************************************
 * 関数名
 *	_neo_ArgMake
 * 機能
 *	引数構造の文字列を作成する。
 *		"arg1 arg2 arg3 ..." --> argv[]
 * 関数値
 *	引数数
 * 注意
 ******************************************************************************/
int
_neo_ArgMake(argc, argv, buf, comment, delimitter)
int	argc;		/* IN:     引数最大数 */
char	*argv[];	/* OUT:    引数アドレス格納バッファ */
char	*buf;		/* IN/OUT: 引数バッファ */
int	comment;	/* IN:     コメント文字 or 0  */
int	delimitter;	/* IN:     区切り文字 or 0  */
{
	register int	n, quot = 0;
	register char *cp = buf;

DBG_ENT( M_SKL, "_neo_ArgMake" );

	for (n = 0;;) {
		if (n >= argc) {
			DBG_EXIT(n);
			return (n);
		}
		while (*cp && (*cp == ' ' || *cp == '\t' || *cp == delimitter))
			cp++;
		 /* ヌルか改行コードかコメント文字までが対象 */
		if (!*cp || *cp == comment || *cp == '\n')
			break;

		/* ', " に囲まれた文字列は一つの引数とみなす。*/
		if (*cp == '\'' || *cp == '"') {
			quot = *cp++;
		}
		argv[n++] = cp;
		for (; *cp; cp++) {
			if (quot) {
				if (*cp == quot) {
					quot = 0;
					break;
				}
				continue;
			}
			if (*cp == comment || *cp == '\n') {
				*cp = 0;
				break;
			}
			/* ブランクかタブかデリミッタ文字を区切りとする。*/
			if (isspace((int)(*cp)) || *cp == (char)delimitter) {
				break;
			}
		}
		if (!*cp)
			break;
		*cp++ = '\0';
	}
	argv[n] = 0;

DBG_EXIT(n);
	return (n);
}
