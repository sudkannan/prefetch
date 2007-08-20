CC=g++
CFLAGS=-g -O0 -Wall -Werror -static -DENABLE_HTREE -DNDEBUG
LDFLAGS=-L/usr/lib -lext2fs -lcom_err

EXECS=e2blockmap e2moveblocks

all: $(EXECS)
install: $(EXECS)
	install -D -m 755 e2moveblocks $(DESTDIR)/sbin/
	install -D -m 755 e2blockmap $(DESTDIR)/bin/
	
uninstall:
	@rm -f $(DESTDIR)/sbin/e2moveblocks
	@rm -f $(DESTDIR)/bin/e2blockmap

e2moveblocks: ext3_layout_optimize.cpp
	$(CC) $(CFLAGS) ext3_layout_optimize.cpp -o e2moveblocks $(LDFLAGS)

e2blockmap: e2blockmap.cpp
	$(CC) $(CFLAGS) e2blockmap.cpp -o e2blockmap $(LDFLAGS)
	
clean:
	@rm -f *.o $(EXECS)