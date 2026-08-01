SUBDIRS = plugins

bin_PROGRAMS  = slimts
slimts_SOURCES  = main.cpp \
				slim.cpp \
				slim_builtins.cpp \
				slim_builtins_dummy_console_provider.cpp \
				slim_configuration_handler.cpp \
				slim_command_line_handler.cpp \
				slim_exception_handler.cpp \
				slim_file_watcher.cpp \
				slim_isolate_wake.cpp \
				slim_module_import_specifier.cpp \
				slim_module_resolver.cpp \
				slim_runtime.cpp \
				slim_path.cpp \
				slim_plugin_loader.cpp \
				slim_queue.cpp \
				slim_service_handles.cpp \
				slim_service_launcher.cpp \
				slim_utilities.cpp \
				slim_v8.cpp \
				simdutf.cpp

#slimts_LDFLAGS = -static-libgcc -static-libstdc++ -pthread -rdynamic -Wl,--whole-archive -l:libslimcommon.a -Wl,--no-whole-archive
#slimts_LDADD = -ldl -lv8_monolith -lv8_libbase -lv8_libplatform -lssl -lcrypto -ltsgo

# Bundle the whole-archive block into a single string with commas
slimts_LDFLAGS = -static-libgcc -static-libstdc++ -pthread -rdynamic -Wl,--whole-archive,-l:libslimcommon.a,--no-whole-archive
slimts_LDADD   = -ldl -lv8_monolith -lv8_libbase -lv8_libplatform -lssl -lcrypto -ltsgo -lhwy

#slimts_LDADD   = -ldl -lv8_monolith -lv8_libbase -lv8_libplatform -lssl -lcrypto -ltsgo -l:libslimcommon.a

# Define variables to bypass Automake's strict checks
#WL_WHOLE = -Wl,--whole-archive
#WL_NO_WHOLE = -Wl,--no-whole-archive

#slimts_LDFLAGS = -static-libgcc -static-libstdc++ -pthread -rdynamic $(WL_WHOLE) -l:libslimcommon.a $(WL_NO_WHOLE)
#slimts_LDADD   = -ldl -lv8_monolith -lv8_libbase -lv8_libplatform -lssl -lcrypto -ltsgo

#all-local:
#	strip --strip-unneeded $(builddir)/slimts
