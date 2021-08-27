AM_CPPFLAGS = -D_REENTRANT \
              $(putools_CFLAGS) \
              $(kvcpp_CFLAGS) \
              $(BOOST_CPPFLAGS) \
              $(omniORB4_CFLAGS)  

bin_PROGRAMS = norcom2kv
noinst_PROGRAMS = testWMORaport
norcom2kv_SOURCES = norcom2kv.cc \
                    CollectWmoReports.cc CollectWmoReports.h \
                    App.cc App.h \
                    WMORaport.cc WMORaport.h \
                    crc_ccitt.cc crc_ccitt.h \
                    AdminImpl.cc AdminImpl.h \
                    File.cc File.h \
                    InitLogger.cc InitLogger.h \
                    FInfo.h \
                    kvDataSrcList.h \
                    decodeArgv0.cc decodeArgv0.h

norcom2kv_CPPFLAGS = $(AM_CPPFLAGS) 
norcom2kv_LDFLAGS = -pthread 
norcom2kv_LDADD = $(kvcpp_LIBS) \
                  $(putools_LIBS) $(omniORB4_LIBS)\
                  $(BOOST_REGEX_LIB) $(BOOST_THREAD_LIB) \
                  $(BOOST_FILESYSTEM_LIB) $(BOOST_SYSTEM_LIB) \
                  -lm -ldl 

testWMORaport_SOURCES = \
	testWMORaport.cc \
	decodeArgv0.cc decodeArgv0.h \
	WMORaport.cc WMORaport.h 

testWMORaport_CPPFLAGS = $(AM_CPPFLAGS) \
                         -DSYSCONFDIR="\""$(sysconfdir)"\"" 
testWMORaport_LDFLAGS = -pthread 
testWMORaport_LDADD = $(kvcpp_LIBS) \
              $(putools_LIBS) \
              $(BOOST_REGEX_LIB) $(BOOST_THREAD_LIB) \
              $(BOOST_FILESYSTEM_LIB) \
              $(BOOST_SYSTEM_LIB) \
              -lm -ldl 

