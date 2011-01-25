AM_CPPFLAGS = -D_REENTRANT \
              $(kvcpp_CFLAGS) \
              $(BOOST_CPPFLAGS) \
              $(omniORB4_CFLAGS)  

bin_PROGRAMS = norcom2kv

norcom2kv_SOURCES = norcom2kv.cc \
                    CollectSynop.cc  CollectSynop.h \
                    App.cc App.h \
                    WMORaport.cc WMORaport.h \
                    crc_ccitt.cc crc_ccitt.h \
                    CorbaThread.cc CorbaThread.h \
                    AdminImpl.cc AdminImpl.h \
                    File.cc File.h \
                    InitLogger.cc InitLogger.h \
                    FInfo.h \
                    kvDataSrcList.h

norcom2kv_CPPFLAGS = $(AM_CPPFLAGS)
norcom2kv_LDFLAGS = -pthread 
norcom2kv_LDADD = $(kvcpp_LIBS) \
                  $(putools_LIBS) $(omniORB4_LIBS) $(BOOST_REGEX_LIB) $(BOOST_THREAD_LIB) \
                  -lm -ldl 

