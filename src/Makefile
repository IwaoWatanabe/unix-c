
# make Darwin=1  # for MacOS X

CXXFLAGS = -g -fexceptions
CXXFLAGS += -fPIC
CXXFLAGS += -pthread -D_REENTRANT -DUSE_SUBCMD

LDFLAGS = -lstdc++ 

ifdef Darwin
CFLAGS += -I/opt/X11/include
CPPFLAGS += -I/opt/X11/include
LDFLAGS += -L/opt/X11/lib
CXXFLAGS += -DDARWIN

CPPFLAGS += -I/usr/local/opt/gettext/include
LDFLAGS += -L/usr/local/opt/gettext/lib

CXXFLAGS += -Wno-c++11-compat-deprecated-writable-strings
endif

ALL_MODULE =hello cat01
ALL_MODULE += app01
ALL_MODULE += app02
ALL_MODULE += env01
ALL_MODULE += app03
ALL_MODULE += app04
TEST_MODULE = header-test.o

CFLAGS = -g -fexceptions
CFLAGS += -pthread -D_REENTRANT -DUSE_SUBCMD

CXXFLAGS += -DIGNORE_NDBM -DIGNORE_GDBM

LDFLAGS += -lpthread

ifdef USE_MOTIF
LDFLAGS += -lXm -lMrm -lUil
endif


ifdef USE_XVIEW
CXXFLAGS += -DUSE_XVIEW
OPENWIN_HOME=/usr/openwin
LDFLAGS += -L$(OPENWIN_BUILD)/lib/libxview -L$(OPENWIN_BUILD)/lib/libolgx
LDFLAGS += -L$(OPENWIN_HOME)/lib -lxview -lolgx
endif
ifdef USE_XAW3D
LDFLAGS += -lXaw3d
else
LDFLAGS += -lXaw
endif
LDFLAGS += -lXt -lXmu -lX11
ifdef USE_READLINE
CXXFLAGS += -DUSE_READLINE
LDFLAGS += -lreadline -lhistory
endif
LDFLAGS += -lncursesw
LDFLAGS += -ldb
LDFLAGS += -lldap

ifdef Darwin
LDFLAGS += -lintl
LDFLAGS += -liconv
LDFLAGS += -L/usr/local/opt/ncurses/lib
CPPFLAGS += -I/usr/local/opt/ncurses/include
LDFLAGS += -L/usr/local/opt/berkeley-db4/lib
CPPFLAGS += -I/usr/local/opt/berkeley-db4/include
endif

ifdef USE_DB_DBM
CXXFLAGS += -DDB_DBM_HSEARCH=1
endif



CXXFLAGS += `xml2-config --cflags`
LDFLAGS += `xml2-config --libs`

CXXFLAGS += `mysql_config --cflags`
LDFLAGS += `mysql_config --libs_r`
CXXFLAGS += -DLDAP_DEPRECATED=1

VERSION=0.1.0
RPMBASE=~/rpm/SOURCES

APP_NAME=examples-cpp
PREFIX=/opt/$(APP_NAME)

MYBINDIR=${PREFIX}
MYDOCDIR=${PREFIX}/docs
DESTDIR=

all: uc $(ALL_MODULE) $(TEST_MODULE)

uc:
	test -L uc || ln -s . uc

docs: 
	doxygen

app01: app01.o args.o

APP03_OBJS = app03.o subcmd.o
APP03_OBJS += term.o
app03: $(APP03_OBJS)

APP04_OBJS = app04.o
APP04_OBJS += temp.o

LIBUC_OBJS = subcmd.o 
LIBUC_OBJS += datetime.o
LIBUC_OBJS += times.o logger.o ini.o datetime.o
LIBUC_OBJS += csv-util.o csv-tool.o 
LIBUC_OBJS += file-op.o lineread.o mbs.o
LIBUC_OBJS += stdc.o stl.o
LIBUC_OBJS += term.o
LIBUC_OBJS += xml.o
LIBUC_OBJS += kvs-util.o kvs-bdb.o
LIBUC_OBJS += mysql-driver.o mysql-tool.o
LIBUC_OBJS += xt-proc.o
LIBUC_OBJS += awt-util.o SmeCascade.o
LIBUC_OBJS += userinfo.o
LIBUC_OBJS += keytool.o
LIBUC_OBJS += dir.o

APP04_OBJS += xwin.o 
APP04_OBJS += awt.o
APP04_OBJS += xwin02.o 
APP04_OBJS += awt02.o 


ifdef USE_FCGI
LIBUC_OBJS += fcgi-service.o
LDFLAGS += -lfcgi
CXXFLAGS += -Iext
LDFLAGS += -lboost_thread
else
CXXFLAGS += -DNOUSE_FCGI
endif

ifdef USE_QDBM
LIBUC_OBJS += kvs-qdbm.o
LDFLAGS += -lqdbm
else
CXXFLAGS += -DIGNORE_QDBM
endif

ifdef Darwin
CXXFLAGS += -DNOUSE_INDEX
endif

ifndef Darwin
LIBUC_OBJS += kvs-ndbm.o
LIBUC_OBJS += kvs-gdbm.o
LDFLAGS += -lgdbm

LIBUC_OBJS += index.o
LDFLAGS += -lmecab -lsenna
endif

ifdef USE_MOTIF
CXXFLAGS += -DUSE_MOTIF
APP04_OBJS += motif.o 
endif

ifdef USE_XAW3D
CXXFLAGS += -DXAW3D
CFLAGS += -DXAW3D
endif

ifdef USE_XVIEW
CXXFLAGS += -I$(OPENWIN_HOME)/include -DUSE_XVIEW
APP04_OBJS += olt.o 
endif

libuc.a: $(LIBUC_OBJS)
	$(AR) r $@ $(LIBUC_OBJS)

lib: libuc.a

app04: $(APP04_OBJS) libuc.a

header-test.o: subcmd.h xt-proc.h
subcmd.o: subcmd.h
csv-util.o csv-tool.o: csv.hpp
motif.o: mrmtest.uid

mrmtest.uid: mrmtest.uil
	uil -s -o $@ $<

rpm: 
	tar zcf $(RPMBASE)/$(APP_NAME)-$(VERSION).tar.gz \
 --exclude CVS --exclude .git\* --exclude \*.o \
 --exclude \*~ --exclude \*.SEN\* --exclude work .
	rpmbuild -ba $(APP_NAME).spec

install: all
	mkdir -p  $(DESTDIR)$(MYBINDIR)
	install -c -t $(DESTDIR)$(MYBINDIR) $(ALL_MODULE) run.sh *.uid
	install -c -t $(DESTDIR)$(MYBINDIR) *.a
	mkdir -p  $(DESTDIR)$(MYBINDIR)/uc
	install -c -t $(DESTDIR)$(MYBINDIR)/uc *.h *.hpp
	doxygen
	mkdir -p $(DESTDIR)$(MYDOCDIR) 
	test -d html && cp -Rf html $(DESTDIR)$(MYDOCDIR) || true
	rmdir -p $(DESTDIR)$(MYDOCDIR) || true

clean :
	rm -rf *~ \#*\# *.o $(ALL_MODULE)

