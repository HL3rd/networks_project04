# Makefile

# Authors:
# 	Blake Trossen (btrossen)
# 	Horacio Lopez (hlopez1)

CC		= gcc
CFLAGS	= -lncurses -lpthread

TARGETS	= netpong
PHONY	= all clean

all: $(TARGETS)

netpong: netpong.c utils.c
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGETS)
