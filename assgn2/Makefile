CC 	= gcc

CFLAGS  = -Wall -g -I .

LD 	= gcc

LDFLAGS  = -Wall -g -L/home/pn-cs453/Given/Asgn2

PUBFILES =  README  hungrymain.c  libPLN.a  libsnakes.a  lwp.h\
	    numbersmain.c  snakemain.c  snakes.h

TARGET =  pn-cs453@hornet:Given/asgn2

PROGS	= snakes nums hungry

SNAKEOBJS  = snakemain.o 

HUNGRYOBJS = hungrymain.o 

NUMOBJS    = numbersmain.o

OBJS	= $(SNAKEOBJS) $(HUNGRYOBJS) $(NUMOBJS) liblwp.o malloc.o 

SRCS	= snakemain.c numbersmain.c

HDRS	= 

EXTRACLEAN = core $(PROGS)i

liblwp.so: clean
	$(CC) $(CFLAGS) -g -fPIC -m64 -c lwp.c lwp.h
	$(CC) $(CFLAGS) -g -fPIC -m64 -c -o magic64.o magic64.S
	$(CC) $(CFLAGS) -g -fPIC -shared -o $@ lwp.o magic64.o

all: 	$(PROGS)

allclean: clean
	@rm -f $(EXTRACLEAN)

clean:	
	rm -f $(OBJS) *~ TAGS
	rm -f liblwp.so

snakes: snakemain.o libPLN.a libsnakes.a
	$(LD) $(LDFLAGS) -o snakes snakemain.o -L. -lncurses -lsnakes -lPLN

hungry: hungrymain.o libPLN.a libsnakes.a
	$(LD) $(LDFLAGS) -o hungry hungrymain.o -L. -lncurses -lsnakes -lPLN

nums: numbersmain.o libPLN.a 
	$(LD) $(LDFLAGS) -o nums numbersmain.o -L. -lPLN

hungrymain.o: lwp.h snakes.h

snakemain.o: lwp.h snakes.h

numbermain.o: lwp.h

#libPLN.a: ../Publish/lwp.c
#	gcc -c ../Publish/lwp.c
#	ar r libPLN.a lwp.o
#	rm lwp.o
pub:
	scp $(PUBFILES) $(TARGET)

