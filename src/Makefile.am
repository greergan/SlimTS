SUBDIRS=plugins

bin_PROGRAMS  = slimts
slimts_SOURCES  = main.cpp \
				slim.cpp \
				slim_builtins.cpp \
				slim_builtins_dummy_console_provider.cpp \
				slim_configuration_handler.cpp \
				slim_command_line_handler.cpp \
				slim_common_web_file.cpp \
				slim_exception_handler.cpp \
				slim_launchable_service_bin.cpp \
				slim_launchable_service_library_typescript.cpp \
				slim_macros.cpp \
				slim_metrics_server.cpp \
				slim_module_import_specifier.cpp \
				slim_module_resolver.cpp \
				slim_path.cpp \
				slim_plugin_loader.cpp \
				slim_queue.cpp \
				slim_service_launcher.cpp \
				slim_utilities.cpp \
				slim_v8.cpp \
				simdutf.cpp

slimts_CPPFLAGS  = -DV8_COMPRESS_POINTERS -DV8_ENABLE_SANDBOX -fno-rtti
slimts_LDFLAGS   = -static-libgcc -static-libstdc++ -pthread -rdynamic
slimts_LDADD     = -ldl -lv8_monolith -lv8_libbase -lv8_libplatform -lssl -lcrypto
