
CXXFLAGS=-g -pthread -fexceptions -D_REENTRANT 
LDFLAGS= -lstdc++ 
ALL_MODULE=hello cat01
ALL_MODULE += app01
ALL_MODULE += app02
ALL_MODULE += env01
ALL_MODULE += app03

LDFLAGS += -lpthread
LDFLAGS += -lXm -lMrm -lUil
LDFLAGS += -lXaw
LDFLAGS += -lXt -lXmu -lX11 -lSM -lICE 
LDFLAGS += -lncursesw

VERSION=0.1
RPMBASE=~/rpm/SOURCES

APP_NAME=examples-cpp
PREFIX=/opt/$(APP_NAME)

MYBINDIR=${PREFIX}
MYDOCDIR=${PREFIX}/docs
DESTDIR=

all: $(ALL_MODULE)

docs: 
	doxygen

app01: app01.o args.o

csv-util.o csv-tool.o: csv.h
APP03_OBJS = app03.o subcmd.o stl.o term.o temp.o csv-util.o csv-tool.o mbs.o
APP03_OBJS += times.o logger.o
APP03_OBJS += xwin.o 
APP03_OBJS += xt-proc.o
APP03_OBJS += awt.o awt-util.o SmeCascade.o
APP03_OBJS += motif.o 

app03: $(APP03_OBJS)

rpm: 
	tar zcf $(RPMBASE)/$(APP_NAME)-$(VERSION).tar.gz \
 --exclude CVS --exclude .git\* --exclude \*.o \
 --exclude \*~ --exclude \*.SEN\* --exclude work .
	rpmbuild -ba $(APP_NAME).spec

install: all
	mkdir -p  $(DESTDIR)$(MYBINDIR)
	install -c -t $(DESTDIR)$(MYBINDIR) $(ALL_MODULE) run.sh
	doxygen
	mkdir -p $(DESTDIR)$(MYDOCDIR) 
	test -d html && cp -Rf html $(DESTDIR)$(MYDOCDIR) || true
	rmdir -p $(DESTDIR)$(MYDOCDIR) || true

clean :
	rm -rf *~ \#*\# *.o $(ALL_MODULE)

