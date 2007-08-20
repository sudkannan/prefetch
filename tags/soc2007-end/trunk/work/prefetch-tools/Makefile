EXECS=prefetch-print-trace prefetch-process-trace
CXXFLAGS=-g -Wall

all: $(EXECS)

install:
	install -m 0755 prefetch-process-trace /usr/sbin/
	install -m 0755 prefetch-print-trace /usr/bin/

prefetch-print-trace: prefetch-print-trace.cpp

prefetch-process-trace: prefetch-process-trace.cpp

clean: 
	@rm -f $(EXECS)

