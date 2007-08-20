.PHONY: clean all

CFLAGS = -Wall -ggdb -D _GNU_SOURCE -D_FILE_OFFSET_BITS=64
LDFLAGS = -lext2fs -lcom_err

all: e2block2file e2remapblocks preloadblocks

e2block2file: e2block2file.c

e2remapblocks: e2remapblocks.c

preloadblocks: preloadblocks.c

clean:
	rm e2block2file e2remapblocks preloadblocks
