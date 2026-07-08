CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include
LIBRARY_FLAGS=-L../lib -lrtd-cm35i2c -lpthread -lm 
OPTIMIZE_FLAGS=-O6
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)

EXAMPLES = \
	cm35i2c_list_fb \
	cm35i2c_send \
	cm35i2c_set_clock_rate \

all:    $(EXAMPLES)

cm35i2c_list_fb: cm35i2c_list_fb.c
	$(CC) $(CFLAGS) -o cm35i2c_list_fb cm35i2c_list_fb.c \
	$(LIBRARY_FLAGS)

cm35i2c_send: cm35i2c_send.c
	$(CC) $(CFLAGS) -o cm35i2c_send cm35i2c_send.c \
	$(LIBRARY_FLAGS)

cm35i2c_set_clock_rate: cm35i2c_set_clock_rate.c
	$(CC) $(CFLAGS) -o cm35i2c_set_clock_rate cm35i2c_set_clock_rate.c \
	$(LIBRARY_FLAGS)

dm35mrm_write_relay_immediate: dm35mrm_write_relay_immediate.c
	$(CC) $(CFLAGS) -o dm35mrm_write_relay_immediate dm35mrm_write_relay_immediate.c \
	$(LIBRARY_FLAGS)

dm35mrm_csv_import: dm35mrm_csv_import.c
	$(CC) $(CFLAGS) -o dm35mrm_csv_import dm35mrm_csv_import.c \
	$(LIBRARY_FLAGS)

dm35mrm_csv_export: dm35mrm_csv_export.c
	$(CC) $(CFLAGS) -o dm35mrm_csv_export dm35mrm_csv_export.c \
	$(LIBRARY_FLAGS)

dm35mrm_delay_write_test: dm35mrm_delay_write_test.c
	$(CC) $(CFLAGS) -o dm35mrm_delay_write_test dm35mrm_delay_write_test.c \
	$(LIBRARY_FLAGS)


clean:
	rm -f *.o *.dat *~ $(EXAMPLES)
