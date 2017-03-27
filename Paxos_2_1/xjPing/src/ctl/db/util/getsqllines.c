/*******************************************************************************
 *
 *  getsqllines.c --- xjPing (On Memory DB)  Library programs
 *
 *  Copyright (C) 2011-2016 triTech Inc. All Rights Reserved.
 *
 *  See GPL-LICENSE.txt for copyright and license details.
 *
 *  This program is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free Software
 *  Foundation; either version 3 of the License, or (at your option) any later
 *  version.
 *
 *  This program is distributed in the hope that it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with
 *  this program. If not, see <http://www.gnu.org/licenses/>.
 *
 ******************************************************************************/
/*
 *   作成            渡辺典孝
 *   試験
 *   特許、著作権    トライテック
 */

#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <string.h>

#define XXX_PROMPT  "sql>"

/*
 * initialize readline() keybind and history buffer
 */
static void
init_getsqllines()
{
    rl_bind_key('\t', rl_insert);   /* disable completion   */
    rl_unbind_key('');          /* delete -> EOF    */
    clear_history();            /* clerar edit buffer   */
}

/*
 * quasi-multiline editer
 *
 * in : buf0  : pointer to the start of a buffer
 *      buflen: buffer size
 * out: NULL  : return by EOF
 *      !NULL : buf0
 */
char *
getsqllines(char *buf0, int buflen)
{
    char    *buf,*ln;
    int     index,lnlen, len, rest;

    /*
     * initialize readline() keybind and history buffer
     */
    init_getsqllines();

    /*
     * get input lines;
     */
    for(;;)
    {
        ln = readline(XXX_PROMPT);
        if( ln == NULL )
        {
            /*
             * if EOF in the empty line, return NULL
             */
            return NULL;
        }

        index = where_history();

        if( index < history_length )
        {
            replace_history_entry( index, ln, NULL );
        }
        else
        {
            add_history(ln);
        }

            if( ln[0] == '#' || ln[strlen(ln)-1] == ';' )
                break;
    }

    /*
     *  copy the readline() buffer to the specified buffer
     */
    buf = buf0;
    *buf = '\0';

    for( index=1;index<=history_length;index++)
    {
        HIST_ENTRY *hp;
#ifdef DEBUG
        printf("%d\n",index);
#endif

        hp = history_get( index );
        lnlen = strlen( hp->line );
#ifdef DEBUG
        printf("%s, %s\n",hp->line,buf);
#endif
        rest = buflen - (buf-buf0);

        len = lnlen < rest ? lnlen : rest - 1;

        strncpy( buf, hp->line, len );
        buf += len;

        if( buflen - (buf-buf0) < 2 ) break;

        *buf++ = '\n';

    }

    *buf='\0';

    return buf0;
}


