drop view V_1_1;
create view V_1_1 as select * from TEST_1 where INT<1000;
drop view V_2_1;
create view V_2_1 as select * from TEST_2 where INT<1000;
drop view V1;
create view V1 as select * from V_1_1,V_2_1 where V_1_1.INT=V_2_1.INT;
#;
drop view V_1_2;
create view V_1_2 as select * from TEST_1 where 1000<=INT and INT<2000;
drop view V_2_2;
create view V_2_2 as select * from TEST_2 where 1000<=INT and INT<2000;
drop view V2;
create view V2 as select * from V_1_2,V_2_2 where V_1_2.INT=V_2_2.INT;
#;
drop view V;
create view V as select * from V1 
	union all select * from V2;
