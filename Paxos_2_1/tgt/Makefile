sbindir ?= $(PREFIX)/sbin
libdir ?= $(PREFIX)/lib/tgt

VERSION ?= 1.0.49
#EXTRAVERSION ?= -8
VERS = $(subst ., ,$(VERSION))
V_MAJOR = $(word 1, $(VERS))
V_MINOR = $(word 2, $(VERS))
V_RELEASE = $(word 3, $(VERS))

ifneq ($(V_MINOR),10)
V_MINOR := 0$(V_MINOR)
endif

TGT_VER = $(V_MAJOR)$(V_MINOR)$(V_RELEASE)

TGTDIR = ../../tgt-$(VERSION)$(EXTRAVERSION)/usr

ifneq ($(shell test -e /usr/include/linux/signalfd.h && echo 1),)
CFLAGS += -DUSE_SIGNALFD
endif

ifneq ($(shell test -e /usr/include/sys/timerfd.h && echo 1),)
CFLAGS += -DUSE_TIMERFD
endif

TGTD_OBJS += $(addprefix $(TGTDIR)/iscsi/, conn.o param.o session.o \
		target.o chap.o sha1.o md5.o transport.o iscsi_tcp.o \
		isns.o)

ifneq ($(CEPH_RBD),)
MODULES += bs_rbd.so
endif

ifneq ($(GLFS_BD),)
MODULES += bs_glfs.so
endif

ifneq ($(SD_NOTIFY),)
CFLAGS += -DUSE_SYSTEMD
endif

ifneq ($(shell test -e /usr/include/sys/eventfd.h && test -e /usr/include/libaio.h && echo 1),)
CFLAGS += -DUSE_EVENTFD
TGTD_OBJS += $(TGTDIR)/bs_aio.o
LIBS += -laio
endif

ifneq ($(ISCSI_RDMA),)
TGTD_OBJS += i$(TGTDIR)/scsi/iser.o $(TGTDIR)/iscsi/iser_text.o
LIBS += -libverbs -lrdmacm
endif

INCLUDES += -I.
INCLUDES += -I../../NWGadget/h -I../paxos/h -I../session/h -I../status/h \
         -I../cache/h -I../PFS/h \
        -I$(TGTDIR) -I$(TGTDIR)/iscsi

CFLAGS += -D_GNU_SOURCE
CFLAGS += $(INCLUDES)
ifneq ($(DEBUG),)
CFLAGS += -g -O0 -ggdb -rdynamic
else
CFLAGS += -g -O2 -fno-strict-aliasing
endif
CFLAGS += -Wall -Wstrict-prototypes -fPIC
CFLAGS += -DTGT_VERSION=\"$(VERSION)$(EXTRAVERSION)\"
CFLAGS += -DBSDIR=\"$(DESTDIR)$(libdir)/backing-store\"
CFLAGS += -DPAXOS_VER=2
CFLAGS += -DCSS_MULTI_SESSION
#CFLAGS +=1 -DPAXOS_SESSION_SYNC

LIBS += -lPaxosSession -lPaxos -lStatus -lPaxosPFS -lCache -lCacheDlg -lNWGadget
LIBS += -lpthread -ldl

LPATH += -L../../NWGadget/lib -L../paxos/lib \
	-L../session/lib -L../status/lib -L../cache/lib -L../PFS/lib


ifneq ($(SD_NOTIFY),)
LIBS += -lsystemd-daemon
endif

PROGRAMS += tgtd.css tgtadm tgtimg clearFile createISCSIVol extendISCSIVol
TGTD_OBJS += $(TGTDIR)/tgtd.o $(TGTDIR)/mgmt.o \
		$(TGTDIR)/target.o $(TGTDIR)/scsi.o \
		$(TGTDIR)/driver.o \
		$(TGTDIR)/util.o $(TGTDIR)/work.o \
		$(TGTDIR)/concat_buf.o $(TGTDIR)/parser.o \
		$(TGTDIR)/spc.o $(TGTDIR)/sbc.o \
		$(TGTDIR)/mmc.o $(TGTDIR)/osd.o \
		$(TGTDIR)/scc.o $(TGTDIR)/smc.o \
		$(TGTDIR)/ssc.o $(TGTDIR)/libssc.o \
		$(TGTDIR)/bs_rdwr.o $(TGTDIR)/bs_ssc.o \
		$(TGTDIR)/bs_null.o $(TGTDIR)/bs_sg.o \
		$(TGTDIR)/bs.o $(TGTDIR)/libcrc32c.o $(TGTDIR)/bs_sheepdog.o

