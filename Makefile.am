SUBDIRS = src 
ACLOCAL_AMFLAGS = -I m4

	
EXTRA_DIST = changelog

install-data-hook:
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/lib/@PACKAGE_NAME@
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/lib/@PACKAGE_NAME@/tmp
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/lib/@PACKAGE_NAME@/data2kv
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/run/kvalobs
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/log/kvalobs
	$(install_sh) -m 755 -d $(DESTDIR)/$(sysconfdir)/kvalobs
	$(install_sh) -m 755 -d $(DESTDIR)/$(datadir)/@PACKAGE_NAME@

#pkgconfig_DATA = @PACKAGE_TARNAME@.pc
#$(pkgconfig_DATA): config.status

include mk/debian.mk