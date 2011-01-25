SUBDIRS = src share/bufrtables
ACLOCAL_AMFLAGS = -I m4
sqldir = $(datadir)/kvbufrd
sql_DATA = cleanbufrdb.sql 
bin_SCRIPTS = kvbufrdbadmin check_kvbufrd.sh

SED_SUBST = sed s,KVCONFIG,$(bindir)/kvconfig,g $< > $@ 

kvbufrdbadmin: kvbufrdbadmin.sh 
	$(SED_SUBST)
	
EXTRA_DIST = cleanbufrdb.sql kvbufrdbadmin.sh changelog

CLEANFILES = kvbufrdbadmin

install-data-hook:
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/lib/@PACKAGE_NAME@
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/lib/@PACKAGE_NAME@/debug
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/run/kvalobs
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/log/kvalobs
	$(install_sh) -m 755 -d $(DESTDIR)/$(localstatedir)/log/kvalobs/kvbufr
	$(install_sh) -m 755 -d $(DESTDIR)/$(datadir)/@PACKAGE_NAME@

#pkgconfig_DATA = @PACKAGE_TARNAME@.pc
#$(pkgconfig_DATA): config.status

include mk/debian.mk