OBJS += $(TGT_OBJS) $(TGTADM_OBJS) $(TGTIMG_OBJS) 

PSS_OBJS = bs_css.o paxos_css.o log.o iscsid.o

TGTD_DEP = $(TGTD_OBJS:.o=.d)

LDFLAGS = -Wl,-E,-rpath=$(libdir)

.PHONY:all
all: $(PROGRAMS) $(MODULES)

tgtd.css: $(TGTD_OBJS) $(PSS_OBJS)
	echo $(CC) $^ -o $@ $(LIBS)
	$(CC) $(LPATH) $^ -o $@ $(LDFLAGS) $(LIBS)

-include $(TGTD_DEP)

TGTADM_OBJS = $(TGTDIR)/tgtadm.o $(TGTDIR)/concat_buf.o
TGTADM_DEP = $(TGTADM_OBJS:.o=.d)

tgtadm: $(TGTADM_OBJS)
	$(CC) $^ -o $@

-include $(TGTADM_DEP)

TGTIMG_OBJS = $(TGTDIR)/tgtimg.o $(TGTDIR)/libssc.o $(TGTDIR)/libcrc32c.o
TGTIMG_DEP = $(TGTIMG_OBJS:.o=.d)

tgtimg: $(TGTIMG_OBJS)
	$(CC) $^ -o $@

-include $(TGTIMG_DEP)

CLEARFILE_OBJS = clearFile.o paxos_css.o
CLEARFILE_DEP = $(CLEARFILE_OBJS:.o=.d)

clearFile: $(CLEARFILE_OBJS)
	$(CC) $(LPATH) $^ -o $@ $(LIBS)

-include $(CLEARFILE_DEP)

CREATEVOL_OBJS = createISCSIVol.o paxos_css.o
CREATEVOL_DEP = $(CREATEVOL_OBJS:.o=.d)

createISCSIVol: $(CREATEVOL_OBJS)
	$(CC) $(LPATH) $^ -o $@ $(LIBS)

-include $(CLEARFILE_DEP)

EXTENTVAL_OBJS = extendISCSIVol.o paxos_css.o
EXTENTVAL_DEP = $(EXTENTVAL_OBJS:.o=.d)

extendISCSIVol: $(EXTENTVAL_OBJS)
	$(CC) $(LPATH) $^ -o $@ $(LIBS)

-include $(CLEARFILE_DEP)

%.o: %.c
ifeq ($(OBJS), )
	-@ echo "tgt/usr/*.o not found"
	-@ echo "usage: make TGTDIR=<path>"
	-@ echo "     You need to specify path to 'tgt/usr' directory."
else
	-@ echo TGTDIR=$(TGTDIR)
	$(CC) -c $(CFLAGS) $*.c -o $*.o
	@$(CC) -MM $(CFLAGS) -MF $*.d -MT $*.o $*.c
endif

%.so: %.c
	$(CC) -shared $(CFLAGS) $*.c -o $*.so

bs_rbd.so: bs_rbd.c
	$(CC) -shared $(CFLAGS) bs_rbd.c -o bs_rbd.so -lrados -lrbd

bs_glfs.so: bs_glfs.c
	$(CC) -I/usr/include/glusterfs/api -shared $(CFLAGS) bs_glfs.c -o bs_glfs.so -lgfapi

.PHONY: install
install: $(PROGRAMS) $(MODULES)
	install -d -m 755 $(DESTDIR)$(sbindir)
	install -m 755 $(PROGRAMS) $(DESTDIR)$(sbindir)
ifneq ($(MODULES),)
	install -d -m 755 $(DESTDIR)$(libdir)/backing-store
	install -m 755 $(MODULES) $(DESTDIR)$(libdir)/backing-store
endif

.PHONY: clean
clean:
	rm -f *.[od] *.so $(PROGRAMS)
