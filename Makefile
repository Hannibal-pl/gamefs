CC = gcc
LIBS = -L/usr/lib64 -L/lib64 -lfuse -lz
INCLUDE = -I/usr/include -I/usr/include/fuse
CFLAGS = -Wall -pipe -O0 -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 -g -std=gnu99
GAMEMOD = $(patsubst %.c,%.o,$(foreach sdir,modules,$(wildcard $(sdir)/*.c)))
BASEMOD = gamefs.o generic.o tools.o

all: gamefs

gamefs.o: modules.h

$(BASEMOD): %.o: %.c gamefs.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(GAMEMOD): %.o: %.c gamefs.h
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

gamefs: $(BASEMOD) $(GAMEMOD)
	$(CC) $(CFLAGS) $(LIBS) $(BASEMOD) $(GAMEMOD) -o gamefs

clean:
	rm -f *.o modules/*.o gamefs

rebuild: clean all

.PHONY: all clean rebuild