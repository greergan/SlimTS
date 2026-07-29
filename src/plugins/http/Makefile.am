module_LTLIBRARIES = http.la
http_la_SOURCES = \
    headers_object.cpp \
    request_object.cpp \
    response_object.cpp \
    connection_handler.cpp \
    listener.cpp \
    http_plugin.cpp
http_la_CPPFLAGS = -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -fno-rtti -fsanitize=address
http_la_LDFLAGS = -module -shared -version-info $(MODULE_VERSION_INFO) -fsanitize=address
#pkgconfig_DATA = slimts.pc
