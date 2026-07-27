module_LTLIBRARIES   = boringssl.la
boringssl_la_SOURCES = boringssl.cpp
boringssl_la_LDFLAGS = -module -shared -version-info $(MODULE_VERSION_INFO)
boringssl_la_LIBADD  = -lssl -lpki -lcrypto
