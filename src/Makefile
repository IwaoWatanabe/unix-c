
CXXFLAGS=-g -fexceptions
LDFLAGS= -lstdc++ 
ALL_MODULE=hello cat01
ALL_MODULE += app01
ALL_MODULE += app02
ALL_MODULE += env01
ALL_MODULE += app03

all: $(ALL_MODULE)

app01: app01.o args.o

app03: app03.o subcmd.o stl.o term.o temp.o

clean :
	rm -rf *~ \#*\# *.o $(ALL_MODULE)

