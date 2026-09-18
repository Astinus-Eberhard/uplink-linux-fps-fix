CC = gcc
CFLAGS = -Wall -O2 -fPIC
LDLIBS = -ldl

libbitmapcache.so: bitmapcache.c
	$(CC) $(CFLAGS) -shared -o $@ $< $(LDLIBS)

clean:
	rm -f libbitmapcache.so

.PHONY: clean
