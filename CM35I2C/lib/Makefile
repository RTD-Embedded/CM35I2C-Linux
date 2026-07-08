CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include
OPTIMIZE_FLAGS=-O2
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)
LIB_FILES:= \
	librtd-cm35i2c_util.o \
	cm35i2c_board_access.o \
	cm35i2c_os.o \
	librtd-cm35i2c_gbc.o \
	librtd-cm35i2c_i2c3300.o \

all:			librtd-cm35i2c.a

librtd-cm35i2c.a:	$(LIB_FILES)
	ar -cr librtd-cm35i2c.a $(LIB_FILES)

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.a *~

