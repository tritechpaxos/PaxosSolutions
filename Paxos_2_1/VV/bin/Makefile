TARGET = InitDB.sh \
		iscsi-admin.sh iscsi-deploy.sh iscsi-login.sh iscsi-logout.sh \
		iscsi-start.sh iscsi-stop.sh \
		lvs_adm.sh lvs_deploy.sh lvs_monitor.sh \
		vv_adm.sh vv_autonomic.sh vv_deploy.sh vv_log.sh vv_start.sh vv_stop.sh

ROOT = ../../

install:
	cp $(TARGET) $(ROOT)bin

uninstall:
	cd $(ROOT)bin; rm -f $(TARGET)
