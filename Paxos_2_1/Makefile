all:
	cd paxos;make
	cd session;make
	cd status;make
	cd cache;make
	cd PFS;make
	cd xjPing;make
	cd VV;make
	cd memcache;make
	cd RAS;make
	cd CNV;make

clean: #clean_installdir
	cd paxos;make clean
	cd session;make clean
	cd status;make clean
	cd cache;make clean
	cd PFS;make clean
	cd xjPing;make clean
	cd VV;make clean
	cd memcache;make clean
	cd RAS;make clean
	cd CNV;make clean

install:
	cd paxos;make install
	cd session;make install
	cd status;make install
	cd cache;make install
	cd PFS;make install
	cd xjPing;make install
	cd VV;make install
	cd memcache;make install
	cd RAS;make install
#	cd CNV;make install

uninstall:
	cd paxos;make uninstall
	cd session;make uninstall
	cd status;make uninstall
	cd cache;make uninstall
	cd PFS;make uninstall
	cd xjPing;make uninstall
	cd VV;make uninstall
	cd memcache;make uninstall
	cd RAS;make uninstall
#	cd CNV;make uninstall


DISTRIBUTOR=$(shell lsb_release  -a | grep Distributor | cut -f 2 -d '	')
CSSCOM_INSTDIR=csscom
CSSD_INSTDIR=cssd
CSSDC_INSTDIR=cssdc
RASD_INSTDIR=rasd

