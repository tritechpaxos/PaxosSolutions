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
#include	<stdio.h>
#include	<string.h>

#ifndef REC_NUM_ONCE
#define REC_NUM_ONCE	(100)
#endif /* REC_NUM_ONCE */

#define SCM_DEF_SIZE	(4096)
#define TABLE_LINE	(100000)

struct parser{
	char *cs_p;
	int  cs_s;
};

static void   parser_free(struct parser *, int) ;
static void   parser_init(struct parser *, int) ;
int 			parser_csv_line(char*, int, int, struct parser[] );

int
neo_main( ac, av )
	int	ac;
	char	*av[];
{
	FILE		*fp = NULL;
	char 		*bufp = NULL;
	char		*rp = NULL;
	char		*s = NULL;
	r_man_t		*md = NULL;
	r_tbl_t		*td = NULL;
	int		n;
	int		size;
	char            file_nm[128];
	int		rec_num_once;
	int 		count;

	long long	ll;
	unsigned long 	long ull;
	float		ft;
	unsigned int	ui;
	int		ii;

	struct parser 	*par = NULL;
	int 		columns = 0;

DBG_ENT(M_RDB,"neo_main");

	if( ac < 4 ) {
		printf("usage:xjImport db_man table_nm csv_file_nm [rec_num_once]\n");
		neo_exit( 1 );
	}
DBG_A(("db_man=[%s],table=[%s] file[%s]\n", 
			av[1], av[2], av[3]));

	if (av[4]){
		rec_num_once = atoi(av[4]);
	}else{
		rec_num_once = REC_NUM_ONCE;
	}

	/* input file  */
	memset(file_nm, '\0', sizeof(file_nm));
	sprintf(file_nm, "%s", av[3]);
	if ((fp = fopen(file_nm, "r")) == NULL){
		goto err1;
	}

#ifdef  CSV_CREATE_TABLE
	/* read scm line */
	if (fgets(clm_name_line, sizeof(clm_name_line, fp) == NULL) 
		goto err1;

	if (fgets(clm_def_line, sizeof(clm_def_line, fp) == NULL) 
		goto err1;
#endif /* CSV_CREATE_TABLE */

	/* db open */
	if( !(md = rdb_link( "CLIENT", av[1] ) ))
		goto err1;

	/*
	 *      table open
	 */
	if( !(td = rdb_open( md, av[2] ) ) ) {
		/* create table */
#ifdef  CSV_CREATE_TABLE
		/* how to map SQL DATA <-> C DATA ??? */
		if ((td = create_table(
				md, av[2], 
				clm_name_line, clm_def_line)) == NULL)
			goto err1;
#else /* CSV_CREATE_TABLE */
       	 	goto err1;
#endif /* CSV_CREATE_TABLE */
	}

	/*
	 *	parameters
	 */
	size = rdb_size( td );

	/*
	 *	allocate buffer
	 */
	if( !(bufp = (char *)neo_zalloc( size * 2) ) )
		goto err1;

	if( !(rp = (char *)neo_zalloc( size * (rec_num_once + 2)) ) )
		goto err1;

	columns = td->t_scm->s_n;

loop:
	count = 0;
	s = rp;

	while(count != rec_num_once){

		if (fgets(bufp, size * 2 -1, fp) == NULL) break;
		count++; 

		/* parser csv line */
		if( (par = (struct parser *)malloc( sizeof(struct parser) * columns)) == NULL){
			goto err1;
		}

		parser_init(par, columns);

		if (parser_csv_line(bufp, strlen(bufp) - 1,
				    td->t_scm->s_n, par) != 0)
			goto err2;

		/* set rdb */
		for (n = 0; n < columns; n++){

			switch(td->t_scm->s_item[n].i_attr)
			{
				case (R_STR) : 
					if (par[n].cs_p != NULL){
						if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, par[n].cs_p)) goto err1;
					}else{
						if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, "")) goto err1;
					}
					break;
				case (R_HEX) :
					if (sscanf(par[n].cs_p, "%x", &ui) == -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ui)) goto err1;
					break;
				case (R_UINT) :
					if (sscanf(par[n].cs_p, "%u", &ui) == -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ui)) goto err1;
					break;
				case (R_INT) :
					if (sscanf(par[n].cs_p, "%d", &ii) ==  -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ii)) goto err1;
					break;
				case (R_FLOAT) :
					if (sscanf(par[n].cs_p, "%f", &ft) == -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ft)) goto err1;
					break;
				case (R_LONG) :
					if (sscanf(par[n].cs_p, "%lld", &ll) == -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ll)) goto err1;
					break;
				case (R_ULONG) :
					ull = atoll(par[n].cs_p);
					if (sscanf(par[n].cs_p, "%lld", &ull) == -1)
						goto err1;
					if (rdb_set(td, (r_head_t*)s, td->t_scm->s_item[n].i_name, (char*)&ull)) goto err1;
					break;
					
			}
		}
		s += size;
		parser_free(par, columns);
	}

	if (count){
		/* insert */
		if (rdb_insert(td, count, (r_head_t*)rp, 0, 0, 0))
			goto err1;

		/* next */
		if (count == rec_num_once)
			goto loop;
	}

	/*
	 *	close
	 */
	if( rdb_close( td ) )
		goto err1;

	rdb_unlink( md );

