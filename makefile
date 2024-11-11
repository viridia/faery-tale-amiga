CFLAGS = -pp -hi amiga39.pre -qf

OBJS = fmain.o fsubs.o narr.o fmain2.o iffsubs.o gdriver.o makebitmap.o hdrive.o

POBJS = fmain.p fmain2.p iffsubs.p

.c.p:
	cc $(CFLAGS) -qsp -o $@ $*.c

fmain: $(OBJS)
	ln +c -o fmain $(OBJS) -lc

proto: $(POBJS)
