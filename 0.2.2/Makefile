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
	
uninstall:
	@rm -f $(DESTDIR)/usr/sbin/prefetch-process-trace
	@rm -f $(DESTDIR)/bin/prefetch-print-trace

prefetch-print-trace: prefetch-print-trace.cpp

prefetch-process-trace: prefetch-process-trace.cpp

clean: 
	@rm -f $(EXECS)

