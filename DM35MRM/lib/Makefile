CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include -I../../CM35I2C/include
OPTIMIZE_FLAGS=-O2
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)
LIB_FILES:= \
	librtd-dm35mrm.o

all:			librtd-dm35mrm.a

librtd-dm35mrm.a:	$(LIB_FILES)
	ar -cr librtd-dm35mrm.a $(LIB_FILES)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a *~

