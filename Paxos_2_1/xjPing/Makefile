all:
	mkdir -p lib bin
	cd src/ctl;make
	cd Paxos;make
	cd adm;make
clean:
	cd src/ctl;make clean
	cd Paxos;make clean
	cd adm;make clean

install:
	cd src/ctl;make install
	cd Paxos;make install
	cd adm;make install
	cd bin;make install

uninstall:
	cd src/ctl;make uninstall
	cd Paxos;make uninstall
	cd adm;make uninstall
	cd bin;make uninstall
