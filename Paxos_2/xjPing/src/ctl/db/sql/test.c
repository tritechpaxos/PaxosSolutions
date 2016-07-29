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

#include	<sys/types.h>
#include	<sys/stat.h>
#include	<fcntl.h>

#include	"neo_db.h"
#include	"sql.h"

#include	"string.h"

char	buff[2048];

/***
char	*_search_tbl, *_search_usr;
***/

extern r_expr_t *SQLreturn;
extern r_expr_t *sql_getnode();
extern r_expr_t *sql_read_tree();
extern		sql_write_tree();

/***
char	*sql[] = {

"ZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZZ",
"select * from A where a = 1 and name not like 'aa' escape '\\';",
"select * from A where name not like 'aa';",
"select * from A where INT>=5;",
"update XJ_SEQUENCE set SEQ=1+2 where NAME='userID';",
"create view TEST_VIEW as select A.NAME A_NAME,  B.HEX B_HEX from A, B where A.NAME = B.NAME;",
"select * from A; drop table dep;delete from B;"
"create VIEW view_table AS SELECT A.a A_a, B.b B_b,Cc FROM A, B, C WHERE a.id=b.id AND a.id=9;drop table view_table;",
"create VIEW view_table AS SELECT A.a A_a, B.b B_b,Cc FROM A, B, C WHERE a.id=b.id AND a.id=9;",
"SElect a.a1 aaa, b FROM A;",
"SElect * FROM A, B, C, D where a = 1 and b = 'bbbb';",
"drop view  view_table;",
"drop table  department;",
"create VIEW view_table AS SELECT A, B FROM A, B WHERE a.id=b.id;",
"CREATE TABLE department (depno NUMBER(2),dname char(10) );",
"delete from A;",
"delete from A where A.a = 9;",
"insert into A values( 1, 'a');",
"insert into A(a1,a2,a3) values( 1, 'a', 99);",
"update a set loc='aa' where id=1;",
"update a set loc='aa', bb=10  where id=1;",
"SElect * FROM A;",
"SElect a.a1 aaa, b FROM A;",
"SElect * FROM A where a = 1 and (b = 'bbbb' OR A.c=2);",
"delete from A;",
"insert into A values( 1, 'a');",

	};
***/

neo_main()
{
	int	fd;
	int	ret;
	int	i;
	int	m;

	r_expr_t	*np, *nnp;

char *wcp;
char wbuf[256];
char sql[256];

extern int sqldebug;
sqldebug = 1;
	sql_lex_init();
/*****
	for( i = 0; i < sizeof(sql)/sizeof(sql[0]); i++ ) {
******/
	for( i = 0; i < 10; i++ ) {
printf("sql>");
wcp = gets(wbuf);
if( wcp == 0 ) {
printf("\n Read Error\n");
break;
}
printf("\nINPUT:%s\n", wbuf);
sql[0] = '\0';
strcpy(sql,wbuf);

printf("**** INPUT: HEX DUMP ***\n");
for(m=0;m<256;m++) {
	if( sql[m] == '\0' )
		break;
	printf("%c(%0x) ",sql[m],sql[m]);
}
printf("\n**** HEX DUMP ***\n");

		sql_mem_init();
/***
		sql_lex_start( sql[i], buff, sizeof(buff) );
***/
		sql_lex_start( sql, buff, sizeof(buff) );

		ret = sqlparse();
/***
		printf("%d:sqlparse=%d[%s]\n", i, ret, sql[i] );
***/
		printf("%d:sqlparse=%d[%s]\n", i, ret, sql );

		if( ret )	break;

		sqlprint( SQLreturn, 0 );

		sql_free( SQLreturn );
	}
}
