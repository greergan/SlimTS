#https://stackoverflow.com/questions/70584133/when-using-libadd-in-automake-libtool-links-incorrectly
# might need this for windows linking -> libbar_la_LDFLAGS += -no-undefined
bin_PROGRAMS  = slimTCP
#slim_SOURCES = example.cpp slim_utilities.cpp slim_common_log.cpp
slim_SOURCES  = main.cpp \
				slimTCP.cpp \
				../slim_common_http_header.cpp \
				slim_common_http_request.cpp \
				slim_common_http_response.cpp \
				slim_common_http_parser.cpp \
				slim_common_http_utilities.cpp \
				slim_common_log.cpp \
				slim_common_metrics.cpp \
				slim_common_network_address_set.cpp \
				slim_common_network_client_tcp.cpp \
				slim_common_network_client_connection_handler_common.cpp \
				slim_common_network_client_request_handler_metrics.cpp \
				slim_common_network_listener.cpp \
				slim_common_utilities.cpp \
				slim_common_validators.cpp \
				slim_common_vector_buffer.cpp \
				slimTCP_metrics_server.cpp \

				
slim_CPPFLAGS = -std=c++23
slim_LDFLAGS  = -static-libgcc -static-libstdc++ -pthread 
slim_LDADD    = -ldl -lssl -lcrypto
