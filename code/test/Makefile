# use normal make for this Makefile
#
# Makefile for building user programs to run on top of Nachos
#
# Several things to be aware of:
#
#    Nachos assumes that the location of the program startup routine (the
# 	location the kernel jumps to when the program initially starts up)
#       is at location 0.  This means: start.o must be the first .o passed 
# 	to ld, in order for the routine "Start" to be loaded at location 0
#

# If you are running on SunOS 5.x machines, uncomment the next line
GCCDIR = ../gnu/
LDFLAGS = -T newscript -N
ASFLAGS = -mips1
CPPFLAGS = $(INCDIR)

# if you aren't cross-compiling:
# GCCDIR =
# LDFLAGS = -N -T 0
# ASFLAGS =
# CPPFLAGS = -P $(INCDIR)


CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
CPP = $(GCCDIR)cpp

INCDIR =-I../userprog -I../threads
CFLAGS = -G 0 -c $(INCDIR)

all: halt matmult matmult2 sort testfiles ppo_main customer ApplicationClerk PictureClerk PassportClerk Cashier

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	rm strt.s

halt.o: halt.c
	$(CC) $(CFLAGS) -c halt.c
halt: halt.o start.o
	$(LD) $(LDFLAGS) start.o halt.o -o halt.coff
	../bin/coff2noff halt.coff halt

sort.o: sort.c
	$(CC) $(CFLAGS) -c sort.c
sort: sort.o start.o
	$(LD) $(LDFLAGS) start.o sort.o -o sort.coff
	../bin/coff2noff sort.coff sort

matmult.o: matmult.c
	$(CC) $(CFLAGS) -c matmult.c
matmult: matmult.o start.o
	$(LD) $(LDFLAGS) start.o matmult.o -o matmult.coff
	../bin/coff2noff matmult.coff matmult

matmult2.o: matmult2.c
	$(CC) $(CFLAGS) -c matmult2.c
matmult2: matmult2.o start.o
	$(LD) $(LDFLAGS) start.o matmult2.o -o matmult2.coff
	../bin/coff2noff matmult2.coff matmult2
	
testfiles.o: testfiles.c
	$(CC) $(CFLAGS) -c testfiles.c
testfiles: testfiles.o start.o
	$(LD) $(LDFLAGS) start.o testfiles.o -o testfiles.coff
	../bin/coff2noff testfiles.coff testfiles
	
	
customer.o: customer.c
	$(CC) $(CFLAGS) -c customer.c
customer: customer.o start.o
	$(LD) $(LDFLAGS) start.o customer.o -o customer.coff
	../bin/coff2noff customer.coff customer
	
ppo_main.o: ppo_main.c
	$(CC) $(CFLAGS) -c ppo_main.c
ppo_main: ppo_main.o start.o
	$(LD) $(LDFLAGS) start.o ppo_main.o -o ppo_main.coff
	../bin/coff2noff ppo_main.coff ppo_main
	
ApplicationClerk.o: ApplicationClerk.c
	$(CC) $(CFLAGS) -c ApplicationClerk.c
ApplicationClerk: ApplicationClerk.o start.o
	$(LD) $(LDFLAGS) start.o ApplicationClerk.o -o ApplicationClerk.coff
	../bin/coff2noff ApplicationClerk.coff ApplicationClerk
	
PictureClerk.o: PictureClerk.c
	$(CC) $(CFLAGS) -c PictureClerk.c
PictureClerk: PictureClerk.o start.o
	$(LD) $(LDFLAGS) start.o PictureClerk.o -o PictureClerk.coff
	../bin/coff2noff PictureClerk.coff PictureClerk
	
PassportClerk.o: PassportClerk.c
	$(CC) $(CFLAGS) -c PassportClerk.c
PassportClerk: PassportClerk.o start.o
	$(LD) $(LDFLAGS) start.o PassportClerk.o -o PassportClerk.coff
	../bin/coff2noff PassportClerk.coff PassportClerk
	
Cashier.o: Cashier.c
	$(CC) $(CFLAGS) -c Cashier.c
Cashier: Cashier.o start.o
	$(LD) $(LDFLAGS) start.o Cashier.o -o Cashier.coff
	../bin/coff2noff Cashier.coff Cashier
	
clean:
	rm -f *.o *.coff
