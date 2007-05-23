CC=g++
CFLAGS=-g -O0 -Wall -Werror
LDFLAGS=-L/usr/lib -lext2fs

EXECS=e2blockmap ext3_optimize

all: $(EXECS)

ext3_optimize: ext3_layout_optimize.cpp
	$(CC) $(CFLAGS) ext3_layout_optimize.cpp -o ext3_optimize $(LDFLAGS)

e2blockmap: e2blockmap.cpp
	$(CC) $(CFLAGS) e2blockmap.cpp -o e2blockmap $(LDFLAGS)
	
clean:
	@rm -f *.o $(EXECS)