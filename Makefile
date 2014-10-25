# Makefile for the test programs

.SUFFIXES: .o .cpp

# Compiler and loader definitions

LD = ld
LDFLAGS =

CXX = g++
CXXFLAGS = -g -Wall

PURIFY = purify -collector=/usr/ccs/bin/ld -g++

# general definitions

MAKEFILE = Makefile

# list of all object and source files

OBJS =  db.o buf.o bufHash.o error.o page.o testbuf.o 
OBJS2 =  db.o buf.o bufHash.o error.o
SRCS =	db.cpp buf.cpp bufHash.cpp error.cpp page.cpp testbuf.cpp 

all:		testbuf 

testbuf:	$(OBJS) 
		$(CXX) -o $@ $(OBJS) $(LDFLAGS)

##testBhash:	$(OBJS2) 
##		$(CXX) -o $@ $(OBJS2) $(LDFLAGS)

testbuf.pure:	$(OBJS) 
		$(PURIFY) $(CXX) -o $@ $(OBJS) $(LDFLAGS)

.cpp.o:
		$(CXX) $(CXXFLAGS) -c $<

clean:
		rm -f core \#* *.bak *~ *.o test.1 test.2 test.3 test.4 testbuf testbuf.pure .pure

depend:
		makedepend -I /s/gcc/include/g++ -f$(MAKEFILE) \
		$(SRCS)

# DO NOT DELETE THIS LINE -- make depend depends on it.