clean_installdir:
	sudo rm -rf $(CSSCOM_INSTDIR)/* $(CSSD_INSTDIR)/* $(CSSDC_INSTDIR)/* $(RASD_INSTDIR)/*

bin_package:
	make ORG_DIR=`pwd` estimate-bin_package

estimate-bin_package: bin_package-csscom bin_package-cssd bin_package-cssdc bin_package-rasd

bin_package-csscom:
ifeq ($(DISTRIBUTOR),Ubuntu)
	-@ if [ ! -d $(CSSCOM_INSTDIR) ]; then mkdir $(CSSCOM_INSTDIR);fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/DEBIAN ]; then mkdir $(CSSCOM_INSTDIR)/DEBIAN;fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/etc/css ]; then sudo mkdir -p $(CSSCOM_INSTDIR)/etc/css;fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/etc/init.d ]; then sudo mkdir -p $(CSSCOM_INSTDIR)/etc/init.d;fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/usr/bin ]; then sudo mkdir -p $(CSSCOM_INSTDIR)/usr/bin;fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/usr/local/css ]; \
		then sudo mkdir -p $(CSSCOM_INSTDIR)/usr/local/css;fi
	-@ if [ ! -d $(CSSCOM_INSTDIR)/lib/css ]; then sudo mkdir -p $(CSSCOM_INSTDIR)/lib/css;fi
	sudo install session/bin/SessionShutdown $(CSSCOM_INSTDIR)/usr/bin/SessionShutdown
	sudo install paxos/bin/PaxosAdmin $(CSSCOM_INSTDIR)/usr/bin/PaxosAdmin
	sudo install css_shutdown.sh $(CSSCOM_INSTDIR)/usr/bin/
	sudo install css_funcs  $(CSSCOM_INSTDIR)/lib/css/
	sudo install example_server.conf  $(CSSCOM_INSTDIR)/etc/css/
	sudo install README.csscom  $(CSSCOM_INSTDIR)/usr/local/css/

	install DEBIAN.csscom/* $(CSSCOM_INSTDIR)/DEBIAN/
	sudo dpkg -b $(CSSCOM_INSTDIR) csscom.deb
else
ifeq ($(DISTRIBUTOR),CentOS)
	rpmbuild -bb csscom.spec
endif
endif


bin_package-cssd:
ifeq ($(DISTRIBUTOR),Ubuntu)
	-@ if [ ! -d $(CSSD_INSTDIR) ]; then mkdir $(CSSD_INSTDIR);fi
	-@ if [ ! -d $(CSSD_INSTDIR)/DEBIAN ]; then mkdir $(CSSD_INSTDIR)/DEBIAN;fi
	-@ if [ ! -d $(CSSD_INSTDIR)/etc/css ]; then sudo mkdir -p $(CSSD_INSTDIR)/etc/css;fi
	-@ if [ ! -d $(CSSD_INSTDIR)/etc/init.d ]; then sudo mkdir -p $(CSSD_INSTDIR)/etc/init.d;fi
	-@ if [ ! -d $(CSSD_INSTDIR)/usr/sbin ]; then sudo mkdir -p $(CSSD_INSTDIR)/usr/sbin;fi
	-@ if [ ! -d $(CSSD_INSTDIR)/usr/local/css ]; \
		then sudo mkdir -p $(CSSD_INSTDIR)/usr/local/css;fi
	sudo install PFS/bin/PFSServer $(CSSD_INSTDIR)/usr/sbin/
	sudo chmod a+s $(CSSD_INSTDIR)/usr/sbin/PFSServer
	sudo install cssd.sh $(CSSD_INSTDIR)/usr/sbin/cssd
	sudo install css_delper.sh $(CSSD_INSTDIR)/usr/sbin/
	sudo install cssd_init.d  $(CSSD_INSTDIR)/etc/init.d/cssd
	sudo chmod a+s $(CSSD_INSTDIR)/etc/init.d/cssd
	sudo install README.cssd  $(CSSD_INSTDIR)/usr/local/css/
	install DEBIAN.cssd/* $(CSSD_INSTDIR)/DEBIAN/
	sudo dpkg -b $(CSSD_INSTDIR) cssd.deb
else
ifeq ($(DISTRIBUTOR),CentOS)
	rpmbuild -bb cssd.spec
endif
endif

bin_package-cssdc:
ifeq ($(DISTRIBUTOR),Ubuntu)
	-@ if [ ! -d $(CSSDC_INSTDIR) ]; then mkdir $(CSSDC_INSTDIR);fi
	-@ if [ ! -d $(CSSDC_INSTDIR)/DEBIAN ]; then mkdir $(CSSDC_INSTDIR)/DEBIAN;fi
	-@ if [ ! -d $(CSSDC_INSTDIR)/usr/bin ]; then sudo mkdir -p $(CSSDC_INSTDIR)/usr/bin;fi
	-@ if [ ! -d $(CSSDC_INSTDIR)/usr/local/css ]; \
		then sudo mkdir -p $(CSSDC_INSTDIR)/usr/local/css;fi
	sudo install PFS/bin/PFSClient $(CSSDC_INSTDIR)/usr/bin/
	sudo install PFS/bin/PFSProbe $(CSSDC_INSTDIR)/usr/bin/
	sudo install css_lookup-server.sh $(CSSDC_INSTDIR)/usr/bin/
	sudo install css_lookup-port.sh $(CSSDC_INSTDIR)/usr/bin/
	sudo install css_set_ssh.sh $(CSSDC_INSTDIR)/usr/bin/
	sudo install css_run.sh $(CSSDC_INSTDIR)/usr/bin/
	sudo install README.cssdc  $(CSSDC_INSTDIR)/usr/local/css/
	install DEBIAN.cssdc/* $(CSSDC_INSTDIR)/DEBIAN/
	sudo dpkg -b $(CSSDC_INSTDIR) cssdc.deb
else
ifeq ($(DISTRIBUTOR),CentOS)
	rpmbuild -bb cssdc.spec
endif
endif


bin_package-rasd:
ifeq ($(DISTRIBUTOR),Ubuntu)
	-@ if [ ! -d $(RASD_INSTDIR) ]; then mkdir $(RASD_INSTDIR);fi
	-@ if [ ! -d $(RASD_INSTDIR)/DEBIAN ]; then mkdir $(RASD_INSTDIR)/DEBIAN;fi
	-@ if [ ! -d $(RASD_INSTDIR)/etc/ras ]; then sudo mkdir -p $(RASD_INSTDIR)/etc/ras;fi
	-@ if [ ! -d $(RASD_INSTDIR)/etc/init.d ]; then sudo mkdir -p $(RASD_INSTDIR)/etc/init.d;fi
	-@ if [ ! -d $(RASD_INSTDIR)/usr/sbin ]; then sudo mkdir -p $(RASD_INSTDIR)/usr/sbin;fi
	-@ if [ ! -d $(RASD_INSTDIR)/usr/local/ras ]; \
		then sudo mkdir -p $(RASD_INSTDIR)/usr/local/ras;fi
	#-@ if [ ! -d $(RASD_INSTDIR)/lib/css ]; then sudo mkdir -p $(RASD_INSTDIR)/lib/css;fi
	sudo install PFS/bin/PFSServer $(RASD_INSTDIR)/usr/sbin/
	sudo chmod a+s $(RASD_INSTDIR)/usr/sbin/PFSServer
	sudo install cssd.sh $(RASD_INSTDIR)/usr/sbin/cssd
	sudo install css_delper.sh $(RASD_INSTDIR)/usr/sbin/
	sudo install cssd_init.d  $(RASD_INSTDIR)/etc/init.d/rasd
	sudo chmod a+s $(RASD_INSTDIR)/etc/init.d/rasd
	sudo install example_server.conf  $(RASD_INSTDIR)/etc/ras/
	sudo install README.rasd  $(RASD_INSTDIR)/usr/local/ras/
	install DEBIAN.rasd/* $(RASD_INSTDIR)/DEBIAN/
	sudo dpkg -b $(RASD_INSTDIR) rasd.deb
else
ifeq ($(DISTRIBUTOR),CentOS)
	rpmbuild -bb rasd.spec
endif
endif
