drop view TESTB_1_0;
drop view TESTB_2_0;
drop view TESTB0;
create view TESTB_1_0 as select * from TESTB_1 where 0<=TESTB_1.INT and TESTB_1.INT<1000;
create view TESTB_2_0 as select * from TESTB_2 where 0<=TESTB_2.INT and TESTB_2.INT<1000;
create view TESTB0 as select * from TESTB_1_0, TESTB_2_0 where TESTB_1_0.INT=TESTB_2_0.INT;
select * from TESTB0;