DBG_EXIT(0);
	if (fp) fclose(fp);
	if(bufp) neo_free(bufp);
	if(rp) neo_free(rp);
	printf("xjImport OK\n");
	return( 0 );

err2:
	printf("error in line [%s]\n", bufp);
err1:
DBG_ERR("err1");
DBG_A(("neo_errno=0x%x\n", neo_errno ));
DBG_EXIT(-1);
	if (md) rdb_unlink( md );
	if (fp) fclose(fp);
	if(bufp) neo_free(bufp);
	if(rp) neo_free(rp);
	if (par) parser_free(par, columns);
	printf("xjImport FAIL\n");
	return( -1 );
}


int
parser_csv_line(csv_line, n, column_n, par)
	char	*csv_line;	/* (I) csv string to match */
	int	n;		/* (I) csv line size */
	int     column_n;	/* (I) column count */
	struct  parser    par[]; /* (O) parser struct */
{
	char *rp;
	char *wp;
	char *p;
	int size = 0;
	int i;
	int wfg = 0;
	int endfg = 0;
	int endpos = 0;
	int quartn = 0;
	int rn = 0;

	if ((p = wp = neo_zalloc(n)) == NULL){
		return (-1);
	}
	
	for (i = 0, rp = csv_line; i < n ; i++){
		if (endfg) {
			wp = p; endfg = 0; quartn = 0;
			endpos = i - 1; size = 0;
		}

		switch(rp[i]){
			case (','):
				if (quartn%2 == 0 ){
					endfg = 1;
				}else{
					wfg = 1;
				}
				break;			
			case ('"'):
				quartn ++;
				if (quartn%2 == 0) wfg = 1;
				break;
			default :
				wfg = 1;
				break;
		}

		if (wfg) {
			sprintf(wp, "%1.1s", rp+i);
			wp++;
			size++;
			wfg = 0;
		}


		if (endfg || i == n - 1){
			if (quartn) size--;
			rn ++;
			if (rn > column_n){
				if (p) neo_free(p);
				return (-1);
			}
			par[rn-1].cs_s = size;
			if ( !*p  && (endpos == i - 1 || endpos == i)){
				par[rn-1].cs_p = NULL;	
			}else{
				par[rn-1].cs_p = neo_zalloc(size + 1);
				bcopy(p, par[rn-1].cs_p, size);
			}
		}
			
	}

	if (p) neo_free(p);
	if (rn < column_n){
		return (-1);
	}else{
		return (0);
	}
}

	
static void
parser_free(par, n) 
	struct parser *par;
	int n;
{ 
	int i; 
	
	if (!par) return;
	for(i = 0; i < n; i++) { 
		if (par[i].cs_p) {
			neo_free(par[i].cs_p); 
		} 
	} 
	free(par);
}

static void
parser_init(par, n) 
	struct parser *par;
	int n;
{ 
	int i; 

	for(i = 0; i < n; i++) { 
		par[i].cs_p = NULL;
		par[i].cs_s = 0;
	} 
}

int
create_table()
{
	return (-1);
}
