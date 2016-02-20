CC=gcc
HEADERS=libbbssms.h protocol.h

all: libbbssms.a

libbbssms.so: libbbssms.o
	$(CC) -shared -fPIC -o $@ $^
%.o:  %.c $(HEADERS)
	$(CC) -static -fPIC -c $< -o $@

libbbssms.a: libbbssms.c
	$(CC) -c -static libbbssms.c
	ar cr libbbssms.a libbbssms.o
	ranlib libbbssms.a

.PHONY:clean

clean:
	rm -rf libbbssms.o libbbssms.so libbbssms.a
