# Ways of measuring OpenOffice startup time #

Note: this page is about means to measure time. To see test results, go to OpenOfficeStartupTimeResults page.

## Start with document which runs program when it is loaded ##
The idea is to prepare a document which upon loading calls a program which will record the time and/or other metrics.

This approach works, preliminary script is in [trunk/tools/openoffice in SVN](http://prefetch.googlecode.com/svn/trunk/tools/openoffice/).

Preparation for measurements:
  * Add path with measurements documents to safe macro paths: in menu  Tools->Options->OpenOffice.org->Security->Macro security->Trusted sources->Trusted file locations. Otherwise OpenOffice will ask for confirmation of running macro in document, spoiling the measurement.

# Ways of measuring time which did not work #
## Start with document which calls macro to quit OpenOffice ##

The idea is to prepare a document which upon loading calls macro which quits OpenOffice.

This approach does not work due to bug in OpenOffice - when quit command is executed, OpenOffice crashes.

Adding a 10 seconds delay before quitting OpenOffice does not help - apparently calling quit from Basic macro is not supported.

See [bug 57748](http://www.openoffice.org/issues/show_bug.cgi?id=57748) in OpenOffice.

# Useful links #
  * [Cold start simulator](http://go-oo.org/~michael/howto-cold-start.c)