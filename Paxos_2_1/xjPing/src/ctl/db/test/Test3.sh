#!/bin/sh

LANG=EUC-JP

echo	"=== drop ºı∏≥¿∏;"
echo	"drop table ºı∏≥¿∏;"
echo	"create table ºı∏≥¿∏(Ãæ¡∞ char(30), ∏© char(30));"
echo	"insert into ºı∏≥¿∏ values('≈œ ’≈µπß', 'ÀÃ≥§∆ª');"
echo	"insert into ºı∏≥¿∏ values('≈œ ’º£…◊', 'ø∑≥„');"
echo	"insert into ºı∏≥¿∏ values('±ˆø¨æÔ¿≤', 'ÀÃ≥§∆ª');"
echo	"insert into ºı∏≥¿∏ values('_é‹é¿é≈éÕéﬁé…éÿé¿é∂', 'ÀÃ≥§∆ª');"
echo	"select * from ºı∏≥¿∏;"
echo	"commit;			==="
xjSql $1 <<_EOF_ > z
drop table ºı∏≥¿∏;
create table ºı∏≥¿∏(Ãæ¡∞ char(30), ∏© char(30));
insert into ºı∏≥¿∏ values('≈œ ’≈µπß', 'ÀÃ≥§∆ª');
insert into ºı∏≥¿∏ values('≈œ ’º£…◊', 'ø∑≥„');
insert into ºı∏≥¿∏ values('±ˆø¨æÔ¿≤', 'ÀÃ≥§∆ª');
insert into ºı∏≥¿∏ values('_é‹é¿é≈éÕéﬁé…éÿé¿é∂', 'ÀÃ≥§∆ª');
select * from ºı∏≥¿∏;
commit;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=4" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=0" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '%≈œ ’%' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '%≈œ ’%';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’≈µπß' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’≈µπß';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’%≈µπß' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’%≈µπß';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’%' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’%';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '%≈µπß' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '%≈µπß';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’____' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ ’____';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '____≈µπß' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '____≈µπß';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '__≈µπß' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '__≈µπß';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=0" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ%≈µ__' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '≈œ%≈µ__';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ not like '≈œ%≈µ__' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ not like '≈œ%≈µ__';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=3" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ not like '%é‹é¿é≈éÕéﬁ%' ===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ not like '%é‹é¿é≈éÕéﬁ%';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=3" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from ºı∏≥¿∏ where Ãæ¡∞ like '\_é‹é¿é≈éÕéﬁ%' escape '\'===";
xjSql $1 <<_EOF_ > z
select * from ºı∏≥¿∏ where Ãæ¡∞ like '\_é‹é¿é≈éÕéﬁ%' escape '\';
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== drop table …Ω;"
echo	"	create table …Ω(≥ÿ¿∏ char(20), ≈¿øÙ int);"
echo	"	insert into …Ω values('≈œ ’≈µπß', 100);"
echo	"	insert into …Ω values('≈œ ’º£…◊', 90);"
echo	"	commit;"
echo	"	select * from …Ω;"
xjSql $1 <<_EOF_ > z
drop table …Ω;
create table …Ω(≥ÿ¿∏ char(20), ≈¿øÙ int);
insert into …Ω values('≈œ ’≈µπß', 100);
insert into …Ω values('≈œ ’º£…◊', 90);
commit;
select * from …Ω;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=2" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== select * from …Ω where ≈¿øÙ = 100;"
xjSql $1 <<_EOF_ > z
select * from …Ω where ≈¿øÙ = 100;
quit;
_EOF_
if [ $? -ne 0 ] ; then exit 1 ; fi
count=$( fgrep "Total=1" z |wc -l )
if [ $count -ne 1 ]; then exit 1; fi

echo	"=== OK ==="
