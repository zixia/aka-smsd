CC=gcc
HEADERS=sendsms.h libbbssms.h

all: sendsms akasmsd

sendsms: sendsms.c
	$(CC) -o sendsms sendsms.c
akasmsd: akasmsd.c
	$(CC) -o akasmsd akasmsd.c -lbbssms -L.
.PHONY:clean

clean:
	rm -rf sendsms akasmsd
