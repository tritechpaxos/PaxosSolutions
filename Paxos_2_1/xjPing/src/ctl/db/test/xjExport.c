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

#include	"neo_db.h"
#include	"stdio.h"

#ifndef REC_NUM_ONCE
#define REC_NUM_ONCE	(100)
#endif /* REC_NUM_ONCE */

FILE	*fp;

int csv_create(char*, int, char*, int*);

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	r_man_t		*md;
	r_tbl_t		*td;
	r_head_t	*rp;
	char		*bufp;
	int		i, n;
	int		size, num;
	char            file_nm[128];
	int		rec_num_once;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 3 ) {
		printf("usage:xjExport db_man table_nm [rec_num_once]\n");
		neo_exit( 1 );
	}

DBG_A(("db_man=[%s],table=[%s]\n", av[1], av[2]));

	if (av[3]){
		rec_num_once = atoi(av[3]);
	}else{
		rec_num_once = REC_NUM_ONCE;
	}

	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	/*
	 *	table open
	 */
	if( !(td = rdb_open( md, av[2] ) ) )
		goto err1;

	/* output file  */
	memset(file_nm, '\0', sizeof(file_nm));
	sprintf(file_nm, "%s.csv", av[2]);

	if ((fp=fopen(file_nm, "w")) == NULL)
		goto err1;

	/*
	 *	parameters
	 */
	size = rdb_size( td );
	num  = rdb_number( td, 1 );
	num = num > rec_num_once ? rec_num_once : num;

DBG_A(("size=%d,num=%d\n", size, num ));

#ifdef CSV_EXPORT_SCM

	/* write scm name to csv file */
	for (i = 0; i < td->t_scm->s_n; i++){
		fprintf(fp, "%s", td->t_scm->s_item[i].i_name);
		if (i < td->t_scm->s_n - 1)
			fprintf(fp, ",");
	}

	fprintf(fp, "\n");

	/* write scm define to csv file */
	for (i = 0; i < td->t_scm->s_n; i++){
		memset(csv_bp, '\0', sizeof(csv_bp);
		if (csv_create(td->t_scm->s_item[i].i_type,
			   strlen(td->t_scm->s_item[i].i_type),
			   csv_bp,
		           &on) != 0)
			goto err1;
	
		fprintf(fp, "%s", csv_bp);
		if (i < td->t_scm->s_n - 1)
			fprintf(fp, ",");
	}

	fprintf(fp, "\n");

#endif /* CSV_EXPORT_SCM */

	/*
	 *	allocate buffer
	 */
	if( !(bufp = (char *)neo_malloc( num*size ) ) )
		goto err1;

	/*
	 *	find
	 */
	while( (n = rdb_find( td, num, (r_head_t*)bufp, 0 ) ) > 0 ) {

DBG_A(("n=%d\n", n ));
		for( i = 0, rp = (r_head_t*)bufp; i < n;
				i++, rp = (r_head_t*)((char*)rp + size ) ) {
	
DBG_A(("rec(rel=%d,abs=%d,stat=0x%x[%s])\n", 
	rp->r_rel, rp->r_abs, rp->r_stat, rp+1 ));

			print_record(i, rp, td->t_scm->s_n, td->t_scm->s_item);
		}
	}

DBG_A(("n=%d,neo_errno = 0x%x\n", n, neo_errno ));

	/*
	 *	close
	 */
	if( rdb_close( td ) )
		goto err1;

	rdb_unlink( md );
	if (fp) fclose(fp);
	if (bufp) neo_free(bufp);

DBG_EXIT(0);
	printf("xjExport OK\n");
	return( 0 );

err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));

	if (md) rdb_unlink( md );
	if (fp) fclose(fp);
	if (bufp) neo_free(bufp);

DBG_EXIT(-1);
	printf("xjExport FAIL\n");
	return( -1 );
}

int csv_create(p, n, op, on)
	char *p;	/* (I) input string */
	int n;		/* (I) size */
	char *op;	/* (O) output point */
	int *on;	/* (O) output strint size */
{
	int must_use_quart = 0;
	int w = 0;
	int pos = 0;
	char *rp = NULL;
	char *noquart = NULL;

	if(p == NULL || on == NULL)
		return (-1);
	
	*on = 0;

	if (strchr(p, ',') != NULL)
		must_use_quart = 1;
	if ((noquart = strchr(p, '"')) != NULL)
		must_use_quart = 1;
	
	if (!must_use_quart){
		*on = n;
		memcpy(op, p, n);	
		return (0);
	}

	if (must_use_quart)
	{
		bcopy("\"", op + w, 1);
		w ++ ;
		if (!noquart){
			sprintf(op + w, "%s\"", p);
			*on = n + 2;
			return (0);
		}
	}

	for (pos = 0, rp = p; pos < strlen(p); pos++, rp++, w++){
		*(op + w) = *rp;
		if (*rp == '\"'){
			w ++;
			*(op + w) = '\"';
		}
	}

	if (must_use_quart)
	{
		sprintf(op + w, "\"");
		w ++ ;
	}

	*on = w;

	return (0);
}
