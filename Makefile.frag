XHP_SHARED_DEPENDENCIES = $(srcdir)/xhp/libxhp.dylib
XHP_SHARED_LIBADD := ${XHP_SHARED_LIBADD}
$(srcdir)/ext.cpp: $(srcdir)/xhp/libxhp.dylib
$(srcdir)/xhp/libxhp.dylib: FORCE
	$(MAKE) $(MFLAGS) -C $(srcdir)/xhp libxhp.dylib

FORCE:
