CC=g++
CFLAGS=-g -O0 -Wall -Werror -static -DENABLE_HTREE
LDFLAGS=-L/usr/lib -lext2fs -lcom_err

EXECS=e2blockmap e2moveblocks

all: $(EXECS)

e2moveblocks: ext3_layout_optimize.cpp
	$(CC) $(CFLAGS) ext3_layout_optimize.cpp -o e2moveblocks $(LDFLAGS)

e2blockmap: e2blockmap.cpp
	$(CC) $(CFLAGS) e2blockmap.cpp -o e2blockmap $(LDFLAGS)
	
clean:
	@rm -f *.o $(EXECS)