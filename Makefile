CC ?= gcc
CFLAGS = -std=c99 -Wall -Wextra -O2

stardate: stardate.c Makefile
	$(CC) $(CFLAGS) stardate.c -o stardate

.PHONY: clean test
test: stardate
	./test_stardate.sh

clean:
	rm -f stardate
