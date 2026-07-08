CC=gcc
DEBUG_FLAGS=-g
INCLUDE_FLAGS=-I../include -I../../CM35I2C/include
LIBRARY_FLAGS=-L../lib -L../../CM35I2C/lib -lrtd-dm35mrm -lrtd-cm35i2c -lpthread -lm 
OPTIMIZE_FLAGS=-O6
WARNING_FLAGS=-Wall
CFLAGS:=$(DEBUG_FLAGS) $(INCLUDE_FLAGS) $(OPTIMIZE_FLAGS) $(WARNING_FLAGS)

EXAMPLES = \
	dm35mrm_write_relay_immediate \
	dm35mrm_csv_import \
	dm35mrm_csv_export \
	dm35mrm_delay_write_test \
	dm35mrm_pin_connection \
	dm35mrm_get_version \


all:    $(EXAMPLES)

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

dm35mrm_pin_connection: dm35mrm_pin_connection.c
	$(CC) $(CFLAGS) -o dm35mrm_pin_connection dm35mrm_pin_connection.c \
	$(LIBRARY_FLAGS)

dm35mrm_get_version: dm35mrm_get_version.c
	$(CC) $(CFLAGS) -o dm35mrm_get_version dm35mrm_get_version.c \
	$(LIBRARY_FLAGS)

clean:
	rm -f *.o *.dat *~ $(EXAMPLES)
