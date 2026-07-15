module_LTLIBRARIES = kafka.la
kafka_la_SOURCES   = kafka.cpp
kafka_la_LDFLAGS   = -module -shared -version-info $(MODULE_VERSION_INFO)
kafka_la_LIBADD    = -lrdkafka++
pkgconfig_DATA = slimts.pc
