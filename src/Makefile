
CXXFLAGS=-g -fexceptions
LDFLAGS= -lstdc++ 
ALL_MODULE=hello cat01
ALL_MODULE += app01
ALL_MODULE += app02
ALL_MODULE += env01

all: $(ALL_MODULE)

app01: app01.o args.o

clean :
	rm -rf *~ \#*\# *.o $(ALL_MODULE)

