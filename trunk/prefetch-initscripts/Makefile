EXECS=prefetch-print-trace prefetch-process-trace
CXXFLAGS=-g -Wall

#Note: prefetch-print-trace is in /bin, as it will be needed during 
#reboot in the future

all: $(EXECS)

install:
	install -d $(DESTDIR)/usr/sbin
	install -d $(DESTDIR)/bin
	install -m 0755 prefetch-process-trace $(DESTDIR)/usr/sbin/
	install -m 0755 prefetch-print-trace $(DESTDIR)/bin/
	install -m 0755 split-prefetch-trace.py $(DESTDIR)/bin/split-prefetch-trace
	
uninstall:
	@rm -f $(DESTDIR)/usr/sbin/prefetch-process-trace
	@rm -f $(DESTDIR)/bin/prefetch-print-trace
	@rm -f $(DESTDIR)/bin/split-prefetch-trace

prefetch-print-trace: prefetch-print-trace.cpp prefetch_lib.cpp

prefetch-process-trace: prefetch-process-trace.cpp prefetch_lib.cpp

clean: 
	@rm -f $(EXECS)

